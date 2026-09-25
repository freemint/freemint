/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

/*
 * File : nfssys.c
 *        networking filesystem driver, NFS version 3 (RFC 1813)
 */

# include "nfssys.h"
# include "nfsdev.h"

# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/pathconf.h"

# include "cache.h"
# include "index.h"
# include "nfsutil.h"
# include "sock_ipc.h"
# include "version.h"


static long	do_remove	(long nfs_opcode, fcookie *dir, const char *name);
static long	do_fsinfo	(NFS_INDEX *ni);


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
static long	_cdecl nfs_sync		(void);


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
	FS_DO_SYNC		|
	FS_OWN_MEDIACHANGE	|
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
	nfs_sync,

	/* FS_EXT_1 */
	NULL, NULL,

	/* FS_EXT_2
	 */

	/* FS_EXT_3 */
	nfs_stat64,

	0, 0, 0, 0, 0,
	NULL, NULL
};



/* An index may have been created by nfs_readdir(), which -- unlike
 * READDIRPLUS -- does not deliver file handles. In that case we have to
 * ask the server before the index can be used for anything else.
 */
long
nfs_get_handle (NFS_INDEX *ni)
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
		fc1.index = (long) ni->dir;

		r = nfs_lookup (&fc1, ni->name, &fc2);
		if (r != 0)
		{
			DEBUG (("get_handle: failed to get handle, -> ENOENT"));
			return ENOENT;
		}

		newi = (NFS_INDEX *) fc2.index;
		if (newi != ni)
		{
			ni->handle = newi->handle;
			ni->attr = newi->attr;
			ni->size = newi->size;
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
	init_ipc (NFS_PROGRAM, NFS3_VERSION);
	root_attr.blksize = sizeof (NFS_INDEX);
}

