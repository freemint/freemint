/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include "isofs_global.h"

#include "mint/dcntl.h"
#include "mint/emu_tos.h"
#include "mint/endian.h"
#include "mint/ioctl.h"
#include "mint/pathconf.h"
#include "mint/stat.h"

#include "isofs.h"
#include "isofs_rrip.h"
#include "metados.h"


/*
 * version
 */

#define VER_MAJOR	0
#define VER_MINOR	1

#if 1
#define ALPHA
#endif

#if 0
#define BETA
#endif

/*
 * startup messages
 */

#define MSG_VERSION	str(VER_MAJOR) "." str(VER_MINOR)
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT	\
	"\033p ISO filesystem driver version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"\275 2000-2010 by Frank Naumann <fnaumann@freemint.de>.\r\n" \

#define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

#define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_OLDMINT	\
	"\033pMiNT too old, this xfs requires at least a FreeMiNT 1.15!\033q\r\n"

#define MSG_BIOVERSION	\
	"\033pIncompatible FreeMiNT buffer cache version!\033q\r\n"

#define MSG_BIOREVISION	\
	"\033pFreeMiNT buffer cache revision too old!\033q\r\n"

#define MSG_FAILURE	\
	"\7Sorry, isofs.xfs NOT installed!\r\n\r\n"

#define MSG_FAILURE1(s)	\
	"\7Sorry, isofs.xfs NOT installed: " s "!\r\n\r\n"

/*
 * default settings
 */



/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *KERNEL;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * filesystem
 */

static long	_cdecl isofs_root	(int drv, fcookie *fc);

static long	_cdecl isofs_lookup	(fcookie *dir, const char *name, fcookie *fc);
static DEVDRV *	_cdecl isofs_getdev	(fcookie *fc, long *devsp);
static long	_cdecl isofs_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl isofs_stat64	(fcookie *fc, STAT *stat);

static long	_cdecl isofs_chattr	(fcookie *fc, int attrib);
static long	_cdecl isofs_chown	(fcookie *fc, int uid, int gid);
static long	_cdecl isofs_chmode	(fcookie *fc, unsigned mode);

static long	_cdecl isofs_mkdir	(fcookie *dir, const char *name, unsigned mode);
static long	_cdecl isofs_rmdir	(fcookie *dir, const char *name);
static long	_cdecl isofs_creat	(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long	_cdecl isofs_remove	(fcookie *dir, const char *name);
static long	_cdecl isofs_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl isofs_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long	_cdecl isofs_opendir	(DIR *dirh, int flags);
static long	_cdecl isofs_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl isofs_rewinddir	(DIR *dirh);
static long	_cdecl isofs_closedir	(DIR *dirh);

static long	_cdecl isofs_pathconf	(fcookie *dir, int which);
static long	_cdecl isofs_dfree	(fcookie *dir, long *buf);
static long	_cdecl isofs_writelabel	(fcookie *dir, const char *name);
static long	_cdecl isofs_readlabel	(fcookie *dir, char *name, int namelen);

static long	_cdecl isofs_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl isofs_readlink	(fcookie *file, char *buf, int len);
static long	_cdecl isofs_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl isofs_fscntl	(fcookie *dir, const char *name, int cmd, long arg);
static long	_cdecl isofs_dskchng	(int drv, int mode);

static long	_cdecl isofs_release	(fcookie *fc);
static long	_cdecl isofs_dupcookie	(fcookie *dst, fcookie *src);
static long	_cdecl isofs_sync	(void);

static FILESYS ftab =
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
//	FS_REENTRANT_L1		|
//	FS_REENTRANT_L2		|
	FS_EXT_2		|
	FS_EXT_3		,
	
	isofs_root,
	isofs_lookup, isofs_creat, isofs_getdev, isofs_getxattr,
	isofs_chattr, isofs_chown, isofs_chmode,
	isofs_mkdir, isofs_rmdir, isofs_remove, isofs_getname, isofs_rename,
	isofs_opendir, isofs_readdir, isofs_rewinddir, isofs_closedir,
	isofs_pathconf, isofs_dfree, isofs_writelabel, isofs_readlabel,
	isofs_symlink, isofs_readlink, isofs_hardlink, isofs_fscntl, isofs_dskchng,
	isofs_release, isofs_dupcookie,
	isofs_sync,
	
	/* FS_EXT_1 */
	NULL, NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	isofs_stat64,
	
	0, 0, 0, 0, 0,
	NULL, NULL
};

