/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/*
 * File : index.c 
 *        functions dealing with file indices
 */


# include "index.h"


INDEX_CLUSTER *cluster[MAX_CLUSTER];

NFS_INDEX *mounted = NULL;
NFS_MOUNT_OPT *opt_list = NULL;


void
init_index (void)
{
	long i;
	
	for (i = 0; i < MAX_CLUSTER; i++)
		cluster [i] = NULL;
	
	mounted = NULL;
}


# if 0
/* this is for debugging: these functions do a dump of used nfs indices
 * so that one can see whats going on (hopefully)
 */
void
index_statistics (void)
{
	int i,j;
	NFS_INDEX *ni;

	DEBUG(("Index statistics"));
	for (i = 0; i < MAX_CLUSTER;  i++)
		if (cluster[i])
		{
			DEBUG(("cluster %ld has %ld used indices",
			            (long)i, (long)cluster[i]->n_used));
			for (j = 0; j < CLUSTER_SIZE;  j++)
			{
				ni = &cluster[i]->index[j];
				if (ni->link > 0)
					DEBUG(("C %ld, I %ld, L %ld, '%s' in '%s'",(long)i,(long)j,
					           ni->link,ni->name,
					           (ni->dir) ? ni->dir->name : "root"));
			}
		}
}


void
do_mountdump (void)
{
	NFS_INDEX *ni;

	DEBUG(("Dump of mounted directories"));
	for (ni = mounted;  ni;  ni = ni->next)
	{
		DEBUG(("'%s' in dir '%s', L %ld, F 0x%lx", ni->name,
		            (ni->dir) ? ni->dir->name : "root", ni->link, ni->flags));
	}
}
# endif


void
init_mount_attr (XATTR *ap)
{
	ap->mode = DEFAULT_DMODE | S_IFDIR;
	ap->attr = FA_DIR;
	ap->nlink = 1;
	ap->uid = ap->gid = 0;
	ap->size = 0;
	ap->blksize = 1024;
	ap->nblocks = 0;
	ap->rdev = 0;
	
	*((long *) &(ap->atime)) = CURRENT_TIME;
	*((long *) &(ap->mtime)) = CURRENT_TIME;
	*((long *) &(ap->ctime)) = CURRENT_TIME;
	
	ap->reserved2 = 0;
	ap->reserved3[0] = 0;
	ap->reserved3[1] = 0;
}



/* get a slot in the root directory for a mounted dir, so we can either
 * set up a new one or know about an already existing one.
 */