static long _cdecl
nfs_root (int drv, fcookie *fc)
{
	TRACE (("nfs_root"));

	if (drv != nfs_dev)
	{
		DEBUG (("nfs_root(%d) != %d -> ENXIO", drv, nfs_dev));
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
	long req_buf [LOOKUPBUFSIZE / sizeof (long)];
	NFS_INDEX *ni, *newi;
	long r;
	int dom;
	MESSAGE *mreq, *mrep, m;
	diropargs3 dirargs;
	lookup3res dirres;
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
			DEBUG (("nfs_lookup(%s) -> ENOENT", name));
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
	if (nfs_get_handle (ni) != 0)
 	{
		DEBUG (("nfs_lookup(%s): no handle for current dir, -> ENOTDIR", name));
		return ENOTDIR;
	}

	dirargs.dir = ni->handle;
	dirargs.name = name;

	mreq = alloc_message (&m, (char *) req_buf, LOOKUPBUFSIZE,
			      xdr_size_diropargs3 (&dirargs));
	if (!mreq)
	{
		DEBUG (("nfs_lookup(%s): failed to alloc request msg, -> ENOENT", name));
		return ENOENT;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_diropargs3 (&x, &dirargs))
	{
		DEBUG (("nfs_lookup(%s): failed to encode arguments", name));
		free_message (mreq);
		return ENOENT;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_LOOKUP, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_lookup(%s): couldn't contact server, -> ENOENT", name));
		return IS_TRANSPORT_ERROR (r) ? r : ENOENT;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_lookup3res (&x, &dirres))
	{
		DEBUG (("nfs_lookup(%s): couldnt decode results, -> ENOENT", name));
		free_message (mrep);
		return ENOENT;
	}

	free_message (mrep);

	/* the directory attributes come with every LOOKUP3 reply, use them */
	update_index_attr (ni, &dirres.dir_attributes);

	if (dirres.status != NFS3_OK)
	{
		DEBUG (("nfs_lookup(%s) rpc->%ld -> %ld", name, dirres.status,
			nfs3_error (dirres.status, ENOENT)));
		return nfs3_error (dirres.status, ENOENT);
	}

	newi = get_slot (ni, name, dom);
	if (!newi)
		return EMFILE;

	newi->dir = ni;
	newi->link += 1;
	newi->handle = dirres.object;
	newi->flags &= ~NO_HANDLE;

	if (dirres.obj_attributes.attributes_follow)
		set_index_attr (newi, &dirres.obj_attributes.attributes);
	else
		/* force a GETATTR3 on the next nfs_getxattr() */
		newi->stamp = get_timestamp () - ni->opt->actimeo - 1;

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


/* Common tail of CREATE3, MKDIR3 and SYMLINK3: take over the result and
 * build a new index for the object that was just created.
 *
 * Unlike NFS2, where the new file handle was part of the reply, NFS3
 * returns it only optionally (post_op_fh3). If the server left it out we
 * mark the index as handle-less; get_handle() will then do a LOOKUP3
 * when the handle is needed for the first time.
 */
static long
finish_create (fcookie *dir, const char *name, create3res *res, fcookie *fc)
{
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;
	NFS_INDEX *newi;

	update_index_attr (ni, &res->dir_wcc.after);

	newi = get_slot (ni, name, p_domain (-1));
	if (!newi)
	{
		DEBUG (("finish_create: no slot found -> EACCES"));
		return EACCES;
	}

	newi->dir = ni;
	newi->link += 1;

	if (res->obj.handle_follows)
	{
		newi->handle = res->obj.handle;
		newi->flags &= ~NO_HANDLE;
	}
	else
	{
		newi->handle.len = 0;
		newi->flags |= NO_HANDLE;
	}

	if (res->obj_attributes.attributes_follow)
		set_index_attr (newi, &res->obj_attributes.attributes);
	else
		newi->stamp = get_timestamp () - ni->opt->actimeo - 1;

	if (fc)
	{
		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long) newi;
	}

	return 0;
}

static long _cdecl
nfs_creat (fcookie *dir, const char *name, unsigned int mode, int attrib, fcookie *fc)
{
	long req_buf [CREATEBUFSIZE / sizeof (long)];
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;
	MESSAGE *mreq, *mrep, m;
	create3args arg;
	create3res res;
	xdrs x;
	long r;

	(void) attrib;
	TRACE (("nfs_creat(%s)", name));

	if (ROOT_INDEX == ni)
	{
		/* only mount dcntl() is allowed in the root dir */
		DEBUG (("nfs_creat(%s): no creation in root dir -> EACCES", name));
		return EACCES;
	}

	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("nfs_creat: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_creat(%s): no handle for current dir, -> ENOTDIR", name));
		return ENOTDIR;
	}

	arg.where.dir = ni->handle;
	arg.where.name = name;

	/* UNCHECKED is what NFS2's CREATE did: create it, and if it is
	 * already there just truncate it. The kernel takes care of O_EXCL.
	 */
	arg.how = UNCHECKED;
	sattr3_init (&arg.obj_attributes);
	arg.obj_attributes.set_mode = TRUE;
	arg.obj_attributes.mode = nfs3_mode ((ushort) mode);
	arg.obj_attributes.set_size = TRUE;
	arg.obj_attributes.size = 0;

	/* owner and group are taken from the RPC credentials; asking for a
	 * specific uid/gid the way the NFS2 driver did makes the request
	 * fail on servers that squash our credentials.
	 */

	mreq = alloc_message (&m, (char *) req_buf, CREATEBUFSIZE,
			      xdr_size_create3args (&arg));
	if (!mreq)
	{
		DEBUG (("nfs_creat(%s): failed to alloc request msg, -> EACCES", name));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_create3args (&x, &arg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_CREATE, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_creat(%s): couldn't contact server, -> EACCES", name));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_create3res (&x, &res))
	{
		DEBUG (("nfs_creat(%s): couldnt decode results, -> EACCES", name));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	if (res.status != NFS3_OK)
	{
		update_index_attr (ni, &res.dir_wcc.after);
		DEBUG (("nfs_creat(%s) rpc->%ld", name, res.status));
		return nfs3_error (res.status, EACCES);
	}

	return finish_create (dir, name, &res, fc);
}

static long _cdecl
nfs_mkdir (fcookie *dir, const char *name, unsigned int mode)
{
	long req_buf [CREATEBUFSIZE / sizeof (long)];
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;
	MESSAGE *mreq, *mrep, m;
	mkdir3args arg;
	create3res res;
	xdrs x;
	long r;

	TRACE (("nfs_mkdir(%s)", name));

	if (ROOT_INDEX == ni)
	{
		/* only mount dcntl() is allowed in the root dir */
		DEBUG (("nfs_mkdir(%s): no creation in root dir -> EACCES", name));
		return EACCES;
	}

	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("nfs_mkdir: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_mkdir(%s): no handle for current dir, -> ENOTDIR", name));
		return ENOTDIR;
	}

	arg.where.dir = ni->handle;
	arg.where.name = name;
	sattr3_init (&arg.attributes);
	arg.attributes.set_mode = TRUE;
	arg.attributes.mode = nfs3_mode ((ushort) mode);

	mreq = alloc_message (&m, (char *) req_buf, CREATEBUFSIZE,
			      xdr_size_mkdir3args (&arg));
	if (!mreq)
	{
		DEBUG (("nfs_mkdir(%s): failed to alloc request msg", name));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_mkdir3args (&x, &arg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_MKDIR, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_mkdir(%s): couldn't contact server, -> EACCES", name));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_create3res (&x, &res))
	{
		DEBUG (("nfs_mkdir(%s): couldnt decode results, -> EACCES", name));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	if (res.status != NFS3_OK)
	{
		update_index_attr (ni, &res.dir_wcc.after);
		DEBUG (("nfs_mkdir(%s) rpc->%ld", name, res.status));
		return nfs3_error (res.status, EACCES);
	}

	/* MiNT does not want a cookie for the new directory */
	return finish_create (dir, name, &res, NULL);
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
	long req_buf [XATTRBUFSIZE / sizeof (long)];
	long stamp, r;
	MESSAGE *mreq, *mrep, m;
	getattr3res stat_res;
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

	if (nfs_get_handle (ni) != 0)
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

	mreq = alloc_message (&m, (char *) req_buf, XATTRBUFSIZE,
			      xdr_size_nfs_fh3 (&ni->handle));
	if (!mreq)
	{
		DEBUG (("nfs_getxattr(%s): failed to alloc msg, -> ENOENT", ni->name));
		return ENOENT;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_nfs_fh3 (&x, &ni->handle))
	{
		free_message (mreq);
		return ENOENT;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_GETATTR, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_getxattr(%s): couldn't contact server, -> EACCES",ni->name));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_getattr3res (&x, &stat_res))
	{
		DEBUG (("nfs_getxattr(%s): couldnt decode results, -> EACCES", ni->name));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("nfs_getxattr(%s) rpc->%ld", ni->name, stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
	}

	set_index_attr (ni, &stat_res.obj_attributes);

	if (xattr)
	{
		*xattr = ni->attr;
		xattr->dev = fc->dev;

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

	(void) ni;		/* suppress warning */
	DEBUG (("nfs_stat64(%s)", (ni) ? ni->name : "root"));

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

		stat->atime.high_time	= 0;
		stat->atime.time	= XATTRL_TD(xattr,a);
		stat->atime.nanoseconds	= 0;

		stat->mtime.high_time	= 0;
		SHORT2LONG(xattr.mtime, xattr.mdate, stat->mtime.time);
		stat->mtime.nanoseconds	= 0;

		stat->ctime.high_time	= 0;
		SHORT2LONG(xattr.ctime, xattr.cdate, stat->ctime.time);
		stat->ctime.nanoseconds	= 0;

		/* NFS3 knows the real 64 bit size; hand it out here even
		 * though the 16 bit XATTR path had to truncate it
		 */
		if (ROOT_INDEX != ni)
			stat->size = (llong) ni->size;
		else
			stat->size = xattr.size;

		stat->blocks	= (xattr.nblocks * xattr.blksize) >> 9;
		stat->blksize	= xattr.blksize;

		stat->flags	= 0;
		stat->gen	= 0;

		bzero (stat->res, sizeof (stat->res));
	}

	return r;
}

long
do_sattr (fcookie *fc, sattr3 *ap)
{
	long req_buf [SATTRBUFSIZE / sizeof (long)];
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	long r;
	MESSAGE *mreq, *mrep, m;
	setattr3args s_arg;
	wcc3res stat_res;
	xdrs x;

	TRACE (("do_sattr(%s)", ni->name));

	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("do_sattr: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("do_sattr(%s): failed to get handle, -> ENOENT", ni->name));
		return ENOENT;
	}

	s_arg.object = ni->handle;
	s_arg.new_attributes = *ap;
	s_arg.check = FALSE;		/* no sattrguard3 */

	mreq = alloc_message (&m, (char *) req_buf, SATTRBUFSIZE,
			      xdr_size_setattr3args (&s_arg));
	if (!mreq)
	{
		DEBUG (("do_sattr(%s): failed to allocate request message", ni->name));
		return ENOENT;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_setattr3args (&x, &s_arg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_SETATTR, &mrep);
	if (r != 0)
	{
		DEBUG (("do_sattr(%s): couldn't contact server, -> EACCES", ni->name));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_wcc3res (&x, &stat_res))
	{
		free_message (mrep);

		DEBUG (("do_sattr(%s): couldnt decode results, -> EACCES", ni->name));
		return EACCES;
	}

	free_message (mrep);

	update_index_attr (ni, &stat_res.wcc.after);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("do_sattr(%s) rpc->%ld", ni->name, stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
	}

	if (!stat_res.wcc.after.attributes_follow)
		/* the cached attributes are stale now */
		ni->stamp = get_timestamp () - ni->opt->actimeo - 1;

	TRACE (("do_sattr(%s) -> OK", ni->name));
	return E_OK;
}

long
do_commit (fcookie *fc, uint64 offset, ulong count, char *verf)
{
	long req_buf [COMMITBUFSIZE / sizeof (long)];
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	MESSAGE *mreq, *mrep, m;
	commit3args arg;
	commit3res res;
	xdrs x;
	long r;

	if (nfs_get_handle (ni) != 0)
		return ENOENT;

	arg.file = ni->handle;
	arg.offset = offset;
	arg.count = count;

	mreq = alloc_message (&m, (char *) req_buf, COMMITBUFSIZE,
			      xdr_size_commit3args (&arg));
	if (!mreq)
		return ENOMEM;

	/* nfs_sync() commits from the update daemon's context, so use the
	 * credentials of the process whose data we are flushing
	 */
	if (ni->wcred.valid)
		mreq->cred = &ni->wcred;

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_commit3args (&x, &arg))
	{
		free_message (mreq);
		return EBADARG;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_COMMIT, &mrep);
	if (r != 0)
	{
		DEBUG (("do_commit(%s): couldn't contact server", ni->name));
		return IS_TRANSPORT_ERROR (r) ? r : EWRITE;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_commit3res (&x, &res))
	{
		free_message (mrep);
		return EWRITE;
	}

	free_message (mrep);

	update_index_attr (ni, &res.file_wcc.after);

	if (res.status != NFS3_OK)
		return nfs3_error (res.status, EWRITE);

	if (verf)
		memcpy (verf, res.verf, NFS3_WRITEVERFSIZE);

	return E_OK;
}

static long _cdecl
nfs_chattr (fcookie *fc, int attrib)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr3 attr;
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
		ushort mode;

		/* set write permissions correctly */
		mode = ni->attr.mode | (S_IWOTH|S_IWGRP|S_IWUSR);
		if (attrib & FA_RDONLY)
			mode &= ~(S_IWOTH|S_IWGRP|S_IWUSR);

		sattr3_init (&attr);
		attr.set_mode = TRUE;
		attr.mode = nfs3_mode (mode);

		return do_sattr (fc, &attr);
	}

	/* BUG: we should do some time calculations on which the archive
	 *      attribute setting could be based. Also, the system and hidden
	 *      attribute should be maintained somehow.
	 */
	DEBUG (("nfs_chattr: other than readonly attribute not implemented"));
	return EACCES;
}

static long _cdecl
nfs_chown (fcookie *fc, int uid, int gid)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr3 attr;

	TRACE (("nfs_chown"));

	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_chown on root dir"));

		if (uid != -1) root_attr.uid = uid;
		if (gid != -1) root_attr.gid = gid;

		return 0;
	}

	sattr3_init (&attr);

	if (uid != -1)
	{
		attr.set_uid = TRUE;
		attr.uid = (ulong) (ushort) uid;
	}

	if (gid != -1)
	{
		attr.set_gid = TRUE;
		attr.gid = (ulong) (ushort) gid;
	}

	if (!attr.set_uid && !attr.set_gid)
		return E_OK;

	return do_sattr (fc, &attr);
}

static long _cdecl
nfs_chmode (fcookie *fc, unsigned int mode)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;
	sattr3 attr;

	TRACE (("nfs_chmode"));

	if (ROOT_INDEX == ni)
	{
		TRACE (("nfs_chmode on root dir"));

		/* make sure to preserve the file type
		 */
		root_attr.mode = (root_attr.mode & S_IFMT) | (mode & ~S_IFMT);
		return 0;
	}

	sattr3_init (&attr);
	attr.set_mode = TRUE;
	attr.mode = nfs3_mode ((ushort) mode);

	return do_sattr (fc, &attr);
}

