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
 * File : netfs.c
 *        networking filesystem driver
 */

# include "nfssys.h"
# include "nfsdev.h"

# include "cache.h"
# include "index.h"
# include "nfsutil.h"
# include "sock_ipc.h"
# include "version.h"


static long	get_handle	(NFS_INDEX *ni);
static long	do_create	(long nfs_opcode, fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long	do_remove	(long nfs_opcode, fcookie *dir, const char *name);


static long	_cdecl nfs_root		(int drv, fcookie *fc);
static long	_cdecl nfs_lookup	(fcookie *dir, const char *name, fcookie *fc);
static long	_cdecl nfs_creat	(fcookie *dir, const char *name, unsigned int mode, int attrib, fcookie *fc);
static DEVDRV *	_cdecl nfs_getdev	(fcookie *fc, long *devsp);
       long	_cdecl nfs_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl nfs_stat64	(fcookie *fc, STAT *ptr);
static long	_cdecl nfs_chattr	(fcookie *fc, int attrib);
static long	_cdecl nfs_chown	(fcookie *fc, int uid, int gid);
static long	_cdecl nfs_chmode	(fcookie *fc, unsigned int mode);
static long	_cdecl nfs_mkdir	(fcookie *dir, const char *name, unsigned int mode);
static long	_cdecl nfs_rmdir	(fcookie *dir, const char *name);
static long	_cdecl nfs_remove	(fcookie *dir, const char *name);
static long	_cdecl nfs_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl nfs_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
static long	_cdecl nfs_opendir	(DIR *dirh, int flags);
static long	_cdecl nfs_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl nfs_rewinddir	(DIR *dirh);
static long	_cdecl nfs_closedir	(DIR *dirh);
static long	_cdecl nfs_pathconf	(fcookie *dir, int which);
static long	_cdecl nfs_dfree	(fcookie *dir, long *buf);
static long	_cdecl nfs_writelabel	(fcookie *dir, const char *name);
static long	_cdecl nfs_readlabel	(fcookie *dir, char *name, int namelen);
static long	_cdecl nfs_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl nfs_readlink	(fcookie *dir, char *buf, int len);
static long	_cdecl nfs_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl nfs_fscntl	(fcookie *dir, const char *name, int cmd, long arg);
static long	_cdecl nfs_dskchng	(int drv, int mode);
static long	_cdecl nfs_release	(fcookie *fc);
static long	_cdecl nfs_dupcookie	(fcookie *dst, fcookie *src);


FILESYS nfs_filesys =
{
	NULL,
	
	/*
	 * FS_KNOPARSE		kernel shouldn't do parsing
	 * FS_CASESENSITIVE	file names are case sensitive
	 * FS_NOXBIT		if a file can be read, it can be executed
	 * FS_LONGPATH		file system understands "size" argument to "getname"
	 * FS_NO_C_CACHE	don't cache cookies for this filesystem
	 * FS_DO_SYNC		file system has a sync function
	 * FS_OWN_MEDIACHANGE	filesystem control self media change (dskchng)
	 * FS_REENTRANT_L1	fs is level 1 reentrant
	 * FS_REENTRANT_L2	fs is level 2 reentrant
	 * FS_EXT_1		extensions level 1 - mknod & unmount
	 * FS_EXT_2		extensions level 2 - additional place at the end
	 * FS_EXT_3		extensions level 3 - stat & native UTC timestamps
	 */
	FS_CASESENSITIVE	|
	FS_LONGPATH		|
	FS_NO_C_CACHE		|
	FS_OWN_MEDIACHANGE	|
/*	FS_REENTRANT_L1		| */
/*	FS_REENTRANT_L2		| */
	FS_EXT_2		|
	FS_EXT_3		,
	
	nfs_root,
	nfs_lookup, nfs_creat, nfs_getdev, nfs_getxattr,
	nfs_chattr, nfs_chown, nfs_chmode,
	nfs_mkdir, nfs_rmdir, nfs_remove, nfs_getname, nfs_rename,
	nfs_opendir, nfs_readdir, nfs_rewinddir, nfs_closedir,
	nfs_pathconf, nfs_dfree, nfs_writelabel, nfs_readlabel,
	nfs_symlink, nfs_readlink, nfs_hardlink, nfs_fscntl, nfs_dskchng,
	nfs_release, nfs_dupcookie,
	NULL, /* sync */
	
	/* FS_EXT_1 */
	NULL, NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	nfs_stat64,
	
	0, 0, 0, 0, 0,
	NULL, NULL
};



static long
get_handle (NFS_INDEX *ni)
{
	if (ni->flags & NO_HANDLE)    /* handle not initialised */
	{
		NFS_INDEX *newi;
		fcookie fc1, fc2;
		long r;
		
		TRACE (("get_handle: file '%s' without handle, looking up", (ni) ? ni->name : "root"));
		
		fc1.fs = &nfs_filesys;
		fc1.dev = nfs_dev;
		fc1.aux = 0;
		fc1.index = (long)ni->dir;
		
		r = nfs_lookup (&fc1, ni->name, &fc2);
		if (r != 0)
		{
			DEBUG(("get_handle: failed to get handle, -> ENOENT"));
			return ENOENT;
		}
		
		newi = (NFS_INDEX *) fc2.index;
		if (newi != ni)
		{
			ni->handle = newi->handle;
			ni->attr = newi->attr;
			ni->stamp = newi->stamp;
		}
		
		nfs_release (&fc2);
		ni->flags &= ~NO_HANDLE;
	}
	
	return 0;
}


static XATTR root_attr;
static fcookie root_cookie = { &nfs_filesys, 0, 0, 0 };

void
init_fs (void)
{
	init_mount_attr (&root_attr);
	init_index ();
	init_ipc (NFS_PROGRAM, NFS_VERSION);
	root_attr.blksize = sizeof (NFS_INDEX);
}

static long _cdecl
nfs_root (int drv, fcookie *fc)
{
	TRACE (("nfs_root"));
	
	if (drv != nfs_dev)
	{
		DEBUG (("nfs_root(%d) -> ENXIO", drv));
		return ENXIO;
	}
	
	root_cookie.dev = nfs_dev;
	
	fc->fs = &nfs_filesys;
	fc->dev = nfs_dev;
	fc->aux = 0;
	fc->index = (long) ROOT_INDEX;
	
	TRACE (("nfs_root(%d) -> OK", drv));
	return 0;
}

static long _cdecl
nfs_lookup (fcookie *dir, const char *name, fcookie *fc)
{
# ifdef TOSDOMAIN_LOWERCASE
	char lower_buf [256];
# endif
	char req_buf [LOOKUPBUFSIZE];
	NFS_INDEX *ni, *newi;
	long r;
	int dom;
	MESSAGE *mreq, *mrep, m;
	diropargs dirargs;
	diropres dirres;
	xdrs x;
	
	
	ni = (NFS_INDEX *) dir->index;
	
	/* get process domain */
	dom = p_domain (-1);
	
# ifdef TOSDOMAIN_LOWERCASE
	if (dom == 0)
	{
		/* We are in tos domain, so convert the file name to lower
		 * case
		 */
		strcpy (lower_buf, name);
		strlwr (lower_buf);
		name = lower_buf;
	}
# endif
	
	DEBUG (("nfs_lookup('%s' in dir '%s')", name, (ni) ? ni->name : "root"));
	
	if (!*name || !strcmp (name, "."))
	{
		nfs_dupcookie (fc, dir);
		TRACE (("nfs_lookup(%s in itself) -> ok", name));
		return 0;
	}
	
	if (!strcmp (name, ".."))
	{
		if (ROOT_INDEX == ni)
		{
			nfs_dupcookie (fc, dir);
			TRACE (("nfs_lookup(%s) -> EMOUNT", name));
			return EMOUNT;
		}
		else
		{
			newi = ni->dir;
			newi->link += 1;
			fc->fs = &nfs_filesys;
			fc->dev = nfs_dev;
			fc->aux = 0;
			fc->index = (long) newi;
			TRACE (("nfs_lookup('%s' in '%s' ) -> ok", name, (ni) ? ni->name : "root"));
			return 0;
		}
	}
	
	/* if we are in the root dir, we have to search in our own data for a
	 * dir with that name
	 */
	if (ROOT_INDEX == ni)
	{
		ni = mounted;
		while (ni)
		{
			if (ni->link == 0)
			{
				/* this one is not used, so skip it. */
				ni = ni->next;
				continue;
			}
			
			if (dom == 0)
			{
				if (!stricmp (name, ni->name))
					break;
			}
			
			if (!strcmp (name, ni->name))
				break;
			
			ni = ni->next;
		}
		
		if (ni)
		{
			ni->link += 1;
			ni->dir = (NFS_INDEX *) dir->index;
			fc->fs = &nfs_filesys;
			fc->dev = nfs_dev;
			fc->aux = 0;
			fc->index = (long) ni;
			
			TRACE (("nfs_lookup(%s) in root dir -> ok", name));
			return 0;
		}
		else
		{
			DEBUG(("nfs_lookup(%s) -> ENOENT", name));
			return ENOENT;
		}
	}
	
	/* here, we have to ask the nfs server to look up the name
	 */
	 
	ni = (NFS_INDEX *) dir->index;
	
# ifdef USE_CACHE
	/* first, consult the lookup cache, if we already have looked this one
	 * up, so that we dont have to ask the server again.
	 */
	newi = nfs_cache_lookup (ni, name, dom);
	if (newi)
	{
		newi->link += 1;
		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long) newi;
		
		DEBUG (("nfs_lookup('%s' in '%s') from cache", name, (ni)?ni->name:"root"));
		return 0;
	}		
# endif
	
	/* check if the directory itself has already got a handle from the server
	 * (see nfs_readdir)
	 */
	if (get_handle (ni) != 0)
 	{
		DEBUG (("nfs_lookup(%s): no handle for current dir, -> ENOTDIR", name));
		return ENOTDIR;
	}
	
	dirargs.dir = ni->handle;
	dirargs.name = name;
	
	mreq = alloc_message (&m, req_buf, LOOKUPBUFSIZE, xdr_size_diropargs (&dirargs));
	if (!mreq)
	{
		DEBUG (("nfs_lookup(%s): failed to alloc request msg, -> ENOENT", name));
		return ENOENT;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_diropargs (&x, &dirargs);
	
	r = rpc_request (&ni->opt->server, mreq, NFSPROC_LOOKUP, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_lookup(%s): couldn't contact server, -> ENOENT", name));
		return ENOENT;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_diropres (&x, &dirres))
	{
		DEBUG (("nfs_lookup(%s): couldnt decode results, -> ENOENT", name));
		free_message (mrep);
		return ENOENT;
	}
	
	free_message (mrep);
	
	if (dirres.status != NFS_OK)
	{
		DEBUG (("nfs_lookup(%s) rpc->%d -> ENOENT", name, dirres.status));
		return ENOENT;
	}
	
	newi = get_slot (ni, name, dom);
	if (!newi)
		return EMFILE;
	
	newi->dir = ni;
	newi->link += 1;
	newi->handle = dirres.diropres_u.diropok.file;
	fattr2xattr (&dirres.diropres_u.diropok.attributes, &newi->attr);
	
	newi->stamp = get_timestamp ();
	fc->fs = &nfs_filesys;
	fc->dev = nfs_dev;
	fc->aux = 0;
	fc->index = (long) newi;
	
# ifdef USE_CACHE
	nfs_cache_add (ni, newi);
# endif
	
	DEBUG (("nfs_lookup('%s' in '%s') -> OK", name, (ni)?ni->name:"root"));
	return 0;
}


static long
do_create (long nfs_opcode, fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
{
	char req_buf[CREATEBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	createargs createarg;
	diropres dirres;
	xdrs x;
	NFS_INDEX *newi, *ni = (NFS_INDEX *) dir->index;
	
	TRACE (("do_create(%s)", name));
	
	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("do_create: mount is read-only -> EACCES"));
		return EACCES;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("do_creat(%s): no handle for current dir, -> ENOTDIR", name));
		return ENOTDIR;
	}
	
	createarg.where.dir = ni->handle;
	createarg.where.name = name;
	createarg.attributes.mode = nfs_mode (mode, attrib);
	createarg.attributes.uid = p_getuid ();
	createarg.attributes.gid = p_getgid ();
	createarg.attributes.size = 0;
	createarg.attributes.atime.seconds = CURRENT_TIME;
	createarg.attributes.atime.useconds = 0;
	createarg.attributes.mtime.seconds = createarg.attributes.atime.seconds;
	createarg.attributes.mtime.useconds = 0;
	
	mreq = alloc_message (&m, req_buf, CREATEBUFSIZE, xdr_size_createargs(&createarg));
	if (!mreq)
	{
		DEBUG (("do_create(%s): failed to alloc request msg, -> EACCES", name));
		return EACCES;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_createargs (&x, &createarg);
	
	r = rpc_request (&ni->opt->server, mreq, nfs_opcode, &mrep);
	if (r != 0)
	{
		DEBUG (("do_create(%s): couldn't contact server, -> EACCES", name));
		return EACCES;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	
	if (!xdr_diropres (&x, &dirres))
	{
		DEBUG (("do_create(%s): couldnt decode results, -> EACCES", name));
		free_message (mrep);
		return EACCES;
	}
	
	free_message (mrep);
	
	if (dirres.status != NFS_OK)
	{
		DEBUG (("do_create(%s) -> EACCES", name));
		return EACCES;
	}
	
	newi = get_slot (ni, name, p_domain (-1));
	if (!newi)
	{
		DEBUG (("do_create: no slot found -> EACCES"));
		return EACCES;
	}
	
	newi->dir = (NFS_INDEX *) dir->index;
	newi->link += 1;
	newi->handle = dirres.diropres_u.diropok.file;
	fattr2xattr (&dirres.diropres_u.diropok.attributes, &newi->attr);
	newi->stamp = get_timestamp ();
	
	if (fc)
	{
		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long) newi;
	}
	
	TRACE (("do_create(%s) -> OK", name));
	return 0;
}

static long _cdecl
nfs_creat (fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
{
	long r;
	
	TRACE (("nfs_creat"));
	
	if (ROOT_INDEX == (NFS_INDEX *) dir->index)
	{
		/* only mount dcntl() is allowed in the root dir */
		DEBUG (("nfs_creat(%s): no creation in root dir -> EACCES", name));
		return EACCES;
	}
	
	r = do_create (NFSPROC_CREATE, dir, name, mode, attrib, fc);
	return r;
}

static long _cdecl
nfs_mkdir (fcookie *dir, const char *name, unsigned mode)
{
	long r;
	
	TRACE (("nfs_mkdir"));
	
	if (ROOT_INDEX == (NFS_INDEX *) dir->index)
	{
		/* only mount dcntl() is allowed in the root dir */
		DEBUG (("nfs_mkdir(%s): no creation in root dir -> EACCES", name));
		return EACCES;
	}
	
	r = do_create (NFSPROC_MKDIR, dir, name, mode, FA_DIR, NULL);
	return r;
}

static DEVDRV * _cdecl
nfs_getdev (fcookie *fc, long *devsp)
{
	TRACE (("nfs_getdev"));
	
	if (nfs_dev != fc->dev)
	{
		*devsp = EBADF;
		return NULL;
	}
	
	*devsp = 0;
	return &nfs_device;
}

long _cdecl
nfs_getxattr (fcookie *fc, XATTR *xattr)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	char req_buf[XATTRBUFSIZE];
	long stamp, r;
	MESSAGE *mreq, *mrep, m;
	attrstat stat_res;
	xdrs x;
	
	DEBUG (("nfs_getxattr(%s)", (ni) ? ni->name : "root"));
	
	if (ROOT_INDEX == ni)
	{
		/* attributes for the root dir */
		if (xattr)
		{
			*xattr = root_attr;
			xattr->index = (long) ROOT_INDEX;
			xattr->dev = fc->dev;
		}
		
		TRACE (("nfs_getxattr(root) -> mode 0%o, ok", root_attr.mode));
		return E_OK;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("nfs_getxattr(%s): failed to get handle, -> ENOENT", ni->name));
		return ENOENT;
	}
	
	/* look if we have the right attributes already cached, that means
	 * that the lifetime of the attributes in the index struct is not
	 * exceeded. If so, return the cached values, but only if the mount
	 * has not specified not to use the attribute cache.
	 */
	if (!(ni->opt->flags & OPT_NOAC))
	{
		stamp = get_timestamp ();
		if (after (ni->stamp + ni->opt->actimeo, stamp))
		{
			if (xattr)
			{
				*xattr = ni->attr;
				xattr->dev = fc->dev;
# if 0   /* BUG: which device is this file on???? */
				xattr->index = (long) ni;
# endif
				
				if (ni->opt->flags & OPT_RO)
				{
					xattr->mode &= ~(S_IWOTH|S_IWGRP|S_IWUSR);
					xattr->attr |= FA_RDONLY;
				}
				
				if ((xattr->mode & S_IFMT) == S_IFLNK)
					/* fix for buffer size when reading symlinks */
					++xattr->size;
			}
			
			DEBUG (("nfs_getxattr(%s): from cache -> mode 0%o, ok", ni->name, ni->attr.mode));
			return E_OK;
		}
	}
	
	mreq = alloc_message (&m, req_buf, XATTRBUFSIZE, xdr_size_nfsfh (&ni->handle));
	if (!mreq)
	{
		DEBUG (("nfs_getxattr(%s): failed to alloc msg, -> ENOENT", ni->name));
		return ENOENT;
	}
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_nfsfh (&x, &ni->handle);
	
	r = rpc_request (&ni->opt->server, mreq, NFSPROC_GETATTR, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_getxattr(%s): couldn't contact server, -> EACCES",ni->name));
		return EACCES;
	}
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_attrstat (&x, &stat_res))
	{
		DEBUG (("nfs_getxattr(%s): couldnt decode results, -> EACCES", ni->name));
		free_message (mrep);
		return EACCES;
	}
	free_message (mrep);
	if (stat_res.status != NFS_OK)
	{
		DEBUG (("nfs_getxattr(%s) rpc->%d, -> EACCES", ni->name, stat_res.status));
		return EACCES;
	}
	fattr2xattr (&stat_res.attrstat_u.attributes, &ni->attr);
	ni->stamp = get_timestamp ();
	
	if (xattr)
	{
		*xattr = ni->attr;
		xattr->dev = fc->dev;
# if 0 /* BUG: which device is this object on???? */
		xattr->index = (long) ni;
# endif
		
		if (ni->opt->flags & OPT_RO)
		{
			xattr->mode &= ~(S_IWOTH|S_IWGRP|S_IWUSR);
			xattr->attr |= FA_RDONLY;
		}
		
		if (ni->opt->flags & OPT_NOSUID)
		{
			/* mount with no set uid bit */
			xattr->mode &= ~S_ISUID;
		}
		
		if ((xattr->mode & S_IFMT) == S_IFLNK)
		{
			/* fix for buffer size when reading symlinks */
			++xattr->size;
		}
	}
	
	DEBUG (("nfs_getxattr(%s) -> mode 0%o, ok", ni->name, ni->attr.mode));
	return E_OK;
}