NFS_INDEX *
get_mount_slot (const char *name, NFS_MOUNT_INFO *info)
{
	NFS_MOUNT_OPT *opt;
	NFS_INDEX *ni;
	
	DEBUG(("get_mount_slot: for %s (server %s)", name, info->hostname));
	
	if (info->version != NFS_MOUNT_VERS)
	{
		DEBUG(("get_mount_slot: wrong version of mount program!"
		       " Got %ld, expected %ld", info->version, NFS_MOUNT_VERS));
		return NULL;
	}
	
	if (!name)
	{
		DEBUG(("get_mount_slot: no mount name specified"));
		return NULL;
	}
	
	if (name[0] == '\0')
	{
	illegal:
		DEBUG(("get_mount_slot: illegal name '%s'", name));
		return NULL;
	}
	
	if (name[0] == '.' && name[1] == '\0')
		goto illegal;
	
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
		goto illegal;
	
	ni = mounted;
	while (ni)
	{
		if (!strcmp (name, ni->name))
			return ni;
		
		ni = ni->next;
	}
	
	ni = kmalloc (sizeof (NFS_INDEX));
	if (!ni)
	{
		DEBUG(("get_mount_slot: no memory for slot"));
		return NULL;
	}
	
	opt = kmalloc (sizeof (NFS_MOUNT_OPT));
	if (!opt)
	{
		DEBUG (("get_mount_slot: no memory for mount options"));
		
		kfree (ni);
		return NULL;
	}
	
	strcpy (opt->server.hostname, info->hostname);
	DEBUG (("get_mount_slot: mounting for server '%s'",
	                                   opt->server.hostname));
	opt->next = NULL;
	opt->flags = info->flags;
	opt->server.flags = info->flags & SERVER_OPTS;
	opt->server.addr = info->server;

	/* set default values */
	opt->server.addr.sin_port = DEFAULT_PORT;
	opt->server.retrans = DEFAULT_RETRANS;
	opt->server.timeo = DEFAULT_TIMEO;
	opt->actimeo = DEFAULT_ACTIMEO;
	opt->rsize = DEFAULT_RSIZE;
	opt->wsize = DEFAULT_WSIZE;

	/* look for the optional values from the mount command */
	if (!(info->flags & OPT_USE_DEFAULTS))
	{
		if (info->server.sin_port > 0)
			opt->server.addr.sin_port = info->server.sin_port;
		if (info->retrans > 0)
			opt->server.retrans = info->retrans;
		if (info->timeo > 0)
			opt->server.timeo = info->timeo;
		if (info->actimeo > 0)
			opt->actimeo = info->actimeo;
		if (info->rsize > 0)
			opt->rsize = info->rsize;
		if (info->wsize > 0)
			opt->wsize = info->wsize;
	}
	
	ni->name = kmalloc (strlen (name) + 1);
	if (!ni->name)
	{
		kfree (opt);
		kfree (ni);
		return NULL;
	}
	
	strcpy (ni->name, name);
	
	ni->opt = opt;
	ni->dir = ROOT_INDEX;
	ni->aux = NULL;
	ni->cluster = NULL;
	ni->flags = IS_MOUNT_DIR;
	ni->link = 0;
	ni->search_val = 0;
	ni->stamp = 0;
	init_mount_attr(&ni->attr);

	ni->next = mounted;
	mounted = ni;

	DEBUG(("get_mount_slot: call succeeded for '%s'", ni->name));

	return ni;
}


int
release_mount_slot (NFS_INDEX *ni)
{
	NFS_INDEX *pn, **ppn;
	
	if (!(ni->flags & IS_MOUNT_DIR))
	{
		DEBUG(("release_mount_slot: is not a mounted dir"));
		return EACCES;
	}
	
	if (ni->link != 1)
	{
		DEBUG(("release_mount_slot: fs is still in use (%ld)!", ni->link));
		return EACCES;
	}
	
	kfree (ni->opt);
	kfree (ni->name);
	
	/* now unlink mount index from chain of mounted directories
	 */
	ppn = &mounted;
	for (pn = mounted; pn; pn = pn->next)
	{
		if (pn == ni)
		{
			*ppn = pn->next;
			kfree (ni);
			
			return 0;
		}
		ppn = &pn->next;
	}
	
	return 0;
}

void
init_cluster (INDEX_CLUSTER *icp, int number)
{
	long i;

	for (i = 0; i < CLUSTER_SIZE; i++)
	{
		icp->index[i].opt = 0L;
		icp->index[i].cluster = icp;
		icp->index[i].aux = 0L;
		icp->index[i].dir = NULL;
		icp->index[i].link = 0;
		icp->index[i].flags = 0;
		icp->index[i].name = NULL;
	}
	
	icp->n_used = 0;
	icp->next = NULL;
	icp->cl_no = number;
}

static long
mystricmp (const char *s1, const char *s2)
{
	return (long) (stricmp (s1, s2));
}