static long
do_remove (long nfs_opcode, fcookie *dir, const char *name)
{
	long req_buf [REMBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	diropargs3 dirargs;
	wcc3res stat_res;
	xdrs x;
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

	DEBUG (("do_remove(%s)", name));

	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("do_remove: mount is read-only ->EACCES"));
		return EACCES;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("do_remove(%s): failed to get handle, -> ENOTDIR", name));
		return ENOTDIR;
	}

	dirargs.dir = ni->handle;
	dirargs.name = name;

	mreq = alloc_message (&m, (char *) req_buf, REMBUFSIZE,
			      xdr_size_diropargs3 (&dirargs));
	if (!mreq)
	{
		DEBUG (("do_remove(%s): failed to allocate buffer -> EACCES", name));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_diropargs3 (&x, &dirargs))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&ni->opt->server, mreq, nfs_opcode, &mrep);
	if (r != 0)
	{
		DEBUG (("do_remove(%s): couldn't contact server, -> EACCES", name));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_wcc3res (&x, &stat_res))
	{
		DEBUG (("do_remove(%s): couldnt decode results, -> EACCES", name));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	update_index_attr (ni, &stat_res.wcc.after);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("do_remove(%s, %ld) rpc->%ld", name, nfs_opcode, stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
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
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

	TRACE (("nfs_rmdir"));
	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_rmdir(%s): no remove from root dir, -> EACCES", name));
		return EACCES;
	}

	return do_remove (NFSPROC3_RMDIR, dir, name);
}