static long _cdecl
nfs_stat64 (fcookie *fc, STAT *stat)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	XATTR xattr;
	long r;
	
	DEBUG (("nfs_getxattr(%s)", (ni) ? ni->name : "root"));
	
	r = nfs_getxattr (fc, &xattr);
	if (!r)
	{
		stat->dev	= xattr.dev;
		stat->ino	= xattr.index;
		stat->mode	= xattr.mode;
		stat->nlink	= xattr.nlink;
		stat->uid	= xattr.uid;
		stat->gid	= xattr.gid;
		stat->rdev	= xattr.rdev;
		
		stat->atime.time = *((long *) &(xattr.atime));
		stat->mtime.time = *((long *) &(xattr.mtime));
		stat->ctime.time = *((long *) &(xattr.ctime));
		
		stat->size	= xattr.size;
		stat->blocks	= (xattr.nblocks * xattr.blksize) >> 9;
		stat->blksize	= xattr.blksize;
		
		/* stat->flags	= 0; */
		/* stat->gen	= 0; */
		
		bzero (stat->res, sizeof (stat->res));
	}
	
	return r;
}

long
do_sattr (fcookie *fc, sattr *ap)
{
	char req_buf[SATTRBUFSIZE];
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	long r;
	MESSAGE *mreq, *mrep, m;
	sattrargs s_arg;
	attrstat stat_res;
	xdrs x;
	
	TRACE (("do_sattr(%s)", ni->name));
	
	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("do_sattr: mount is read-only -> EACCES"));
		return EACCES;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("do_sattr(%s): failed to get handle, -> ENOENT", ni->name));
		return ENOENT;
	}
	
	s_arg.file = ni->handle;
	s_arg.attributes = *ap;
	
	mreq = alloc_message (&m, req_buf, SATTRBUFSIZE, xdr_size_sattrargs (&s_arg));
	if (!mreq)
	{
		DEBUG (("do_sattr(%s): failed to allocate request message", ni->name));
		return ENOENT;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_sattrargs (&x, &s_arg);
	
	r = rpc_request (&ni->opt->server, mreq, NFSPROC_SETATTR, &mrep);
	if (r != 0)
	{
		DEBUG (("do_sattr(%s): couldn't contact server, -> EACCES", ni->name));
		return EACCES;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_attrstat (&x, &stat_res))
	{
		free_message (mrep);
		
		DEBUG (("do_sattr(%s): couldnt decode results, -> EACCES", ni->name));
		return EACCES;
	}
	
	free_message (mrep);
	
	if (stat_res.status != NFS_OK)
	{
		DEBUG (("do_sattr(%s) -> EACCES", ni->name));
		return EACCES;
	}
	fattr2xattr (&stat_res.attrstat_u.attributes, &ni->attr);
	ni->stamp = get_timestamp ();
	
	TRACE (("do_sattr(%s) -> OK", ni->name));
	return E_OK;
}