NFS_INDEX *
get_slot (NFS_INDEX *dir, const char *name, int dom)
{
	INDEX_CLUSTER *icp, **ipp;
	NFS_INDEX *ni;
	int i, j, n;
	long sval;
	char buf[5];
	long (*cmp)(const char *, const char *);
	
	if (!dir)
		DEBUG (("get_slot: PANIC -- no parent dir for '%s'", name));
	
	if (!stricmp (".", name) || !stricmp ("..", name))
		DEBUG (("get_slot: PANIC -- getting slot for '%s'", name));
	
	/* search through all indices to find an already allocated index
	 */
	*(long *) buf = 0L;
	strncpy (buf, name, 4);
	buf[4] = 0;
	strlwr (buf);
	sval = *(long *) buf;
	
	if (dom == 0)
		/* set comparision function, kludge for tos-domain */
		cmp = mystricmp;
	else
		cmp = strcmp;
	
	for (i = 0; i < MAX_CLUSTER - 1; i++)
	{
		icp = cluster[i];
		if (icp && (icp->n_used > 0))
		{
			for (j = 0; j < CLUSTER_SIZE; j++)
				if ((icp->index[j].dir==dir) && (icp->index[j].search_val==sval))
					if (!(*cmp)(name, icp->index[j].name))
						return &icp->index[j];
		}
	}
	
	icp = cluster[MAX_CLUSTER - 1];
	while (icp)
	{
		if (icp->n_used > 0)
		{
			for (j = 0; j < CLUSTER_SIZE; j++)
				if ((icp->index[j].dir == dir) && (icp->index[j].search_val == sval))
					if (!(*cmp)(name, icp->index[j].name))
						return &icp->index[j];
		}
		icp = icp->next;
	}
	
	/* get a new slot for name, as no old one is in use */
	ni = NULL;
	for (i = 0; i < MAX_CLUSTER; i++)
	{
		icp = cluster[i];
		if (!icp)
		{
			/* here is no cluster, get one
			 */
			
			if ((icp = cluster[MAX_CLUSTER - 1]))
			{
				/* if there is a linked list at the last
				 * cluster, try to unlink one and place it
				 * here to shorten the list
				 */
				ipp = &icp->next;
				icp = icp->next;
				if (icp)    /* we found a list element */
				{
					*ipp = icp->next;
					icp->next = NULL;
					cluster[i] = icp;
					icp->cl_no = i;
				}
			}
			else
			{
				/* no list found, so allocate a new cluster
				 */
				cluster[i] = icp = kmalloc (sizeof (INDEX_CLUSTER));
				if (!icp)
					/* out of memory */
					return NULL;
				
				init_cluster (icp, i);
				
				/* we take an index */
				icp->n_used += 1;
				ni = &icp->index[0];
				
				goto init_slot;
			}
		}
		
		if (icp->n_used < CLUSTER_SIZE)
		{
			for (j = 0;  j < CLUSTER_SIZE;  j++)
				if (0 == icp->index[j].link)
				{
					icp->n_used += 1;
					ni = &icp->index[j];
					goto init_slot;
				}
		}
	}
	
	if (!ni)
	{
		/* all indices used and all clusters allocated
		 * 
		 * try to look into a linked list beginning at the last cluster.
		 * If nothing is found, allocate a new cluster.
		 * NOTE: this is a not very efficient fallback method.
		 */
		n = MAX_CLUSTER;
		icp = cluster[MAX_CLUSTER - 1];
		ipp = &icp->next;
		icp = icp->next;
		while (icp)
		{
			if (icp->n_used < CLUSTER_SIZE)
			{
				for (j = 0; j < CLUSTER_SIZE; j++)
				{
					if (0 == icp->index[j].link)
					{
						icp->n_used += 1;
						ni = &icp->index[j];
						goto init_slot;
					}
				}
			}
			ipp = &icp->next;
			icp = icp->next;
			n += 1;
		}
		
		if (!ni)
		{
			/* still nothing found
			 *
			 * now ipp points to the last INDEX_CLUSTER.next pointer, so
			 * we can use it to link in a new cluster.
			 */
			icp = kmalloc (sizeof (INDEX_CLUSTER));
			if (!icp)
				/* out of memory */
				return NULL;
			
			init_cluster (icp, n);
			*ipp = icp;
			icp->n_used += 1;
			ni = &icp->index[0];
			goto init_slot;
		}
	}
	
	if (!ni)
		return NULL;
	
init_slot:
	
	if (ni->name)
	{
		DEBUG (("get_slot: internal error, file has already a name"));
		return NULL;
	}
	
	ni->search_val = sval;
	ni->aux = 0L;
	ni->link = 0;
	ni->flags = 0;
	ni->name = kmalloc (strlen (name) + 1);
	if (!ni->name)
	{
		ni->cluster->n_used -= 1;
		return NULL;
	}
	strcpy (ni->name, name);
	ni->dir = dir;
	dir->link += 1;
	ni->opt = dir->opt;
	
	/* init also the basic fields of the xattr struct, so there are some
	 * useful values, i.e. when opening a new file....
	 */
	ni->attr.size = 0;
	ni->attr.nblocks = 0;
	ni->attr.dev = 0;
	ni->stamp = 0;
	
	return ni;
}