static long _cdecl
nfs_remove (fcookie *dir, const char *name)
{
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

	TRACE (("nfs_remove"));
	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_remove(%s): no remove from root dir, -> EACCES", name));
		return EACCES;
	}

	return do_remove (NFSPROC3_REMOVE, dir, name);
}

static long _cdecl
nfs_getname (fcookie *relto, fcookie *dir, char *pathname, int size)
{
	NFS_INDEX *ni, *oni, *reli;
	int len, copy_name;

	(void) reli;		/* suppress warning */

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

	while (oni != (NFS_INDEX *) relto->index)
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
		oni = ni;
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
		strcat (pathname, "\\");
		size -= len;
		if (size < (len = strlen (ni->name)))
			return EBADARG;
		strcat (pathname, ni->name);
		size -= len;
	}
	while (ni != (NFS_INDEX *) dir->index)
	{
		ni = ni->aux;
		if (!ni)
			return ENOTDIR;
		if (size < (len = 1))
			return EBADARG;
		strcat (pathname, "\\");
		size -= len;
		if (size < (len = strlen (ni->name)))
			return EBADARG;
		strcat (pathname, ni->name);
		size -= len;
	}
	TRACE (("nfs_getname -> '%s'", pathname));
	return 0;
}

static long _cdecl
nfs_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	long req_buf [RENBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	rename3args renarg;
	rename3res stat_res;
	xdrs x;
	NFS_INDEX *newi = (NFS_INDEX *) newdir->index;
	NFS_INDEX *oldi = (NFS_INDEX *) olddir->index;

	TRACE (("nfs_rename('%s' -> '%s')", oldname, newname));
	if ((ROOT_INDEX == oldi) || (ROOT_INDEX == newi))
	{
		DEBUG (("nfs_rename(%s): no rename in the root dir, -> EACCES", oldname));
		return EACCES;
	}

	if ((oldi->opt->flags & OPT_RO) || (newi->opt->flags & OPT_RO))
	{
		DEBUG (("nfs_rename: mount is read-only -> EACCES"));
		return EACCES;
	}

	/* NFS3 has its own error code for this, but catching it early
	 * saves a round trip
	 */
	if (oldi->opt != newi->opt)
	{
		DEBUG (("nfs_rename: cross mount rename -> EXDEV"));
		return EXDEV;
	}

	if (nfs_get_handle (newi) != 0)
	{
		DEBUG (("nfs_rename(%s): no handle for new dir, -> ENOTDIR", oldname));
		return ENOTDIR;
	}

	if (nfs_get_handle (oldi) != 0)
	{
		DEBUG (("nfs_rename(%s): no handle for old dir, -> ENOTDIR", oldname));
		return ENOTDIR;
	}

	renarg.from.dir = oldi->handle;
	renarg.from.name = oldname;
	renarg.to.dir = newi->handle;
	renarg.to.name = newname;

	mreq = alloc_message (&m, (char *) req_buf, RENBUFSIZE,
			      xdr_size_rename3args (&renarg));
	if (!mreq)
	{
		DEBUG (("nfs_rename(%s): failed to allocate buffer -> EACCES", oldname));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_rename3args (&x, &renarg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&oldi->opt->server, mreq, NFSPROC3_RENAME, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_rename(%s): couldn't contact server, -> EACCES", oldname));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);

	if (!xdr_rename3res (&x, &stat_res))
	{
		DEBUG (("nfs_rename(%s): couldnt decode results, -> EACCES", oldname));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	update_index_attr (oldi, &stat_res.fromdir_wcc.after);
	if (newi != oldi)
		update_index_attr (newi, &stat_res.todir_wcc.after);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("nfs_rename(%s) rpc->%ld", oldname, stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
	}

	nfs_cache_removebyname (oldi, oldname);
	nfs_cache_removebyname (newi, newname);

	TRACE (("nfs_rename('%s' -> '%s') -> OK", oldname, newname));
	return 0;
}




/* Add this to the length of the buffer holding the decoded entries of
 * a directory. The decoded form needs a zero terminator per name and up
 * to three padding bytes per entry, where the wire format needs none.
 */
# define ADD_BUF_LEN	(MAX_READDIR_LEN / 4)


/* this is placed in the fsstuff field of a dir handle */
typedef struct
{
	char *		buffer;		/* current entry buffer */
	entry3 *	curr_entry;	/* this is the entry who is returned next */
	cookie3		lastcookie;	/* 64 bit in NFS3, was 4 opaque bytes */
	char		cookieverf[NFS3_COOKIEVERFSIZE];
	short		eof;		/* if set, this buffer is the last in the dir */
} NETFS_STUFF;


static long _cdecl
nfs_opendir (DIR *dirh, int flags)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) dirh->fsstuff;
	NFS_INDEX *ni = (NFS_INDEX *) dirh->fc.index;

	(void) flags;

	if (ROOT_INDEX != ni)
	{
		if (nfs_get_handle (ni) != 0)
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
	stuff->lastcookie = 0;
	bzero (stuff->cookieverf, NFS3_COOKIEVERFSIZE);
	stuff->eof = 0;
	dirh->index = 0;

	TRACE (("nfs_opendir(%s) -> ok", (ni) ? ni->name : "root"));
	return 0;
}

static long _cdecl
nfs_rewinddir (DIR *dirh)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) dirh->fsstuff;

	if (ROOT_INDEX != (NFS_INDEX *) dirh->fc.index)
	{
		stuff->curr_entry = NULL;
		stuff->lastcookie = 0;
		bzero (stuff->cookieverf, NFS3_COOKIEVERFSIZE);
		stuff->eof = 0;
	}

	dirh->index = 0;

	TRACE (("nfs_rewinddir -> ok"));
	return 0;
}