static long _cdecl
nfs_chattr (fcookie *fc, int attrib)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr attr;
	long r;
	int wperm;
	
	TRACE (("nfs_chattr(%d)", attrib));
	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_chattr on root dir"));
		root_attr.attr = attrib;
		if (attrib & FA_RDONLY)
			root_attr.mode &= ~(S_IWOTH|S_IWGRP|S_IWUSR);
		
		return 0;
	}
	
	/* get current attributes */
	r = nfs_getxattr (fc, NULL);
	if (r != 0)
		return r;
	
	wperm = ((attrib & FA_RDONLY) &&
			(ni->attr.mode & (S_IWOTH|S_IWGRP|S_IWUSR)))
		|| (!(attrib & FA_RDONLY) &&
			!(ni->attr.mode & (S_IWOTH|S_IWGRP|S_IWUSR)));
	
	if (wperm)
	{
		/* set write permissions correctly */
		attr.mode = ni->attr.mode;
		attr.mode |= S_IWOTH|S_IWGRP|S_IWUSR;
		if (attrib & FA_RDONLY)
			attr.mode &= ~(S_IWOTH|S_IWGRP|S_IWUSR);
		
		attr.uid = (ulong) -1L;
		attr.gid = (ulong) -1L;
		attr.size = (ulong) -1L;
		attr.atime.seconds = (ulong) -1L;
		attr.atime.useconds = (ulong) -1L;
		attr.mtime.seconds = (ulong) -1L;
		attr.mtime.useconds = (ulong) -1L;
		
		return do_sattr (fc, &attr);
	}
	
	/* BUG: we should do some time calculations on which the archive
	 *      attribute setting could be based. Also, the system and hidden
	 *      attribute should be maintained somehow. ANY IDEAS? we could
	 *      also try to emulate the system flag by chowning to root, but
	 *      is this really a good idea?
	 */
	DEBUG (("nfs_chattr: other than readonly attribute not implemented"));
	return EACCES;
}