/*
 * device driver
 */

static long	_cdecl isofs_open		(FILEPTR *f);
static long	_cdecl isofs_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl isofs_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl isofs_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl isofs_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl isofs_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl isofs_close	(FILEPTR *f, int pid);
static long	_cdecl null_select	(FILEPTR *f, long int p, int mode);
static void	_cdecl null_unselect	(FILEPTR *f, long int p, int mode);

static DEVDRV devtab =
{
	isofs_open,
	isofs_write, isofs_read, isofs_lseek, isofs_ioctl, isofs_datime,
	isofs_close,
	null_select, null_unselect
};

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN init & configuration part */

/* the device number we have to deal with
 */
int isofs_dev;

FILESYS * _cdecl init_xfs(struct kerinfo *k)
{
	struct fs_descr d = { &ftab, -1 };
	long r;


	KERNEL = k;


	c_conws(MSG_BOOT);
	c_conws(MSG_GREET);
#ifdef ALPHA
	c_conws(MSG_ALPHA);
#endif
#ifdef BETA
	c_conws(MSG_BETA);
#endif
	c_conws("\r\n");


	KERNEL_DEBUG(("isofs.c: init"));

	/* version check */
	if (MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 15))
	{
		c_conws(MSG_OLDMINT);
		c_conws(MSG_FAILURE);

		return NULL;
	}

	/* check buffer cache version */
	if (bio.version != 3)
	{
		c_conws(MSG_BIOVERSION);
		c_conws(MSG_FAILURE);

		return NULL;		
	}

	/* check for revision 1 features */
	if (bio.revision < 1)	
	{
		c_conws(MSG_BIOREVISION);
		c_conws(MSG_FAILURE);

		return NULL;		
	}

	/* check for native UTC timestamps */
	if (MINT_KVERSION > 0 && KERNEL->xtime)
	{
		/* yeah, save enourmous overhead */
		native_utc = 1;

		KERNEL_DEBUG("isofs: running in native UTC mode!");
	}
	else
	{
		/* disable extension level 3 */
		ftab.fsflags &= ~FS_EXT_3;
	}

	r = d_cntl(FS_INSTALL, "u:\\", (long)&d);
	if (r != (long) kernel)
	{
		c_conws(MSG_FAILURE1("Dcntl(FS_INSTALL) failed"));
		return NULL;
	}

	r = d_cntl(FS_MOUNT, "u:\\cdrom", (long)&d);
	if (r == d.dev_no)
	{
		isofs_dev = d.dev_no;

		KERNEL_DEBUG("isofs: successful, dev = %i", isofs_dev);
		return (FILESYS *) 1L;
	}

	c_conws(MSG_FAILURE1("Dcntl(FS_MOUNT) failed"));

	if (d_cntl (FS_UNINSTALL, "u:\\cdrom", (long)&d))
	{
		/* can't return NULL here because FS_UNINSTALL failed */
		return (FILESYS *) 1;
	}

	return NULL;
}

/* END init & configuration part */
/****************************************************************************/