static long _cdecl
nfs_closedir (DIR *dirh)
{
	NETFS_STUFF *stuff = (NETFS_STUFF *) dirh->fsstuff;

	if (ROOT_INDEX != (NFS_INDEX *) dirh->fc.index)
	{
		if (stuff->buffer)
			kfree (stuff->buffer);

		stuff->buffer = NULL;
	}

	TRACE (("nfs_closedir -> ok"));
	return 0;
}

static long _cdecl
nfs_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	long req_buf [READDIRBUFSIZE / sizeof (long)];
	int giveindex = dirh->flags == 0;
	int i, dom;
	entry3 *entp;
	NFS_INDEX *ni = (NFS_INDEX *) dirh->fc.index;
	NFS_INDEX *newi;
	MESSAGE *mreq, *mrep, m;
	long r;
	readdir3args read_arg;
	readdir3res read_res;
	xdrs x;
	NETFS_STUFF *stuff = (NETFS_STUFF *) dirh->fsstuff;

	/* we know that ni has a handle, as we did get one in
	 * or before nfs_opendir
	 */
	dom = p_domain (-1);
	if (ROOT_INDEX == ni)
	{
		/* read the root dir of the file sys */
		TRACE (("nfs_readdir(root)"));
		if (giveindex)
		{
			namelen -= sizeof (long);
			if (namelen <= 0)
				return EBADARG;

			unaligned_putl (name, dirh->index);
			name += sizeof (long);
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
			DEBUG (("nfs_readdir(root) -> no more files"));
			return ENMFILES;
		}
		strncpy (name, ni->name, namelen-1);
		name[namelen-1] = '\0';
		if (0 == dom)   /* convert to upper case for TOS domain */
			strupr (name);
		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long) ni;
		if (strlen (ni->name) >= namelen)
			return EBADARG;
		ni->link += 1;
		TRACE (("nfs_readdir -> '%s'", name));
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
			namelen -= sizeof (long);
			if (namelen <= 0)
				return EBADARG;

			/* NFS3 file ids are 64 bit, MiNT's directory index
			 * is not; hand out the lower half
			 */
			unaligned_putl (name, (long) (entp->fileid & 0xffffffffULL));
			name += sizeof (long);
		}
		strncpy (name, entp->name, namelen-1);
		name[namelen-1] = '\0';
		if (0 == dom)    /* convert to upper case for TOS domain */
			strupr (name);
		if (strlen (entp->name) >= namelen)
		{
			DEBUG (("nfs_readdir(%s): name buffer (%d) too short",
			                                          ni->name, namelen));
			res = EBADARG;
			goto prep_next_entry;
		}

		/* check for entries '.' and '..' which have already a local slot */
		if (!strcmp (entp->name, "."))
		{
			newi = ni;  /* '.' does always mean the read directory */
		}
		else if (!strcmp (entp->name, ".."))
		{
			newi = ni->dir;   /* '..' means the parent of the read directory */
		}
		else
		{
			TRACE (("nfs_readdir: getting new slot for '%s'", entp->name));
			newi = get_slot (ni, entp->name, (dirh->flags & TOS_SEARCH) ? 0 : 1);
			if (!newi)
			{
				DEBUG (("nfs_readdir(%s): no index for entry, -> EMFILE", ni->name));
				res = EMFILE;
				goto prep_next_entry;
			}

			/* plain READDIR3 delivers no file handle, so a
			 * fresh index stays incomplete until it is really
			 * used. An index we already have a handle for
			 * keeps it -- no point in looking it up again.
			 */
			if (newi->handle.len == 0)
			{
				newi->flags |= NO_HANDLE;
				newi->stamp = get_timestamp ()
					      - ni->opt->actimeo - 1;
			}
		}
		if (newi)
		{
			newi->link += 1;
		}

		fc->fs = &nfs_filesys;
		fc->dev = nfs_dev;
		fc->aux = 0;
		fc->index = (long) newi;

prep_next_entry:
		stuff->lastcookie = entp->cookie;
		stuff->curr_entry = entp->nextentry;
		DEBUG (("nfs_readdir(%s) -> %s", ni->name, name));
		return res;
	}
	if (stuff->eof)
	{
		TRACE (("nfs_readdir(%s): end of dir reached, -> ENMFILES", ni->name));
		return ENMFILES;
	}

	/* ask the server for another chunk of directory entries */
	TRACE (("nfs_readdir: requesting new chunk"));
	read_arg.dir = ni->handle;
	read_arg.cookie = stuff->lastcookie;
	memcpy (read_arg.cookieverf, stuff->cookieverf, NFS3_COOKIEVERFSIZE);
	/* count is an upper bound for the size of the reply. Tie it to
	 * rsize as well, so that a mount with a small rsize also keeps the
	 * directory replies small -- useful on hardware that cannot cope
	 * with larger datagrams.
	 */
	{
		long cnt = ni->opt->dtpref;

		if (cnt > ni->opt->rsize)
			cnt = ni->opt->rsize;
		if (cnt > MAX_READDIR_LEN)
			cnt = MAX_READDIR_LEN;
		if (cnt < 1024)
			cnt = 1024;		/* room for a few long names */

		read_arg.count = cnt;
	}

	mreq = alloc_message (&m, (char *) req_buf, READDIRBUFSIZE,
			      xdr_size_readdir3args (&read_arg));
	if (!mreq)
	{
		DEBUG (("nfs_readdir(%s): failed to alloc msg, -> ENMFILES", ni->name));
		return ENMFILES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_readdir3args (&x, &read_arg))
	{
		free_message (mreq);
		return ENMFILES;
	}

	TRACE (("nfs_readdir: sending request"));

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_READDIR, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_readdir(%s): couldnt contact server, -> ENMFILES", ni->name));
		return IS_TRANSPORT_ERROR (r) ? r : ENMFILES;
	}

	TRACE (("nfs_readdir: got answer"));

	bzero (stuff->buffer, MAX_READDIR_LEN + ADD_BUF_LEN);
	read_res.buffer = stuff->buffer;
	read_res.buflen = MAX_READDIR_LEN + ADD_BUF_LEN;

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_readdir3res (&x, &read_res))
	{
		DEBUG (("nfs_readdir(%s): could not decode results, -> ENMFILES", ni->name));
		free_message (mrep);
		return ENMFILES;
	}

	free_message (mrep);

	update_index_attr (ni, &read_res.dir_attributes);

	if (NFS3ERR_TOOSMALL == read_res.status)
	{
		/* Our count was too small for even one entry. Ask for more
		 * instead of pretending the directory ended here -- which is
		 * what a single "return ENMFILES" for every status did.
		 */
		if (ni->opt->dtpref < MAX_READDIR_LEN)
		{
			ni->opt->dtpref = (ni->opt->dtpref < 1024)
					  ? 2048 : (ni->opt->dtpref * 2);

			if (ni->opt->dtpref > MAX_READDIR_LEN)
				ni->opt->dtpref = MAX_READDIR_LEN;

			if (ni->opt->rsize < ni->opt->dtpref)
				ni->opt->rsize = ni->opt->dtpref;

			ALERT (("nfs3: readdir buffer too small, retrying "
				"with %ld bytes", ni->opt->dtpref));

			goto restart;
		}

		ALERT (("nfs3: readdir needs more than %d bytes", MAX_READDIR_LEN));
		return EBADARG;
	}

	if (NFS3_OK != read_res.status)
	{
		/* a real error -- do not disguise it as end of directory */
		ALERT (("nfs3: readdir(%s) failed, nfsstat3 %ld",
			ni->name, read_res.status));
		return nfs3_error (read_res.status, ENMFILES);
	}

	{
		/* The cookie verifier is new in NFS3: it has to be handed
		 * back unchanged with every follow-up request so that the
		 * server can detect that the directory was rewritten in
		 * between.
		 */
		memcpy (stuff->cookieverf, read_res.cookieverf, NFS3_COOKIEVERFSIZE);
		stuff->eof = read_res.eof ? 1 : 0;
		stuff->curr_entry = read_res.entries;

		if (stuff->curr_entry || stuff->eof)
			goto restart;
	}

	TRACE (("nfs_readdir(%s) -> no more files", ni->name));
	return ENMFILES;
}