static long _cdecl
nfs_chown (fcookie *fc, int uid, int gid)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr attr;
	
	TRACE (("nfs_chown"));
	
	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_chown on root dir"));
		
		if (uid != -1) root_attr.uid = uid;
		if (gid != -1) root_attr.gid = gid;
		
		return 0;
	}
	
	attr.uid = uid;
	attr.gid = gid;
	attr.mode = (ulong) -1L;
	attr.size = (ulong) -1L;
	attr.atime.seconds = (ulong) -1L;
	attr.atime.useconds = (ulong) -1L;
	attr.mtime.seconds = (ulong) -1L;
	attr.mtime.useconds = (ulong) -1L;
	
	return do_sattr (fc, &attr);
}

static long _cdecl
nfs_chmode (fcookie *fc, unsigned mode)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr attr;
	
	TRACE (("nfs_chmode"));
	
	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_chmode on root dir"));
		
		/* make sure to preserve the file type
		 */
		root_attr.mode = (root_attr.mode & S_IFMT) | (mode & ~S_IFMT);
		return 0;
	}
	
	attr.uid = (ulong) -1;
	attr.gid = (ulong) -1;
	attr.mode = mode;
	attr.size = (ulong) -1L;
	attr.atime.seconds = (ulong) -1L;
	attr.atime.useconds = (ulong) -1L;
	attr.mtime.seconds = (ulong) -1L;
	attr.mtime.useconds = (ulong) -1L;
	
	return do_sattr (fc, &attr);
}

static long
do_remove (long nfs_opcode, fcookie *dir, const char *name)
{
	char req_buf[REMBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	diropargs dirargs;
	nfsstat stat_res;
	xdrs x;
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;
	
	DEBUG (("do_remove(%s)", name));
	
	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("do_remove: mount is read-only ->EACCES"));
		return EACCES;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("do_remove(%s): failed to get handle, -> ENOTDIR", name));
		return ENOTDIR;
	}
	dirargs.dir = ni->handle;
	dirargs.name = name;
	mreq = alloc_message (&m, req_buf, REMBUFSIZE, xdr_size_diropargs(&dirargs));
	if (!mreq)
	{
		DEBUG (("do_remove(%s): failed to allocate buffer -> EACCES", name));
		return EACCES;
	}
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_diropargs (&x, &dirargs);
	r = rpc_request (&ni->opt->server, mreq, nfs_opcode, &mrep);
	if (r != 0)
	{
		DEBUG (("do_remove(%s): couldn't contact server, -> EACCES", name));
		return EACCES;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_nfsstat (&x, &stat_res))
	{
		DEBUG (("do_remove(%s): couldnt decode results, -> EACCES", name));
		free_message (mrep);
		return EACCES;
	}
	free_message (mrep);
	if (stat_res != NFS_OK)
	{
		DEBUG (("do_remove(%s, %ld) -> EACCES", name, nfs_opcode));
		return EACCES;
	}
	
# ifdef USE_CACHE
	nfs_cache_removebyname (ni, name);
# endif
	
	DEBUG (("do_remove(%s, %ld) -> OK", name, nfs_opcode));
	return 0;
}

static long _cdecl
nfs_rmdir (fcookie *dir, const char *name)
{
	NFS_INDEX *ni = (NFS_INDEX*)dir->index;

	TRACE(("nfs_rmdir"));
	if (ROOT_INDEX == ni)
	{
		DEBUG(("nfs_rmdir(%s): no remove from root dir, -> EACCES", name));
		return EACCES;
	}

	return do_remove(NFSPROC_RMDIR, dir, name);
}

static long _cdecl
nfs_remove (fcookie *dir, const char *name)
{
	NFS_INDEX *ni = (NFS_INDEX*)dir->index;

	TRACE(("nfs_remove"));
	if (ROOT_INDEX == ni)
	{
		DEBUG(("nfs_remove(%s): no remove from root dir, -> EACCES", name));
		return EACCES;
	}

	return do_remove(NFSPROC_REMOVE, dir, name);
}