static long _cdecl
rwabs_metados(DI *di, unsigned short rw, void *buf, unsigned long size, unsigned long rec)
{
	unsigned long n;
	unsigned long recno;
	long r;
	
	n = size >> di->pshift;
	recno = rec << di->lshift;

	if (!n)
	{
		ALERT(("rwabs_metados[%c]: n = 0 (failure)!", di->drv+'A'));
		return ESECTOR;
	}

	if (di->size && (recno + n) > di->size)
	{
		ALERT(("rwabs_metados[%c]: access outside partition", di->drv+'A'));
		return ESECTOR;
	}

	recno += di->start;

	while (n > 63)
	{
		r = Metaread(di->major, buf, recno, 63);
		DEBUG(("rwabs_metados: Metaread(%i,%lu,63) -> %li",
			di->major, recno, r));
		if (r) return r;

		recno += 63;
		n -= 63;
		buf = (char *)buf + (63 << di->pshift);
	}

	r = Metaread(di->major, buf, recno, n);
	DEBUG(("rwabs_metados: Metaread(%i,%lu,%li) -> %li",
		di->major, recno, n, r));

	return r;
}

static long _cdecl
dskchng_metados(DI *di)
{
	return 0;
}

static DI *di = NULL;

static long
init_metados(void)
{
	struct meta_drvinfo drvinfo;
	long r;

	/* XXX lookup metados drives */
	r = Metaopen('A', &drvinfo);
	if (r) return r;

	DEBUG(("init_metados: using \"%s\"", drvinfo.mdr_name));

	/* XXX lookup free gemdos drive */
	di = bio.res_di('Z'-'A');
	if (!di) return EBUSY;

	*di->rrwabs = rwabs_metados;
	*di->rdskchng = dskchng_metados;

	di->major = 'A';
	di->minor = 0;
	di->start = 0;
	di->size = 0;

	bio.set_pshift(di, ISO_DEFAULT_BLOCK_SIZE);
	bio.set_lshift(di, ISO_DEFAULT_BLOCK_SIZE);

	return 0;
}

static void
exit_metados(void)
{
	Metaclose('A');

	if (di)
	{
		bio.free_di(di);
		di = NULL;
	}
}

static long
iso_makemp(struct iso_super *super, UNIT *u, long *ea_len)
{
	struct iso_primary_descriptor *pri;
	struct iso_directory_record *rootp;
	long logical_block_size;

	pri = (struct iso_primary_descriptor *)u->data;

	logical_block_size = isonum_723(pri->logical_block_size);
	if (logical_block_size < ISO_DEFAULT_BLOCK_SIZE)
	{
		DEBUG(("isofs: logical_block_size %li not supported", logical_block_size));
		return EMEDIUMTYPE;
	}

	rootp = (struct iso_directory_record *)pri->root_directory_record;

	super->logical_block_size = logical_block_size;
	super->volume_space_size = isonum_733(pri->volume_space_size);
	memcpy(super->root, rootp, sizeof(super->root));
	super->root_extent = isonum_733(rootp->extent);
	super->root_size = isonum_733(rootp->size);
	super->im_joliet_level = 0;

	super->im_bmask = logical_block_size - 1;
	super->im_bshift = 0;
	while ((1 << super->im_bshift) < super->logical_block_size)
		super->im_bshift++;

	if (ea_len != NULL)
		*ea_len = isonum_711(rootp->ext_attr_length);

	return 0;
}