static long _cdecl
nfs_pathconf (fcookie *dir, int which)
{
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

	TRACE (("nfs_pathconf(%d)", which));

	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:
		{
			/* FSINFO3 tells us whether the server can do hard
			 * links at all; NFS2 had no way of asking
			 */
			if (ROOT_INDEX == ni)
				return 1;

			return (ni->opt->properties & FSF3_LINK) ? UNLIMITED : 1;
		}
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
					);
		case DP_XATTRFIELDS:	return (DP_INDEX
						| DP_DEV
						| DP_RDEV
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
	long req_buf [DFREEBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	xdrs x;
	fsstat3res stat_res;
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

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

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_dfree: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}

	mreq = alloc_message (&m, (char *) req_buf, DFREEBUFSIZE,
			      xdr_size_nfs_fh3 (&ni->handle));
	if (!mreq)
	{
		DEBUG (("nfs_dfree: failed to allocate buffer, -> ENOTDIR"));
		return ENOTDIR;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_nfs_fh3 (&x, &ni->handle))
	{
		free_message (mreq);
		return ENOTDIR;
	}

	/* NFS2's STATFS was replaced by FSSTAT3, which reports bytes
	 * instead of blocks, as 64 bit quantities
	 */
	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_FSSTAT, &mrep);
	if (r)
	{
		DEBUG (("nfs_dfree: couldn't contact server, -> ENOTDIR"));
		return IS_TRANSPORT_ERROR (r) ? r : ENOTDIR;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_fsstat3res (&x, &stat_res))
	{
		free_message (mrep);

		DEBUG (("nfs_dfree: couldnt decode results, -> ENOTDIR"));
		return ENOTDIR;
	}

	free_message (mrep);

	update_index_attr (ni, &stat_res.obj_attributes);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("nfs_dfree rpc->%ld", stat_res.status));
		return nfs3_error (stat_res.status, ENOTDIR);
	}

	/* Report 1 KB "sectors" so that even large exports still fit into
	 * the 32 bit values GEMDOS expects.
	 */
	buf[0] = clamp64 (stat_res.abytes >> 10);	/* free clusters */
	buf[1] = clamp64 (stat_res.tbytes >> 10);	/* total clusters */
	buf[2] = 1024;					/* bytes per sector */
	buf[3] = 1;					/* sectors per cluster */

	return E_OK;
}

static char nfs_label [MAX_LABEL+1] = "Network3";

