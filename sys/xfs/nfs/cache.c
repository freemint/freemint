/*
 * Copyright 1993, 1994 by Ulrich Khn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File:  cache.c
 *        a small cache for lookup operations which occur very frequently
 *
 */

# include "cache.h"

# include "index.h"
# include "nfsutil.h"


typedef struct
{
	NFS_INDEX *dir;
	char *name;
	NFS_INDEX *index;
	long expiration;
} NFS_LOOKUP_CACHE;


static NFS_LOOKUP_CACHE nfs_cache[LOOKUP_CACHE_SIZE];



/* Delete the i-th entry in the lookup cache, so that it can be reused.
 */
static void
nfs_cache_del (int i)
{
	NFS_INDEX *ni = nfs_cache[i].index;
	
	if (ni != NULL)
	{
		ni->link -= 1;
		if (ni->link < 0)
		{
			DEBUG (("nfs_cache_del: internal inconsistency: ni->link < 0!"));
			return;
		}
		if (0 == ni->link)
			free_slot (ni);
	}
	
	nfs_cache[i].dir = NULL;
	nfs_cache[i].name = NULL;
	nfs_cache[i].index = NULL;
}


/* This function should be invoked periodically to flush old entries from
 * the lookup cache.
 */
void
nfs_cache_expire (void)
{
	long stamp = get_timestamp ();
	long i;
	
	for (i = 0;  i < LOOKUP_CACHE_SIZE;  i++)
		if (nfs_cache[i].expiration < stamp)
			nfs_cache_del(i);
}


/* Look for a given name in a given directory, but when doing name
 * comparisons watch out for the right process domain.
 * If something is found, return the nfs index of that, otherwise NULL.
 */
NFS_INDEX *
nfs_cache_lookup (NFS_INDEX *dir, const char *name, int dom)
{
	long i, eq;
	
	for (i = 0;  i < LOOKUP_CACHE_SIZE; i++)
	{
		if (dir == nfs_cache[i].dir)
		{
			if (0 == dom)
				eq = stricmp (name, nfs_cache[i].name);
			else
				eq = strcmp (name, nfs_cache[i].name);
			
			if (!eq)
			{
				/* BUG: should we set a new expiration time?? */
				return nfs_cache[i].index;
			}
		}
	}
	
	return NULL;
}


/* Add a given index to the lookup cache. The directory this file is in is
 * given in dir, so that we can search for it later.
 * Look through all the cache entries to find a free one; if there is no,
 * take the least recently used one.
 */
int
nfs_cache_add(NFS_INDEX *dir, NFS_INDEX *index)
{
	long i, least;
	long least_ac, next_expire, now;
	
	least = 0;
	now = get_timestamp ();
	
	next_expire = least_ac = now + NFS_CACHE_EXPIRE;
	for (i = 0;  i < LOOKUP_CACHE_SIZE;  i++)
	{
		/* look for an unused entry
		 */
		if (NULL == nfs_cache[i].dir)
		{
			least = i;
			break;
		}
		
		/* also look for the least recently added entry
		 */
		if (nfs_cache[i].expiration < least_ac)
		{
			least = i;
			least_ac = nfs_cache[i].expiration;
		}
		
		/* if the expiration time is over, free and reuse the entry
		 */
		if (nfs_cache[i].expiration < now)
		{
			nfs_cache_del (i);
			least = i;
		}
	}
	
	/* free the entry if necessary
	 */
	if (nfs_cache[least].dir)
		nfs_cache_del (least);
	
	/* BUG: if the cache entry was not in use, set a timeout for the expire
	 *      function
	 */
	
	/* this index is once more in use
	 */
	index->link++;
	
	/* set up cache entry
	 */
	nfs_cache[least].dir = dir;
	nfs_cache[least].index = index;
	nfs_cache[least].name = index->name;
	nfs_cache[least].expiration = next_expire;
	
	return 0;
}

int
nfs_cache_remove (NFS_INDEX *ni)
{
	long i, n;
	
	n = -1;
	for (i = 0;  i < LOOKUP_CACHE_SIZE; i++)
	{
		if (ni == nfs_cache[i].index)
		{
			n = i;				
			break;
		}
	}
	
	if (n != -1)
	{
		nfs_cache_del (n);
# if 0
		ni->link -= 1;
		nfs_cache[i].dir = NULL;
		nfs_cache[i].name = NULL;
		nfs_cache[i].index = NULL;
		
		return 1;
# endif
	}
	
	return 0;
}

int
nfs_cache_removebyname (NFS_INDEX *parent, const char *name)
{
	long i, n;
	
	n = -1;
	for (i = 0;  i < LOOKUP_CACHE_SIZE;  i++)
	{
		if (nfs_cache[i].dir == parent)
		{
			if (!strcmp (name, nfs_cache[i].name))
				n = i;
		}
	}
	
	if (n != -1)
		nfs_cache_del (n);
	else
		return 1;
	
	return 0;
}