static long
iso_mountfs(void)
{
	long r = EINVAL;
	long session;
	long iso_blknum, iso_bsize;

	struct iso_volume_descriptor *vdp;
	struct iso_supplementary_descriptor *sup;
	long ext_attr_length;
	short joliet_level;

	struct iso_super *super = NULL;
	UNIT *u_pri = NULL;
	UNIT *u_sup = NULL;


	iso_bsize = ISO_DEFAULT_BLOCK_SIZE;

	r = init_metados();
	if (r)
	{
		DEBUG(("isofs: init_metados() -> %li", r));
		return r;
	}
	assert(di);

//	r = get_session(&session);
//	if (r)
		session = 0;	/* never mind */
//	else
//		DEBUG(("isofs: multi-session at offset %ld\n", session));

	for (iso_blknum = 16; iso_blknum < 100; iso_blknum++)
	{
		UNIT *u;

		u = bio.get_resident(di, iso_blknum+session, iso_bsize);
		if (!u)
		{
			r = ESECTOR;
			goto out;
		}

		vdp = (struct iso_volume_descriptor *)u->data;

		if (memcmp(vdp->id, ISO_STANDARD_ID, sizeof(vdp->id)) != 0)
		{
			r = EINVAL;
			goto out;
		}

		switch (isonum_711(vdp->type))
		{
			case ISO_VD_PRIMARY:
			{
				if (u_pri == NULL)
				{
					u_pri = u;
					u = NULL;
				}
				break;
			}

			case ISO_VD_SUPPLEMENTARY:
			{
				if (u_sup == NULL)
				{
					u_sup = u;
					u = NULL;
				}
				break;
			}

			default:
				break;
		}

		if (isonum_711(vdp->type) == ISO_VD_END)
		{
			bio.rel_resident(u);
			u = NULL;
			break;
		}

		if (u != NULL)
		{
			bio.rel_resident(u);
			u = NULL;
		}
	}

	if (u_pri == NULL)
	{
		r = EINVAL;
		goto out;
	}

	super = kmalloc(sizeof(*super));
	bzero(super, sizeof(*super));

	r = iso_makemp(super, u_pri, &ext_attr_length);
	if (r)
	{
		r = EINVAL;
		goto out;
	}

	super->volume_space_size += session;

	bio.rel_resident(u_pri);
	u_pri = NULL;

#if 1
	/* Check the Rock Ridge Extension support */
	{
		struct iso_directory_record *rootp;
		UNIT *u;

		u = bio.get_resident(di,
				     super->root_extent + ext_attr_length,
				     super->logical_block_size);
		if (!u)
		{
			r = ESECTOR;
			goto out;
		}

		rootp = (struct iso_directory_record *)u->data;
		super->rr_skip = cd9660_rrip_offset(rootp, super);

		bio.rel_resident(u);
	}

	/* Check the Joliet Extension support */
	if (u_sup)
	{
		sup = (struct iso_supplementary_descriptor *)u_sup->data;
		joliet_level = 0;

		if ((isonum_711(sup->flags) & 1) == 0)
		{
			if (memcmp(sup->escape, "%/@", 3) == 0)
				joliet_level = 1;
			if (memcmp(sup->escape, "%/C", 3) == 0)
				joliet_level = 2;
			if (memcmp(sup->escape, "%/E", 3) == 0)
				joliet_level = 3;
		}

		if (joliet_level != 0)
		{
			if (iso_makemp(super, u_sup, NULL) == -1)
			{
				r = EINVAL;
				goto out;
			}

			super->im_joliet_level = joliet_level;
		}
	}
#endif

	if (u_sup)
	{
		bio.rel_resident(u_sup);
		u_sup = NULL;
	}

	DEBUG(("isofs: im_flags            0x%x", super->im_flags));
	DEBUG(("isofs: im_joliet_level     %i",   super->im_joliet_level));
	DEBUG(("isofs: logical_block_size  %li",  super->logical_block_size));
	DEBUG(("isofs: im_bshift           %li",  super->im_bshift));
	DEBUG(("isofs: im_bmask            %li",  super->im_bmask));
	DEBUG(("isofs: volume_space_size   %li",  super->volume_space_size));
	DEBUG(("isofs: root_extent         %li",  super->root_extent));
	DEBUG(("isofs: root_size           %li",  super->root_size));
	DEBUG(("isofs: iso_type            %i",   super->iso_type));
	DEBUG(("isofs: rr_skip             %li",  super->rr_skip));
	DEBUG(("isofs: rr_skip0            %li",  super->rr_skip0));

//	return 0;

out:
	if (u_pri) bio.rel_resident(u_pri);
	if (u_sup) bio.rel_resident(u_sup);

	if (super)
		kfree(super, sizeof(*super));

	exit_metados();
	return r;
}

/****************************************************************************/
/* BEGIN filesystem */