static long _cdecl
nfs_writelabel (fcookie *dir, const char *name)
{
	TRACE (("nfs_writelabel"));

	if (ROOT_INDEX == (NFS_INDEX *) dir->index)
	{
		if (strlen (name) > MAX_LABEL)
			return EBADARG;

		strncpy (nfs_label, name, MAX_LABEL);
		nfs_label[MAX_LABEL] = '\0';

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
	long req_buf [SYMLNBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	symlink3args symarg;
	create3res stat_res;
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

	if (!(ni->opt->properties & FSF3_SYMLINK))
	{
		DEBUG (("nfs_symlink: server does not support symlinks"));
		return ENOSYS;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_symlink: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}

	symarg.where.dir = ni->handle;
	symarg.where.name = name;
	symarg.symlink_data = to;
	sattr3_init (&symarg.symlink_attributes);
	symarg.symlink_attributes.set_mode = TRUE;
	symarg.symlink_attributes.mode = 0777;

	mreq = alloc_message (&m, (char *) req_buf, SYMLNBUFSIZE,
			      xdr_size_symlink3args (&symarg));
	if (!mreq)
	{
		DEBUG (("nfs_symlink: failed to allocate buffer, -> EACCES"));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_symlink3args (&x, &symarg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_SYMLINK, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_symlink: couldn't contact server, -> EACCES"));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);

	if (!xdr_create3res (&x, &stat_res))
	{
		DEBUG (("nfs_symlink: couldnt decode results, -> EACCES"));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	update_index_attr (ni, &stat_res.dir_wcc.after);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("nfs_symlink rpc->%ld", stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
	}

	TRACE (("nfs_symlink -> OK"));
	return 0;
}

static long _cdecl
nfs_readlink (fcookie *dir, char *buf, int len)
{
	long req_buf [READLNBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	readlink3res link_res;
	char databuf[MAXPATHLEN+1];
	xdrs x;
	NFS_INDEX *ni = (NFS_INDEX *) dir->index;

	TRACE (("nfs_readlink"));
	if ((ROOT_INDEX == ni) || (ni->flags & IS_MOUNT_DIR))
	{
		DEBUG (("nfs_readlink: no links in root dir"));
		return ENOENT;
	}

	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_readlink: failed to get handle, -> ENOTDIR"));
		return ENOTDIR;
	}

	mreq = alloc_message (&m, (char *) req_buf, READLNBUFSIZE,
			      xdr_size_nfs_fh3 (&ni->handle));
	if (!mreq)
	{
		DEBUG (("nfs_readlink: failed to allocate buffer, -> ENOENT"));
		return ENOENT;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_nfs_fh3 (&x, &ni->handle))
	{
		free_message (mreq);
		return ENOENT;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_READLINK, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_readlink: couldn't contact server, -> ENOENT"));
		return IS_TRANSPORT_ERROR (r) ? r : ENOENT;
	}

	databuf[0] = '\0';
	link_res.data = &databuf[0];

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_readlink3res (&x, &link_res))
	{
		DEBUG (("nfs_readlink: couldnt decode results, -> ENOENT"));
		free_message (mrep);
		return ENOENT;
	}

	free_message (mrep);

	update_index_attr (ni, &link_res.symlink_attributes);

	if (link_res.status != NFS3_OK)
	{
		DEBUG (("nfs_readlink rpc->%ld", link_res.status));
		return nfs3_error (link_res.status, ENOENT);
	}

	{
		short i = len;
		char *p = buf, *cp = databuf;

		while (--i >= 0 && (*p++ = (*cp != '/' ? *cp : '\\')))
			++cp;

		if (i < 0)
		{
			DEBUG (("nfs_readlink: result too long, -> EBADARG"));
			return EBADARG;
		}

		DEBUG (("nfs_readlink -> `%s'", buf));
	}

	TRACE (("nfs_readlink -> OK"));
	return 0;
}

static long _cdecl
nfs_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	long req_buf [HARDLNBUFSIZE / sizeof (long)];
	long r;
	MESSAGE *mreq, *mrep, m;
	link3args linkarg;
	link3res stat_res;
	xdrs x;
	fcookie fc;
	NFS_INDEX *fromi = (NFS_INDEX *) fromdir->index;
	NFS_INDEX *toi = (NFS_INDEX *) todir->index;

	TRACE (("nfs_hardlink(%s -> %s)", fromname, toname));
	if (ROOT_INDEX == toi)
	{
		DEBUG (("nfs_hardlink not allowed in root dir"));
		return EACCES;
	}

	if (toi->opt->flags & OPT_RO)
	{
		DEBUG (("nfs_hardlink: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (!(toi->opt->properties & FSF3_LINK))
	{
		DEBUG (("nfs_hardlink: server does not support hard links"));
		return ENOSYS;
	}

	if (nfs_lookup (fromdir, fromname, &fc) != 0)
	{
		DEBUG (("nfs_hardlink: file not found, -> ENOENT"));
		return ENOENT;
	}

	if (nfs_get_handle (toi) != 0)
	{
		DEBUG (("nfs_hardlink: failed to get handle for dest dir, -> ENOTDIR"));
		nfs_release (&fc);
		return ENOTDIR;
	}

	fromi = (NFS_INDEX *) fc.index;
	if (nfs_get_handle (fromi) != 0)
	{
		nfs_release (&fc);
		return ENOENT;
	}

	linkarg.file = fromi->handle;
	linkarg.link.dir = toi->handle;
	linkarg.link.name = toname;

	nfs_release (&fc);

	mreq = alloc_message (&m, (char *) req_buf, HARDLNBUFSIZE,
			      xdr_size_link3args (&linkarg));
	if (!mreq)
	{
		DEBUG (("nfs_hardlink: failed to allocate buffer, -> EACCES"));
		return EACCES;
	}

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_link3args (&x, &linkarg))
	{
		free_message (mreq);
		return EACCES;
	}

	r = rpc_request (&toi->opt->server, mreq, NFSPROC3_LINK, &mrep);
	if (r != 0)
	{
		DEBUG (("nfs_hardlink: couldn't contact server, -> EACCES"));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_link3res (&x, &stat_res))
	{
		DEBUG (("nfs_hardlink: couldnt decode results, -> EACCES"));
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	update_index_attr (toi, &stat_res.linkdir_wcc.after);

	if (stat_res.status != NFS3_OK)
	{
		DEBUG (("nfs_hardlink rpc->%ld", stat_res.status));
		return nfs3_error (stat_res.status, EACCES);
	}

	TRACE (("nfs_hardlink -> OK"));
	return 0;
}


/* Ask the server for its preferred and maximum transfer sizes and for
 * the features it supports. There is no counterpart in NFS2, where the
 * client had to guess (and the old driver used a hard wired 4k).
 */
static long
do_fsinfo (NFS_INDEX *ni)
{
	long req_buf [FSINFOBUFSIZE / sizeof (long)];
	MESSAGE *mreq, *mrep, m;
	fsinfo3res res;
	xdrs x;
	long r;

	mreq = alloc_message (&m, (char *) req_buf, FSINFOBUFSIZE,
			      xdr_size_nfs_fh3 (&ni->handle));
	if (!mreq)
		return ENOMEM;

	xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
	if (!xdr_nfs_fh3 (&x, &ni->handle))
	{
		free_message (mreq);
		return EBADARG;
	}

	r = rpc_request (&ni->opt->server, mreq, NFSPROC3_FSINFO, &mrep);
	if (r != 0)
	{
		DEBUG (("do_fsinfo: couldn't contact server -> %ld", r));
		return IS_TRANSPORT_ERROR (r) ? r : EACCES;
	}

	xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
	if (!xdr_fsinfo3res (&x, &res))
	{
		free_message (mrep);
		return EACCES;
	}

	free_message (mrep);

	if (res.status != NFS3_OK)
		return nfs3_error (res.status, EACCES);

	if (res.rtmax > 0)
		ni->opt->rtmax = MIN ((long) res.rtmax, (long) MAXDATA);
	if (res.wtmax > 0)
		ni->opt->wtmax = MIN ((long) res.wtmax, (long) MAXDATA);
	if (res.dtpref > 0)
		ni->opt->dtpref = MIN ((long) res.dtpref, (long) MAX_READDIR_LEN);

	ni->opt->properties = res.properties;

	/* honour the server's limits, but never grow beyond what the
	 * mount asked for
	 */
	if (ni->opt->rsize > ni->opt->rtmax)
		ni->opt->rsize = ni->opt->rtmax;
	if (ni->opt->wsize > ni->opt->wtmax)
		ni->opt->wsize = ni->opt->wtmax;

	if (ni->opt->rsize <= 0)
		ni->opt->rsize = DEFAULT_RSIZE;
	if (ni->opt->wsize <= 0)
		ni->opt->wsize = DEFAULT_WSIZE;

	update_index_attr (ni, &res.obj_attributes);

	DEBUG (("do_fsinfo: rsize %ld, wsize %ld, dtpref %ld, props 0x%lx",
		ni->opt->rsize, ni->opt->wsize, ni->opt->dtpref,
		ni->opt->properties));

	return E_OK;
}

static long _cdecl
nfs_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	TRACE (("nfs_fscntl"));

	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "nfs3");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info;

			info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "nfs3-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
				info->type = FS_NFS3;
				strcpy (info->type_asc, "network filesystem (NFS v3)");
			}

			return E_OK;
		}
		case NFS3_MOUNT:
		{
			NFS_MOUNT_INFO *info = (NFS_MOUNT_INFO *) arg;
			NFS_INDEX *ni;

			if (!arg)
				return EACCES;

			if (ROOT_INDEX != (NFS_INDEX *) dir->index)
			{
				DEBUG (("nfs_fscntl: mount only allowed in root dir"));
				return EACCES;
			}

			if (info->handle.len == 0 || info->handle.len > NFS3_FHSIZE)
			{
				DEBUG (("nfs_fscntl: bad file handle length %ld",
					info->handle.len));
				return EBADARG;
			}

			ni = get_mount_slot (name, info);
			if (!ni)
			{
				DEBUG (("nfs_fscntl: failure"));
				return EACCES;
			}

			if (ni->link > 0)
			{
				/* this file was mounted before */

				DEBUG (("nfs_fscntl: no remount allowed, -> EACCES"));
				return EACCES;
			}

			ni->link = 1;
			ni->handle = info->handle;
			ni->flags &= ~NO_HANDLE;

			/* New in NFS3: negotiate the transfer sizes. This is
			 * also the first request that goes over the kernel's
			 * own socket, so it is where an unusable RPC path
			 * shows up. Do not let the mount succeed in that case
			 * -- otherwise the mount looks fine and every later
			 * access fails with a misleading error.
			 */
			{
				long fr = do_fsinfo (ni);

				if (IS_TRANSPORT_ERROR (fr))
				{
					ALERT (("nfs3: no answer from %s over "
						"UDP (error %ld); check that "
						"nfsd serves NFSv3 over UDP and "
						"that udp/2049 is not filtered",
						ni->opt->server.hostname, fr));

					/* ni->link is still the 1 we set above,
					 * which is what release_mount_slot()
					 * expects
					 */
					release_mount_slot (ni);
					return fr;
				}

				if (fr != E_OK)
					DEBUG (("nfs_fscntl: FSINFO3 failed (%ld), "
						"using defaults", fr));
			}

			DEBUG (("nfs_fscntl: mounting dir '%s'", ni->name));
			return 0;
		}
		case NFS3_UNMOUNT:
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
			/* Every cached entry holds a reference on its parent
			 * directory, so a single expire() is not enough: as
			 * long as one child of this mount is still cached,
			 * release_mount_slot() sees link > 1 and refuses.
			 * Drop the whole cache.
			 */
			nfs_cache_flush ();
			nfs_cache_remove (ni);
# endif
			r = release_mount_slot (ni);
			if (r != 0)
				DEBUG (("nfs_fscntl: unmount failed with %ld", r));

			return r;
		}
		case NFS3_MNTDUMP:
		{
			/* for debugging only */
			return ENOSYS;
		}
		case NFS3_DUMPALL:
		{
			/* for debugging only */
			return ENOSYS;
		}
	}

	DEBUG (("nfs_fscntl -> ENOSYS"));
	return ENOSYS;
}