static long _cdecl
nfs_getname (fcookie *relto, fcookie *dir, char *pathname, int size)
{
	NFS_INDEX *ni, *oni, *reli;
	int len, copy_name;
	
	if (size < 0)
		return EBADARG;
	if (size == 0)
		return 0;
	
	/* make a linked list of nfs_index using the aux field from the
	 * top directory to the searched dir
	 */
	oni = (NFS_INDEX *) dir->index;
	reli = (NFS_INDEX *) relto->index;
	copy_name = 0;
	
	if (!oni)
		return ENOTDIR;
	
	TRACE (("nfs_getname: relto = '%s', dir = '%s'",
	           (reli==ROOT_INDEX)?"root":reli->name, oni->name));
	
	while (oni != (NFS_INDEX*)relto->index)
	{
		ni = oni->dir;
		
		/* stop if root dir reached
		 */
		if (ni == ROOT_INDEX)
		{
			if ((NFS_INDEX *) relto->index != ROOT_INDEX)
			{
				return ENOTDIR;
			}
			else
			{
				copy_name = 1;
				break;
			}
		}
		
		ni->aux = oni;
		oni= ni;
	}
	
	/* now fill pathname with up to size characters by going down the
	 * directory structure build up above
	 */
	size -= 1;   /* count off the trailing 0 */
	ni = oni;
	*pathname = '\0';
	if (copy_name)
	{
		if (size < (len = 1))
			return EBADARG;
		strcat(pathname, "\\");
		size -= len;
		if (size < (len = strlen(ni->name)))
			return EBADARG;
		strcat(pathname, ni->name);
		size -= len;
	}
	while (ni != (NFS_INDEX*)dir->index)
	{
		ni = ni->aux;
		if (!ni)
			return ENOTDIR;
		if (size < (len = 1))
			return EBADARG;
		strcat(pathname, "\\");
		size -= len;
		if (size < (len = strlen(ni->name)))
			return EBADARG;
		strcat(pathname, ni->name);
		size -= len;
	}
	TRACE(("nfs_getname -> '%s'", pathname));
	return 0;
}

static long _cdecl
nfs_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	char req_buf[RENBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	renameargs renarg;
	nfsstat stat_res;
	xdrs x;
	NFS_INDEX *newi = (NFS_INDEX *) newdir->index;
	NFS_INDEX *oldi = (NFS_INDEX *) olddir->index;

	TRACE (("nfs_rename('%s' -> '%s')", oldname, newname));
	if ((ROOT_INDEX == oldi) || (ROOT_INDEX == newi))
	{
		DEBUG (("nfs_rename(%s): no rename in the root dir, -> EACCES", oldname));
		return EACCES;
	}

	if ((oldi->opt->flags & OPT_RO) || (newi->flags & OPT_RO))
	{
		DEBUG (("nfs_rename: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (get_handle (newi) != 0)
	{
		DEBUG (("nfs_rename(%s): no handle for new dir, -> ENOTDIR", oldname));
		return ENOTDIR;
	}
	
	if (get_handle (oldi) != 0)
	{
		DEBUG (("nfs_rename(%s): no handle for old dir, -> ENOTDIR", oldname));
		return ENOTDIR;
	}

	renarg.from.dir = oldi->handle;
	renarg.from.name = oldname;
	renarg.to.dir = newi->handle;
	renarg.to.name = newname;
	
	mreq = alloc_message (&m, req_buf, RENBUFSIZE, xdr_size_renameargs (&renarg));
	if (!mreq)
	{
		DEBUG (("nfs_rename(%s): failed to allocate buffer -> EACCES", oldname));
		return EACCES;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_renameargs (&x, &renarg);
	
	r = rpc_request (&oldi->opt->server, mreq, NFSPROC_RENAME, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_rename(%s): couldn't contact server, -> EACCES", oldname));
		return EACCES;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	
	if (!xdr_nfsstat (&x, &stat_res))
	{
		DEBUG (("nfs_rename(%s): couldnt decode results, -> EACCES", oldname));
		free_message (mrep);
		return EACCES;
	}
	
	free_message (mrep);
	
	if (stat_res != NFS_OK)
	{
		DEBUG (("nfs_rename(%s) -> EACCES", oldname));
		return EACCES;
	}
	
	nfs_cache_removebyname (oldi, oldname);
	
	TRACE (("nfs_rename('%s' -> '%s') -> OK", oldname, newname));
	return 0;
}




/* Add this to the length of the buffer holding the decoded entries of
 * a directory. It is necessary as the strings in it contain an terminating
 * zero, whereas the XDRed representations don't. This value is too big,
 * so we are on the safe side.
 */
# define ADD_BUF_LEN	(MAX_READDIR_LEN / 16)


/* this is placed in the fsstuff field of a dir handle */
typedef struct
{
	char *buffer;   /* current entry buffer */
	entry *curr_entry;  /* this is the entry who is returned next */
	nfscookie lastcookie;   /* this is for further requests to the server */
	short eof;      /* if set, this buffer is the last in the dir */
} NETFS_STUFF;


static long _cdecl
nfs_opendir (DIR *dirh, int flags)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) &dirh->fsstuff;
	NFS_INDEX *ni = (NFS_INDEX *) dirh->fc.index;
	
	if (ROOT_INDEX != ni)
	{
		if (get_handle(ni) != 0)
		{
			DEBUG (("nfs_opendir(%s): no handle for dir, -> ENOTDIR", ni->name));
			return ENOTDIR;
		}
		
		stuff->buffer = kmalloc (MAX_READDIR_LEN + ADD_BUF_LEN);
		if (!stuff->buffer)
		{
			DEBUG (("nfs_opendir: out of memory -> ENOMEM"));
			return ENOMEM;
		}
	}
	else
		stuff->buffer = NULL;
	
	stuff->curr_entry = NULL;
	*(long *) &stuff->lastcookie[0] = 0L;
	stuff->eof = 0;
	dirh->index = 0;
	
	TRACE (("nfs_opendir(%s) -> ok", (ni) ? ni->name : "root"));
	return 0;
}

static long _cdecl
nfs_rewinddir (DIR *dirh)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) &dirh->fsstuff;
	
	if (ROOT_INDEX != (NFS_INDEX *) dirh->fc.index)
	{
		stuff->curr_entry = NULL;
		*(long *) &stuff->lastcookie[0] = 0L;
		stuff->eof = 0;
	}
	
	dirh->index = 0;
	
	TRACE (("nfs_rewinddir -> ok"));
	return 0;
}

static long _cdecl
nfs_closedir (DIR *dirh)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) &dirh->fsstuff;
	
	if (ROOT_INDEX != (NFS_INDEX *) dirh->fc.index)
	{
		if (stuff->buffer)
			kfree (stuff->buffer);
	}
	
	TRACE (("nfs_closedir -> ok"));
	return 0;
}

# define XDR_SIZE_READDIRRES	(3 * sizeof (long))
# define MAX_XDR_BUF		(MAX_READDIR_LEN + XDR_SIZE_READDIRRES)