static long _cdecl
isofs_root(int drv, fcookie *fc)
{
//	SI *s = super [drv];
	long r;
	
	DEBUG(("isofs[%c]: isofs_root enter (mem = %li)", drv+'A', memory));
	
	if (drv != isofs_dev)
	{
		DEBUG (("isofs_root(%d) -> ENXIO", drv));
		return ENXIO;
	}
	
	r = iso_mountfs();
	
	DEBUG(("iso_mountfs() -> %li", r));
	return ENXIO;
	
//	if (!s)
	{
		long i = EMEDIUMTYPE;
		
//		i = read_sb_info(drv);
		if (i)
		{
			DEBUG(("isofs[%c]: e_root leave failure", drv+'A'));
			return i;
		}
		
//		s = super [drv];
	}
	
	fc->fs = &ftab;
	fc->dev = drv;
	fc->aux = 0;
//	fc->index = (long) s->root; s->root->links++;
	
	DEBUG(("isofs[%c]: e_root leave ok (mem = %li)", drv+'A', memory));
	return E_OK;
}

static long _cdecl
isofs_lookup(fcookie *dir, const char *name, fcookie *fc)
{
//	COOKIE *c = (COOKIE *) dir->index;
//	SI *s = super [dir->dev];
	
	DEBUG(("isofs[%c]: isofs_lookup (%s)", dir->dev+'A', name));
	
	*fc = *dir;
	
	/* 1 - itself */
	if (!*name || (name [0] == '.' && name [1] == '\0'))
	{	
//		c->links++;
	
		DEBUG(("isofs[%c]: isofs_lookup: leave ok, (name = \".\")", dir->dev+'A'));
		return E_OK;
	}
	
	/* 2 - parent dir */
	if (name [0] == '.' && name [1] == '.' && name [2] == '\0')
	{
//		if (dir == rootcookie)
//		{
//			DEBUG(("isofs[%c]: isofs_lookup: leave ok, EMOUNT, (name = \"..\")", dir->dev+'A'));
//			return EMOUNT;
//		}
	}
	
	/* 3 - normal entry */
	{
//		if (not found)
			return ENOENT;
	}
	
	DEBUG(("isofs[%c]: isofs_lookup: leave ok", dir->dev+'A'));
	return E_OK;
}

static DEVDRV * _cdecl
isofs_getdev(fcookie *fc, long *devsp)
{
	if (fc->fs == &ftab)
		return &devtab;
	
	*devsp = ENOSYS;
	return NULL;
}

static long _cdecl
isofs_getxattr(fcookie *fc, XATTR *xattr)
{
	return ENOSYS;
	
# if 0
	xattr->mode			= 
	xattr->index			= 
	xattr->dev			= 
	xattr->rdev 			= 
	xattr->nlink			= 
	xattr->uid			= 
	xattr->gid			= 
	xattr->size 			= 
	xattr->blksize			= 
	xattr->nblocks			= /* number of blocks of size 'blksize' */
	*((long *) &(xattr->mtime))	= 
	*((long *) &(xattr->atime))	= 
	*((long *) &(xattr->ctime))	= 
# endif
	
	/* fake attr field a little bit */
	if (S_ISDIR(xattr->mode))
	{
		xattr->attr = FA_DIR;
	}
	else
		xattr->attr = (xattr->mode & 0222) ? 0 : FA_RDONLY;
	
	return E_OK;
}

static long _cdecl
isofs_stat64(fcookie *fc, STAT *stat)
{
	/* later */
	return ENOSYS;
}

static long _cdecl
isofs_chattr(fcookie *fc, int attrib)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_chown(fcookie *fc, int uid, int gid)
{
	/* nothing todo */
	return ENOSYS;
}

static long _cdecl
isofs_chmode(fcookie *fc, unsigned mode)
{
	/* nothing todo */
	return ENOSYS;
}