static long _cdecl
nfs_dskchng (int drv, int mode)
{
	(void) drv;
	(void) mode;

	TRACE (("nfs_dskchng -> 0"));
	return 0;
}

static long _cdecl
nfs_release (fcookie *fc)
{
	NFS_INDEX *ni = (NFS_INDEX *) fc->index;

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

/* Flush the unstable write data of every file we still hold.
 *
 * The kernel calls this on Fsync()/sync() and twice while shutting down
 * (see shutdown() in sys/dos.c). Without it, data written with UNSTABLE
 * and not yet closed would sit in the server's memory with nothing to
 * force it out.
 */
static long _cdecl
nfs_sync (void)
{
	INDEX_CLUSTER *icp;
	long err = E_OK;
	int i, j;

	TRACE (("nfs_sync"));

	for (i = 0; i < MAX_CLUSTER; i++)
	{
		for (icp = cluster[i]; icp; icp = icp->next)
		{
			if (icp->n_used <= 0)
				continue;

			for (j = 0; j < CLUSTER_SIZE; j++)
			{
				NFS_INDEX *ni = &icp->index[j];
				char cverf[NFS3_WRITEVERFSIZE];
				fcookie fc;
				long r;

				if (ni->link <= 0 || !ni->wdirty)
					continue;

				fc.fs = &nfs_filesys;
				fc.dev = nfs_dev;
				fc.aux = 0;
				fc.index = (long) ni;

				r = do_commit (&fc, 0, 0, cverf);
				if (r != E_OK)
				{
					ALERT (("nfs3: sync: COMMIT3 of '%s' "
						"failed -> %ld", ni->name, r));
					err = r;
					continue;
				}

				ni->wdirty = 0;

				if (memcmp (ni->wverf, cverf,
					    NFS3_WRITEVERFSIZE) != 0)
				{
					ALERT (("nfs3: sync: server lost "
						"unstable data of '%s'",
						ni->name));
					err = EIO;
				}
			}
		}
	}

	return err;
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