static long _cdecl
nfs_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	char req_buf[READDIRBUFSIZE];
	int giveindex = dirh->flags == 0;
	int i, dom;
	entry *entp;
	NFS_INDEX *ni = (NFS_INDEX *) dirh->fc.index;
	NFS_INDEX *newi;
	MESSAGE *mreq, *mrep, m;
	long r;
	readdirargs read_arg;
	readdirres read_res;
	xdrs x;
	NETFS_STUFF *stuff = (NETFS_STUFF *) dirh->fsstuff;
	
	/* we know that ni has a handle, as we did get one in
	 * or before nfs_opendir
	 */
	dom = p_domain (-1);
	if (ROOT_INDEX == ni)
	{
		/* read the root dir of the file sys */
		TRACE(("nfs_readdir(root)"));
		if (giveindex)
		{
			namelen -= sizeof(long);
			if (namelen <= 0)
				return EBADARG;
			*((long *)name) = dirh->index;
			name += sizeof(long);
		}

		/* Skip the given amount of used indices. Especially skip unused 
		 * indices without counting them.
		 */
		ni = mounted;
		while (ni && (ni->link == 0))
			ni = ni->next;
		for (i = dirh->index++;  i > 0;  i--)
		{
			if (!ni)
				break;
			ni = ni->next;
			while (ni && (ni->link == 0))
				ni = ni->next;
			if (!ni)
				break;
		}

		/* If there are indices left, find the next used one and return it's
		 * name.
		 */
		while (ni && (ni->link == 0))
			ni = ni->next;

		if (!ni)
		{
			DEBUG(("nfs_readdir(root) -> no more files"));
			return ENMFILES;
		}
		strncpy(name, ni->name, namelen-1);
		name[namelen-1] = '\0';
		if (0 == dom)   /* convert to upper case for TOS domain */
			strupr (name);
		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long)ni;
		if (strlen(ni->name) >= namelen)
			return EBADARG;
		ni->link += 1;
		TRACE(("nfs_readdir -> '%s'", name));
		return 0;
	}

restart:
	TRACE (("trying to get entry from buffer"));
	if (stuff->curr_entry)
	{
		long res = 0;

		entp = stuff->curr_entry;
		if (giveindex)
		{
			namelen -= sizeof(long);
			if (namelen <= 0)
				return EBADARG;
			*((long *)name) = entp->fileid;
			name += sizeof(long);
		}
		strncpy(name, entp->name, namelen-1);
		name[namelen-1] = '\0';
		if (0 == dom)    /* convert to upper case for TOS domain */
			strupr (name);
		if (strlen(entp->name) >= namelen)
		{
			DEBUG(("nfs_readdir(%s): name buffer (%ld) too short",
			                                          ni->name, namelen));
			res = EBADARG;
			goto prep_next_entry;
		}

		/* check for entries '.' and '..' which have already a local slot */
		if (!strcmp(entp->name, "."))
		{
			newi = ni;  /* '.' does always mean the read directory */
		}
		else if (!strcmp(entp->name, ".."))
		{
			newi = ni->dir;   /* '..' means the parent of the read directory */
		}
		else
		{
			TRACE(("nfs_readdir: getting new slot for '%s'", entp->name));
			newi = get_slot(ni, entp->name, (dirh->flags & TOS_SEARCH) ? 0 : 1);
			if (!newi)
			{
				DEBUG(("nfs_readdir(%s): no index for entry, -> EMFILE", ni->name));
				res = EMFILE;
				goto prep_next_entry;
			}
			newi->flags |= NO_HANDLE;
			newi->stamp = get_timestamp()-ni->opt->actimeo-1;  /* no attr yet */
		}
		if (newi)
		{
			newi->link += 1;
		}
#ifdef USR_CACHE
		nfs_cache_add(ni, newi);
#endif

		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long)newi;

prep_next_entry:
		for (i = 0;  i < COOKIESIZE;  i++)
			stuff->lastcookie[i] = entp->cookie[i];
		stuff->curr_entry = entp->nextentry;
		DEBUG(("nfs_readdir(%s) -> %s", ni->name, name));
		return res;
	}
	if (stuff->eof)
	{
		TRACE(("nfs_readdir(%s): end of dir reached, -> ENMFILES", ni->name));
		return ENMFILES;
	}

	/* ask the server for another chunk of directory entries */
	TRACE(("nfs_readdir: requesting new chunk"));
	read_arg.dir = ni->handle;
	for (i = 0;  i < COOKIESIZE;  i++)
		read_arg.cookie[i] = stuff->lastcookie[i];
	read_arg.count = MAX_READDIR_LEN;    /* at most this much data per chunk */
	mreq = alloc_message(&m, req_buf, READDIRBUFSIZE,
	                               xdr_size_readdirargs(&read_arg));
	if (!mreq)
	{
		DEBUG(("nfs_readdir(%s): failed to alloc msg, -> ENMFILES", ni->name));
		return ENMFILES;
	}
	xdr_init(&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_readdirargs(&x, &read_arg);
	TRACE(("nfs_readdir: sending request"));

	r = rpc_request(&ni->opt->server, mreq, NFSPROC_READDIR, &mrep);
	if (r != 0)
	{
		DEBUG(("nfs_readdir(%s): couldnt contact server, -> ENMFILES", ni->name));
		return ENMFILES;
	}

	TRACE(("nfs_readdir: got answer"));
	bzero(stuff->buffer, MAX_READDIR_LEN);
	entp = read_res.readdirres_u.readdirok.entries = (entry*)stuff->buffer;

	/* make sure not to write over the end of the entry buffer, so decode
	 * only the length bounded by MAX_XDR_BUF.
	 */
	xdr_init(&x, mrep->data, MIN (MAX_XDR_BUF, mrep->data_len), XDR_DECODE, NULL);
	if (!xdr_readdirres(&x, &read_res))
	{
		DEBUG(("nfs_readdir(%s): could not decode results, -> ENMFILES", ni->name));
		free_message(mrep);
		return ENMFILES;
	}
	free_message(mrep);
	if (NFS_OK == read_res.status)
	{
		stuff->eof = read_res.readdirres_u.readdirok.eof;
		stuff->curr_entry = read_res.readdirres_u.readdirok.entries;
		goto restart;
	}
	TRACE(("nfs_readdir(%s) -> no more files", ni->name));
	return ENMFILES;
}





static long _cdecl
nfs_pathconf (fcookie *dir, int which)
{
	TRACE (("nfs_pathconf(%d)", which));
	
	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return 1;
		case DP_PATHMAX:	return MAXPATHLEN;
		case DP_NAMEMAX:	return MAXNAMLEN;
		case DP_ATOMIC:		return 512;
		case DP_TRUNC:		return DP_NOTRUNC;
		case DP_CASE:		return DP_CASESENS;
		case DP_MODEATTR:	return (DP_ATTRBITS | DP_MODEBITS
						| DP_FT_DIR
						| DP_FT_CHR
						| DP_FT_BLK
						| DP_FT_REG
						| DP_FT_LNK
						| DP_FT_SOCK
						| DP_FT_FIFO
					/*	| DP_FT_MEM	*/
					);
		case DP_XATTRFIELDS:	return (DP_INDEX
						| DP_DEV
					/*	| DP_RDEV	*/
						| DP_NLINK
						| DP_UID
						| DP_GID
						| DP_BLKSIZE
						| DP_SIZE
						| DP_NBLOCKS
						| DP_ATIME
						| DP_CTIME
						| DP_MTIME
					);
		case DP_VOLNAMEMAX:	return MAX_LABEL;
	}
	
	return ENOSYS;
}