/* free a cluster. Do only call if no index is in the lookup cache anymore */
void
free_cluster(INDEX_CLUSTER *icp)
{
	INDEX_CLUSTER **ipp, *cp;
	
	if (icp->cl_no >= MAX_CLUSTER)
	{
		cp = cluster[MAX_CLUSTER-1];
		ipp = &cp->next;
		cp = cp->next;
		while (cp && (icp != cp))
		{
			ipp = &cp->next;
			cp = cp->next;
		}
		
		if (!cp)
		{
			DEBUG(("free_cluster: internal inconsistency: cluster not found."));
			return;
		}
	}
	else
	{
		ipp = &cluster[icp->cl_no];
		if (icp->next)
			icp->next->cl_no = icp->cl_no;
	}
	
	*ipp = icp->next;
	
	kfree (icp);
}




void
free_slot (NFS_INDEX *ni)
{
	INDEX_CLUSTER *icp;
	NFS_INDEX *newi;
	
	while (ni)
	{
		if (ni->link > 0)
			return;
		
		if (ni->link < 0)
		{
			DEBUG (("free_slot: internal error, '%s'->link < 0", ni->name));
			return;
		}
		
		newi = ni->dir;
		if (newi)
			newi->link -= 1;
		
		if (ni->name)
		{
			ni->name[0] = '$';
			kfree (ni->name);
		}
		
		ni->name = NULL;
		if (ni->cluster)
		{
			ni->cluster->n_used--;
			if (ni->cluster->n_used < 0)
				DEBUG (("free_slot: internal inconsistency."));
			
			if (0 == ni->cluster->n_used)
			{
				icp = ni->cluster;
				free_cluster (icp);
			}
		}
		
		ni = newi;
	}
}


#if 0
/* When deleting a file we have to make sure that we don't keep an index
 * for that in order to prevent inconsistencies. The same applies for
 * directories.
 */
long
remove_slot_by_name(NFS_INDEX *dir, char *name)
{
	INDEX_CLUSTER *icp;
	NFS_INDEX *index;
	int i, j, dom;
	long sval;
	char buf[5];
	int (*cmp)(const char *, const char*);
	
	*(long *) buf = 0L;
	strncpy (buf, name, 4);
	buf[4] = 0;
	strlwr (buf);
	sval = *(long *) buf;
	dom = p_domain (-1);
	if (dom == 0)      /* set comparision function, kludge for tos-domain */
		cmp = (int (*)(const char*, const char*))(kernel->stricmp);
	else
		cmp = strcmp;

	index = NULL;
	for (i = 0;  i < MAX_CLUSTER-1;  i++)
	{
		icp = cluster[i];
		if (icp && (icp->n_used > 0))
		{
			for (j = 0;  j < CLUSTER_SIZE;  j++)
				if ((icp->index[j].dir==dir) && (icp->index[j].search_val==sval))
					if (!(*cmp)(name, icp->index[j].name))
					{
						index = &icp->index[j];
						break;
					}
		}
	}
	if (!index)
	{
		icp = cluster[MAX_CLUSTER-1];
		while (icp)
		{
			if (icp && (icp->n_used > 0))
			{
				for (j = 0;  j < CLUSTER_SIZE;  j++)
					if ((icp->index[j].dir==dir) && (icp->index[j].search_val==sval))
						if (!(*cmp)(name, icp->index[j].name))
						{
							index = &icp->index[j];
							break;
						}
			}
			icp = icp->next;
		}
	}

	if (index)
	{
#ifdef USE_CACHE
		nfs_cache_remove(index);
#endif

		/* The remove code in MiNT releases the cookie of the deleted file
		 * after the delete operation, so there is at least one link on it.
		 */
		if (index->link > 1)
		{
			DEBUG(("remove_by_name: index for '%s' has %ld links",
			                                         name, (long)index->link));
			return EACCES;
		}
		free_slot(index);
	}

	return 0;
}
#endif