static long _cdecl
isofs_mkdir(fcookie *dir, const char *name, unsigned mode)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_rmdir(fcookie *dir, const char *name)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_creat(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_remove(fcookie *dir, const char *name)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_getname(fcookie *root, fcookie *dir, char *pathname, int size)
{
	DEBUG(("isofs_getname enter"));
	
	pathname [0] = '\0';
	
	{
		;
	}
	
	pathname [0] = '\0';
	
	DEBUG(("isofs_getname: path not found?"));
	return ENOTDIR;
}

static long _cdecl
isofs_rename(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_opendir(DIR *dirh, int flags)
{
//	if (!S_ISDIR(...))
	{
		DEBUG(("isofs_opendir: dir not a DIR!"));
		return EACCES;
	}
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
isofs_readdir(DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
	long ret = ENMFILES;
	
	{
		;
	}
	
	return ret;
}

static long _cdecl
isofs_rewinddir(DIR *dirh)
{
	{
		;
	}
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
isofs_closedir(DIR *dirh)
{
	{
		;
	}
		
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
isofs_pathconf(fcookie *dir, int which)
{
	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return UNLIMITED;
		case DP_PATHMAX:	return UNLIMITED;
		case DP_NAMEMAX:	return 255;		/* correct me */
		case DP_ATOMIC:		return 1024;		/* correct me */
		case DP_TRUNC:		return DP_NOTRUNC;
		case DP_CASE:		return DP_CASEINSENS;	/* correct me */
		case DP_MODEATTR:	return (DP_ATTRBITS	/* correct me */
						| DP_MODEBITS
						| DP_FT_DIR
						| DP_FT_REG
						| DP_FT_LNK
					);
		case DP_XATTRFIELDS:	return (DP_INDEX	/* correct me */
						| DP_DEV
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
		case DP_VOLNAMEMAX:	return 255;		/* correct me */
	}
	
	return ENOSYS;
}

static long _cdecl
isofs_dfree(fcookie *dir, long *buf)
{
	DEBUG(("isofs_dfree called"));
	
	*buf++	= 0;	/* free cluster */
	*buf++	= 0;	/* cluster count */
	*buf++	= 2048;	/* sectorsize */
	*buf	= 1;	/* nr of sectors per cluster */
	
	return E_OK;
}

static long _cdecl
isofs_writelabel(fcookie *dir, const char *name)
{
	/* nothing todo */
	return EACCES;
}

static long _cdecl
isofs_readlabel(fcookie *dir, char *name, int namelen)
{
	/* cosmetical */
	
	{
		;
	}
	
	return EBADARG;
}

static long _cdecl
isofs_symlink(fcookie *dir, const char *name, const char *to)
{
	/* nothing todo */
	return ENOSYS;
}

static long _cdecl
isofs_readlink(fcookie *file, char *buf, int len)
{
	long ret = ENOSYS;
	
//	if (S_ISLNK(...))
	{
		;
	}
	
	return ret;
}

static long _cdecl
isofs_hardlink(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	/* nothing todo */
	return ENOSYS;
}

static long _cdecl
isofs_fscntl(fcookie *dir, const char *name, int cmd, long arg)
{
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy((char *) arg, "isofs");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info = (struct fs_info *) arg;
			if (info)
			{
				strcpy(info->name, "isofs-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
				// XXX
				info->type = FS_ISO9660;
				strcpy(info->type_asc, "isofs");
				
				/* more types later */
			}
			return E_OK;
		}
		case FS_USAGE:
		{
			struct fs_usage *usage = (struct fs_usage *) arg;
			if (usage)
			{
# if 0
				usage->blocksize = ;
				usage->blocks = ;
				usage->free_blocks = ;
				usage->inodes = FS_UNLIMITED;
				usage->free_inodes = FS_UNLIMITED;
# endif
			}
			return E_OK;
		}
	}
	
	return ENOSYS;
}

static long _cdecl
isofs_dskchng(int drv, int mode)
{
	return 0;
}

static long _cdecl
isofs_release(fcookie *fc)
{
	/* this function decrease the inode reference counter
	 * if reached 0 this inode is no longer used by the kernel
	 */
//	COOKIE *c = (COOKIE *) fc->index;
	
//	c->links--;
	return E_OK;
}

static long _cdecl
isofs_dupcookie(fcookie *dst, fcookie *src)
{
	/* this function increase the inode reference counter
	 * kernel use this to create a new reference to an inode
	 * and to verify that the inode remain valid until it is
	 * released
	 */
//	COOKIE *c = (COOKIE *) src->index;
	
//	c->links++;
	*dst = *src;
	
	return E_OK;
}

static long _cdecl
isofs_sync(void)
{
	return E_OK;
}

/* END filesystem */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver */

static long _cdecl
isofs_open(FILEPTR *f)
{
//	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG(("isofs_open: enter"));
	
//	if (!S_ISREG(...))
	{
		DEBUG(("isofs_open: leave failure, not a valid file"));
		return EACCES;
	}
	
	if (((f->flags & O_RWMODE) == O_WRONLY)
		|| ((f->flags & O_RWMODE) == O_RDWR))
	{
		return EROFS;
	}
	
//	if (c->open && denyshare(c->open, f))
//	{
//		DEBUG(("isofs_open: file sharing denied"));
//		return EACCES;
//	}
	
	f->pos = 0;
	f->devinfo = 0;
//	f->next = c->open;
//	c->open = f;
	
//	c->links++;
	
	DEBUG(("isofs_open: leave ok"));
	return E_OK;
}

static long _cdecl
isofs_write(FILEPTR *f, const char *buf, long bytes)
{
	/* nothing todo */
	return 0;
}

static long _cdecl
isofs_read(FILEPTR *f, char *buf, long bytes)
{
	return 0;
}

static long _cdecl
isofs_lseek(FILEPTR *f, long where, int whence)
{
//	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG(("isofs_lseek: enter (where = %li, whence = %i)", where, whence));
	
	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
//		case SEEK_END:	where += c->stat.size;	break;
		default:	return EINVAL;
	}
	
//	if (where < 0)
	{
		DEBUG(("isofs_lseek: leave failure EBADARG (where = %li)", where));
		return EBADARG;
	}
	
	f->pos = where;
	
	DEBUG(("isofs_lseek: leave ok (f->pos = %li)", f->pos));
	return where;
}

static long _cdecl
isofs_ioctl(FILEPTR *f, int mode, void *buf)
{
//	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG(("isofs_ioctl: enter (mode = %i)", mode));
	
	switch (mode)
	{
		case FIONREAD:
		{
			*(long *) buf = 0; //c->stat.size - f->pos;
			return E_OK;
		}
		case FIONWRITE:
		{
			*(long *) buf = 0;
			return E_OK;
		}
		case FIOEXCEPT:
		{
			*(long *) buf = 0;
			return E_OK;
		}
# if 0
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			struct flock *fl = (struct flock *) buf;
			
			LOCK t;
			LOCK *lck;
			
			int cpid;
				
			t.l = *fl;
			
			switch (t.l.l_whence)
			{
				case SEEK_SET:
				{
					break;
				}
				case SEEK_CUR:
				{
					long r = isofs_lseek(f, 0L, SEEK_CUR);
					t.l.l_start += r;
					break;
				}
				case SEEK_END:
				{
					long r = isofs_lseek(f, 0L, SEEK_CUR);
					t.l.l_start = isofs_lseek(f, t.l.l_start, SEEK_END);
					(void) isofs_lseek(f, r, SEEK_SET);
					break;
				}
				default:
				{
					DEBUG(("isofs_ioctl: invalid value for l_whence\n"));
					return ENOSYS;
				}
			}
			
			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;
			
			cpid = p_getpid();
			
			if (mode == F_GETLK)
			{
				lck = denylock(cpid, c->locks, &t);
				if (lck)
					*fl = lck->l;
				else
					fl->l_type = F_UNLCK;
				
				return E_OK;
			}
			
			if (t.l.l_type == F_UNLCK)
			{
				/* try to find the lock */
				LOCK **lckptr = &(c->locks);
				
				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
		                                && ((lck->l.l_start == t.l.l_start
						     && lck->l.l_len == t.l.l_len) ||
						    (lck->l.l_start >= t.l.l_start
						     && t.l.l_len == 0)))
					{
						/* found it -- remove the lock */
						*lckptr = lck->next;
						
						DEBUG(("isofs_ioctl: unlocked %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
						
						/* wake up anyone waiting on the lock */
						wake(IO_Q, (long) lck);
						kfree(lck, sizeof(*lck));
						
						return E_OK;
					}
					
					lckptr = &(lck->next);
					lck = lck->next;
				}
				
				return ENSLOCK;
			}
			
			DEBUG(("isofs_ioctl: lock %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
			
			/* see if there's a conflicting lock */
			while ((lck = denylock(cpid, c->locks, &t)) != 0)
			{
				DEBUG(("isofs_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
				if (mode == F_SETLKW)
				{
					/* sleep a while */
					sleep(IO_Q, (long) lck);
				}
				else
					return ELOCKED;
			}
			
			/* if not, add this lock to the list */
			lck = kmalloc(sizeof(*lck));
			if (!lck)
			{
				ALERT(("isofs: kmalloc fail in: isofs_ioctl(%lx)", c));
				return ENOMEM;
			}
			
			lck->l = t.l;
			lck->l.l_pid = cpid;
			
			lck->next = c->locks;
			c->locks = lck;
			
			/* mark the file as being locked */
			f->flags |= O_LOCK;
			return E_OK;
		}
# endif
	}
	
	return ENOSYS;
}

static long _cdecl
isofs_datime(FILEPTR *f, ushort *time, int flag)
{
//	COOKIE *c = (COOKIE *) f->fc.index;
	
	switch (flag)
	{
		case 0:
//			*(ulong *) time = c->stat.mtime.time;
			break;
		
		case 1:
			return EROFS;
		
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl
isofs_close(FILEPTR *f, int pid)
{
//	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG(("isofs_close: enter (f->links = %i)", f->links));
	
# if 0
	/* if a lock was made, remove any locks of the process */
	if (f->flags & O_LOCK)
	{
		LOCK *lock;
		LOCK **oldlock;
		
		DEBUG(("fnramfs: isofs_close: remove lock (pid = %i)", pid));
		
		oldlock = &c->locks;
		lock = *oldlock;
		
		while (lock)
		{
			if (lock->l.l_pid == pid)
			{
				*oldlock = lock->next;
				/* (void) isofs_lock((int) f->devinfo, 1, lock->l.l_start, lock->l.l_len); */
				wake(IO_Q, (long) lock);
				kfree(lock, sizeof(*lock));
			}
			else
			{
				oldlock = &lock->next;
			}
			
			lock = *oldlock;
		}
	}
	
	if (f->links <= 0)
	{
		/* remove the FILEPTR from the linked list */
		register FILEPTR **temp = &c->open;
		register long flag = 1;
		
		while (*temp)
		{
			if (*temp == f)
			{
				*temp = f->next;
				f->next = NULL;
				flag = 0;
				break;
			}
			temp = &(*temp)->next;
		}
		
		if (flag)
		{
			ALERT(("isofs_close: remove open FILEPTR failed"));
		}
		
		c->links--;
	}
# endif
	
	DEBUG(("isofs_close: leave ok"));
	return E_OK;
}

static long _cdecl
null_select(FILEPTR *f, long int p, int mode)
{
	if ((mode == O_RDONLY) || (mode == O_WRONLY))
		/* we're always ready to read/write */
		return 1;
	
	/* other things we don't care about */
	return E_OK;
}

static void _cdecl
null_unselect(FILEPTR *f, long int p, int mode)
{
}

/* END device driver */
/****************************************************************************/