static long _cdecl
nfs_dfree (fcookie *dir, long *buf)
{
	char req_buf[DFREEBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	xdrs x;
	statfsres stat_res;
	NFS_INDEX *ni = (NFS_INDEX*)dir->index;
	
	TRACE (("nfs_dfree"));
	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_dfree(root)"));
		
		/* these are really silly values; who knows better ones? */
		buf[0] = 0;   /* number of free clusters */
		buf[1] = 0;   /* total number of clusters */
		buf[2] = 0;   /* bytes per sector */
		buf[3] = 0;   /* sectors per cluster */
		
		return E_OK;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("nfs_dfree: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}
	
	mreq = alloc_message(&m, req_buf, DFREEBUFSIZE, xdr_size_nfsfh(&ni->handle));
	if (!mreq)
	{
		DEBUG(("nfs_dfree: failed to allocate buffer, -> ENOTDIR"));
		return ENOTDIR;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_nfsfh (&x, &ni->handle);
	r = rpc_request (&ni->opt->server, mreq, NFSPROC_STATFS, &mrep);
	if (r)
	{
		DEBUG (("nfs_dfree: couldn't contact server, -> ENOTDIR"));
		return ENOTDIR;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_statfsres (&x, &stat_res))
	{
		free_message (mrep);
		
		DEBUG (("nfs_dfree: couldnt decode results, -> ENOTDIR"));
		return ENOTDIR;
	}
	
	free_message (mrep);
	
	if (stat_res.status != NFS_OK)
	{
		DEBUG (("nfs_dfree -> ENOTDIR"));
		return ENOTDIR;
	}
	
	buf[0] = stat_res.statfsres_u.info.bavail;
	buf[1] = stat_res.statfsres_u.info.blocks;
	buf[2] = stat_res.statfsres_u.info.bsize;
	buf[3] = 1;
	
	return E_OK;
}

static char nfs_label [MAX_LABEL+1] = "Network";

static long _cdecl
nfs_writelabel (fcookie *dir, const char *name)
{
	TRACE (("nfs_writelabel"));
	
	if (ROOT_INDEX == (NFS_INDEX *) dir->index)
	{
		if (strlen (name) > MAX_LABEL)
			return EBADARG;
		
		strncpy (nfs_label, name, MAX_LABEL);
		nfs_label[MAX_LABEL + 1] = '\0';
		
		return E_OK;
	}
	
	return EACCES;
}

static long _cdecl
nfs_readlabel (fcookie *dir, char *name, int namelen)
{
	TRACE (("nfs_readlabel"));
	
	if (ROOT_INDEX == (NFS_INDEX *) dir->index)
	{
		if (namelen <= (strlen (nfs_label) + 1))
			return EBADARG;
		
		strncpy (name, nfs_label, namelen - 1);
		name[namelen - 1] = '\0';
		
		return E_OK;
	}
	
	return EACCES;
}

static long _cdecl
nfs_symlink (fcookie *dir, const char *name, const char *to)
{
	char req_buf[SYMLNBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	symlinkargs symarg;
	nfsstat stat_res;
	xdrs x;
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;
	
	TRACE (("nfs_symlink(%s -> %s)", name, to));
	
	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_symlink not allowed in root dir"));
		return EACCES;
	}
	
	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("nfs_symlink: mount is read-only -> EACCES"));
		return EACCES;
	}
	
	if (get_handle (ni) != 0)
	{
		DEBUG (("nfs_symlink: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}
	
	symarg.from.dir = ni->handle;
	symarg.from.name = name;
	symarg.to = to;
	symarg.attributes.mode = 0120777;
	symarg.attributes.uid = p_getuid ();
	symarg.attributes.gid = p_getgid ();
	symarg.attributes.size = strlen (to) + 1;
	symarg.attributes.atime.seconds = CURRENT_TIME;
	symarg.attributes.atime.useconds = 0;
	symarg.attributes.mtime.seconds = symarg.attributes.atime.seconds;
	symarg.attributes.mtime.useconds = 0;
	
	mreq = alloc_message (&m, req_buf, SYMLNBUFSIZE, xdr_size_symlinkargs (&symarg));
	if (!mreq)
	{
		DEBUG (("nfs_symlink: failed to allocate buffer, -> EACCES"));
		return EACCES;
	}
	
	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_symlinkargs (&x, &symarg);
	
	r = rpc_request (&ni->opt->server, mreq, NFSPROC_SYMLINK, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_symlink: couldn't contact server, -> EACCES"));
		return EACCES;
	}
	
	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	
	if (!xdr_nfsstat (&x, &stat_res))
	{
		DEBUG (("nfs_symlink: couldnt decode results, -> EACCES"));
		free_message (mrep);
		return EACCES;
	}
	
	free_message (mrep);
	
	if (stat_res != NFS_OK)
	{
		DEBUG (("nfs_symlink -> EACCES"));
		return EACCES;
	}
	
	TRACE (("nfs_symlink -> OK"));
	return 0;
}

static long _cdecl
nfs_readlink (fcookie *dir, char *buf, int len)
{
	char req_buf[READLNBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	readlinkres link_res;
	char databuf[MAXPATHLEN+1];
	xdrs x;
	NFS_INDEX *ni = (NFS_INDEX*)dir->index;

	TRACE(("nfs_readlink"));
	if ((ROOT_INDEX == ni) || (ni->flags & IS_MOUNT_DIR))
	{
		DEBUG(("nfs_readlink: no links in root dir"));
		return ENOENT;
	}
	if (get_handle(ni) != 0)
	{
		DEBUG(("nfs_readlink: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}
	mreq = alloc_message(&m, req_buf, READLNBUFSIZE,
	                               xdr_size_nfsfh(&ni->handle));
	if (!mreq)
	{
		DEBUG(("nfs_readlink: failed to allocate buffer, -> ENOENT"));
		return ENOENT;
	}
	xdr_init(&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_nfsfh(&x, &ni->handle);
	r = rpc_request(&ni->opt->server, mreq, NFSPROC_READLINK, &mrep);
	if (r != 0)
	{
		DEBUG(("nfs_readlink: couldn't contact server, -> ENOENT"));
		return ENOENT;
	}
	xdr_init(&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	link_res.readlinkres_u.data = &databuf[0];
	if (!xdr_readlinkres(&x, &link_res))
	{
		DEBUG(("nfs_readlink: couldnt decode results, -> ENOENT"));
		free_message(mrep);
		return ENOENT;
	}
	free_message(mrep);
	if (link_res.status != NFS_OK)
	{
		DEBUG(("nfs_readlink -> ENOENT"));
		return ENOENT;
	}
	{
		short i = len;
		char *p = buf, *cp = databuf;
		while (--i >= 0 && (*p++ = (*cp != '/' ? *cp : '\\')))
			++cp;
		if (i < 0)
		{
			DEBUG(("nfs_readlink: result too long, -> EBADARG"));
			return EBADARG;
		}
		DEBUG(("nfs_readlink -> `%s'", buf));
	}
	TRACE(("nfs_symlink -> OK"));
	return 0;
}

static long _cdecl
nfs_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	char req_buf[HARDLNBUFSIZE];
	long r;
	MESSAGE *mreq, *mrep, m;
	linkargs linkarg;
	nfsstat stat_res;
	xdrs x;
	fcookie fc;
	NFS_INDEX *fromi = (NFS_INDEX*)fromdir->index;
	NFS_INDEX *toi = (NFS_INDEX*)todir->index;

	TRACE(("nfs_hardlink(%s -> %s)", fromname, toname));
	if (ROOT_INDEX == toi)
	{
		DEBUG(("nfs_hardlink not allowed in root dir"));
		return EACCES;
	}

	if (toi->opt->flags & OPT_RO)
	{
		DEBUG(("nfs_hardlink: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (nfs_lookup(fromdir, fromname, &fc) != 0)
	{
		DEBUG(("nfs_hardlink: file not found, -> ENOENT"));
		return ENOENT;
	}

	if (get_handle(toi) != 0)
	{
		DEBUG(("nfs_hardlink: failed to get handle for dest dir, -> ENOTDIR"));
		return ENOTDIR;
	}

	fromi = (NFS_INDEX*)fc.index;
	nfs_release(&fc);
	linkarg.to.dir = toi->handle;
	linkarg.to.name = toname;
	linkarg.from = fromi->handle;

	mreq = alloc_message(&m, req_buf, HARDLNBUFSIZE,
	                              xdr_size_linkargs(&linkarg));
	if (!mreq)
	{
		DEBUG(("nfs_hardlink: failed to allocate buffer, -> EACCES"));
		return EACCES;
	}
	xdr_init(&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	xdr_linkargs(&x, &linkarg);

	r = rpc_request(&fromi->opt->server, mreq, NFSPROC_LINK, &mrep);
	if (r != 0)
	{
		DEBUG(("nfs_hardlink: couldn't contact server, -> EACCES"));
		return EACCES;
	}

	xdr_init(&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_nfsstat(&x, &stat_res))
	{
		DEBUG(("nfs_hardlink: couldnt decode results, -> EACCES"));
		free_message(mrep);
		return EACCES;
	}
	free_message(mrep);

	if (stat_res != NFS_OK)
	{
		DEBUG(("nfs_hardlink -> EACCES"));
		return EACCES;
	}

	TRACE(("nfs_hardlink -> OK"));
	return 0;
}

static long _cdecl
nfs_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	TRACE (("nfs_fscntl"));
	
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "nfs-xfs");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info;
			
			info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "nfs-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
# ifndef FS_NFS2
# define _MAJOR_NFS	(12L << 16)
# define FS_NFS2	(_MAJOR_NFS  | 0)	/* nfs 0.55 */
# endif
				info->type = FS_NFS2;
				strcpy (info->type_asc, "network filesystem");
			}
			
			return E_OK;
		}
# if 0
		case FS_USAGE:
		{
			struct fs_usage *usage;
			
			usage = (struct fs_usage *) arg;
			if (usage)
			{
				usage->blocksize = BLOCK_SIZE;
				usage->blocks = (FreeMemory + (memory + BLOCK_SIZE - 1)) >> BLOCK_SHIFT;
				usage->free_blocks = usage->blocks - ((memory + BLOCK_SIZE - 1) >> BLOCK_SHIFT);
				usage->inodes = FS_UNLIMITED;
				usage->free_inodes = FS_UNLIMITED;
			}
			
			return E_OK;
		}
# endif
		case NFS_MOUNT:
		{
			NFS_MOUNT_INFO *info = (NFS_MOUNT_INFO*)arg;
			NFS_INDEX *ni;
			
			if (!arg)
				return EACCES;
			
			if (ROOT_INDEX != (NFS_INDEX *) dir->index)
			{
				DEBUG (("nfs_fscntl: mount only allowed in root dir"));
				return EACCES;
			}
			
			ni = get_mount_slot (name, info);
			if (ni->link > 0)
			{
				/* this file was mounted before */
				
				DEBUG (("nfs_fscntl: no remount allowed, -> EACCES"));
				return EACCES;
			}
			
			ni->link = 1;
			ni->handle = info->handle;
			
			DEBUG (("nfs_fscntl: mounting dir '%s'", ni->name));
			return 0;
		}
		case NFS_UNMOUNT:
		{
			fcookie fc;
			NFS_INDEX *ni;
			long r;
			
			r = nfs_lookup (&root_cookie, name, &fc);
			if (r)
			{
				DEBUG (("nfs_fscntl: unmount on not mounted directory, -> ENOENT"));
				return ENOENT;
			}
			
			ni = (NFS_INDEX *) fc.index;
			nfs_release (&fc);
			
			if (!(ni->flags & IS_MOUNT_DIR))
			{
				DEBUG (("nfs_fscntl: unmount failed, not a mounted directory"));
				return EACCES;
			}
			
			DEBUG (("nfs_fscntl: unmounting '%s'", ni->name));
# ifdef USE_CACHE
			/* make sure that the cache is coherent */
			nfs_cache_expire ();
			nfs_cache_remove (ni);
# endif
			r = release_mount_slot (ni);
			if (r != 0)
				DEBUG (("nfs_fscntl: unmount fialed with %ld", r));
			
			return r;
		}
		case NFS_MNTDUMP:
		{
			/* for debugging only */
			return ENOSYS;
		}
		case NFS_DUMPALL:
		{
			/* for debugging only */
			return ENOSYS;
		}
	}
	
	DEBUG (("nfs_fcntl -> ENOSYS"));
	return ENOSYS;
}

static long _cdecl
nfs_dskchng (int drv, int mode)
{
	TRACE (("nfs_dskchng -> 0"));
	return 0;
}

static long _cdecl
nfs_release (fcookie *fc)
{
	NFS_INDEX *ni = (NFS_INDEX*)fc->index;
	
	if (ni != ROOT_INDEX)
	{
		ni->link -= 1;
		
		if (ni->link < 0)
			/* this was invalid! */
			return EBADF;
		
		if (0 == ni->link)
			free_slot (ni);
	}
	
	return 0;
}

static long _cdecl
nfs_dupcookie (fcookie *dest, fcookie *src)
{
	NFS_INDEX *ni = (NFS_INDEX *) src->index;
	
	*dest = *src;
	if (ni != ROOT_INDEX)
	{
		/* index is in use once more */
		ni->link += 1;
	}
	
	return 0;
}
