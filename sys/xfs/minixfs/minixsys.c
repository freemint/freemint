/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "minixsys.h"
# include "minixdev.h"
# include "version.h"

# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "bitmap.h"
# include "dir.h"
# include "inode.h"
# include "main.h"
# include "misc.h"
# include "trans.h"
# include "zone.h"


static void	set_atime	(fcookie *fc);


static long	_cdecl m_root		(int dev, fcookie *dir);

static long	_cdecl m_lookup		(fcookie *dir, const char *name, fcookie *entry);
static DEVDRV *	_cdecl m_getdev		(fcookie *fc, long *devsp);
static long	_cdecl m_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl m_stat64		(fcookie *fc, STAT *stat);

static long	_cdecl m_chattr		(fcookie *file, int attr);
static long	_cdecl m_chown		(fcookie *file, int uid, int gid);
static long	_cdecl m_chmode		(fcookie *file, unsigned mode);

static long	_cdecl m_mkdir		(fcookie *dir, const char *name, unsigned mode);
static long	_cdecl m_rmdir		(fcookie *dir, const char *name);
static long	_cdecl m_creat		(fcookie *dir, const char *name, unsigned mode, int attr, fcookie *entry);
static long	_cdecl m_remove		(fcookie *dir, const char *name);
static long	_cdecl m_getname	(fcookie *root, fcookie *dir, char *pathname, int length);
static long	_cdecl m_rename		(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long	_cdecl m_opendir	(DIR *dirh, int flag);
static long	_cdecl m_readdir	(DIR *dirh, char *name, int namelen, fcookie *fc);
static long	_cdecl m_rewinddir	(DIR *dirh);
static long	_cdecl m_closedir	(DIR *dirh);

static long	_cdecl m_pathconf	(fcookie *dir, int which);
static long	_cdecl m_dfree		(fcookie *dir, long *buffer);
static long	_cdecl m_wlabel		(fcookie *dir, const char *name);
static long	_cdecl m_rlabel		(fcookie *dir, char *name, int namelen);

static long	_cdecl m_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl m_readlink	(fcookie *file, char *buf, int len);
static long	_cdecl m_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl m_fscntl		(fcookie *dir, const char *name, int cmd , long arg);
static long	_cdecl m_dskchng	(int d, int mode);

static long	_cdecl m_release	(fcookie *fc);
static long	_cdecl m_dupcookie	(fcookie *dest, fcookie *src);
static long	_cdecl m_sync		(void);

static long	_cdecl m_mknod		(fcookie *dir, const char *name, ulong mode);
static long	_cdecl m_unmount	(int drv);


FILESYS minix_filesys =
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
	FS_DO_SYNC		|
	FS_OWN_MEDIACHANGE	|
	FS_EXT_1		|
	FS_EXT_2		|
	FS_EXT_3		,
	
	root:			m_root,
	lookup:			m_lookup,
	creat:			m_creat,
	getdev:			m_getdev,
	getxattr:		m_getxattr,
	chattr:			m_chattr,
	chown:			m_chown,
	chmode:			m_chmode,
	mkdir:			m_mkdir,
	rmdir:			m_rmdir,
	remove:			m_remove,
	getname:		m_getname,
	rename:			m_rename,
	opendir:		m_opendir,
	readdir:		m_readdir,
	rewinddir:		m_rewinddir,
	closedir:		m_closedir,
	pathconf:		m_pathconf,
	dfree:			m_dfree,
	writelabel:		m_wlabel,
	readlabel:		m_rlabel,
	symlink:		m_symlink,
	readlink:		m_readlink,
	hardlink:		m_hardlink,
	fscntl:			m_fscntl,
	dskchng:		m_dskchng,
	release:		m_release,
	dupcookie:		m_dupcookie,
	sync:			m_sync,
	
	/* FS_EXT_1 */
	mknod:			m_mknod,
	unmount:		m_unmount,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	stat64:			m_stat64,
	res1:			0, 
	res2:			0,
	res3:			0,
	
	lock: 0, sleepers: 0,
	block: NULL, deblock: NULL
};

static short restore_dev = -1;

/* Set the atime of a V2 inode for filesystems. There is a snag here: if the
 * disk is changed then this is likely not to be written out before the whole
 * cache is invalidated. So we set the status to '3' which means that it is
 * not alerted if this is dirty when invalidated (hardly the end of the world
 * if the atime is slightly wrong!)
 */

static void
set_atime (fcookie *fc)
{
	d_inode rip;
	
	if (fc->dev > 1)
	{
		read_inode (fc->index, &rip, fc->dev);
		rip.i_atime = CURRENT_TIME;
		write_inode (fc->index, &rip, fc->dev);
	}
}

void
sync_bitmaps (register SI *psblk)
{
	register void *buf = NULL;
	register long start = 0;
	register long size = 0;
	
	if (psblk->idirty)
	{
		buf = psblk->ibitmap;
		start = 2;
		size = psblk->sblk->s_imap_blks;
		
		if (psblk->zdirty)
			size += psblk->sblk->s_zmap_blks;
	}
	else if (psblk->zdirty)
	{
		buf = psblk->zbitmap;
		start = psblk->sblk->s_imap_blks + 2;
		size = psblk->sblk->s_zmap_blks;
	}
	
	if (start)
	{
		size *= BLOCK_SIZE;
		BIO_RWABS (psblk->di, 3, buf, size, start);
	}
	
	psblk->idirty = 0;
	psblk->zdirty = 0;
}

/*
 * Note: in the first round of initialisations, we assume that floppy
 * drives (A and B) don't belong to us; but in a later disk change,
 * they may very well be ours, so we remember that. This is means that a
 * minix disk inserted into a drive will be unrecognisable at boot up and
 * a forced disk change is needed. However for MiNT 1.05 (and presumably
 * later) drives are initialised on first access so this isn't needed.
 */

static long _cdecl
m_root (int dev, fcookie *dir)
{
	SI **psblk = super_ptr + dev;
	
	DEBUG (("Minix-FS (%c): m_root enter", DriveToLetter(dev)));
	
	/* If not present, see if it's valid */
	if (!*psblk)
	{
		long i = minix_sanity (dev);
		if (i)
		{
			DEBUG (("Minix-FS (%c): m_root leave %li", DriveToLetter(dev), i));
			return i;
		}
		
		/* default: enable writeback mode */
		(void) bio.config (dev, BIO_WB, ENABLE);
	}
	
	if (*psblk)
	{
		dir->fs = &minix_filesys;
		
		/* Aux field tells original device */
		dir->aux = dev | AUX_DRV ;
		dir->index = ROOT_INODE;
		
		dir->dev = dev;
		
		DEBUG (("Minix-FS (%c): m_root leave E_OK", DriveToLetter(dev)));
		return E_OK;
	}
	
	DEBUG (("Minix-FS (%c): m_root leave ENXIO", DriveToLetter(dev)));
	return ENXIO;
}

static long _cdecl
m_lookup (fcookie *dir, const char *name, fcookie *fc)
{

	if (!*name)
	{
		*fc = *dir;
		fc->aux = 0;
		return 0;
	}

	if (dir->index == ROOT_INODE && !strcmp (name, ".."))
	{
		*fc = *dir;
		
		fc->index = search_dir (name, fc->index, fc->dev, FIND);
		if (fc->index < 0)
			return fc->index;
		
		fc->aux = 0;
		return 0;
	}

	fc->index = search_dir (name, dir->index, dir->dev, FIND);
	fc->dev = dir->dev;
	if (fc->index < 0)
		return fc->index;
	
	fc->aux = 0;
	fc->fs = &minix_filesys;
	
	return E_OK;
}

static DEVDRV * _cdecl
m_getdev (fcookie *fc, long int *devsp)
{
	if (fc->fs == &minix_filesys)
		return &minix_dev;
	
	*devsp = ENOSYS;
	
	DEBUG (("MinixFS: m_getdev: leave failure"));
	return NULL;
}

static long _cdecl
m_getxattr (fcookie *fc, XATTR *xattr)
{
	SI *psblk = super_ptr[fc->dev];
	d_inode rip;
	long nblocks;
	
	read_inode (fc->index, &rip, fc->dev);
	
	/* Minix and gcc use different values for FIFO's */
	if ((rip.i_mode & I_TYPE) == I_NAMED_PIPE)
		xattr->mode = S_IFIFO | (rip.i_mode & ALL_MODES);
	else
		xattr->mode = rip.i_mode;
	
	/* We could potentially have trouble with symlinks too */
# if I_SYMLINK != S_IFLNK
	if ((rip.i_mode & I_TYPE) == I_SYMLINK)
		xattr->mode = S_IFLNK | (rip.i_mode & ALL_MODES);
# endif

	/* Fake attr field a bit , to keep TOS happy */
	if (IS_DIR (rip)) xattr->attr = FA_DIR;
	else xattr->attr = (rip.i_mode & 0222) ? 0 : FA_RDONLY;
	
        xattr->index = fc->index;
        xattr->dev = fc->dev;
	
	/* Char and block special files need major/minor device nos filled in */
	if (IM_SPEC (rip.i_mode)) xattr->rdev = rip.i_zone[0];
	else xattr->rdev = 0;
	
        xattr->nlink = rip.i_nlinks;
        xattr->uid = rip.i_uid;
        xattr->gid = rip.i_gid;
        xattr->size = rip.i_size;
	xattr->blksize = BLOCK_SIZE;
	
	/* Note: the nblocks calculation is accurate only if the file is
	 * contiguous. It usually will be, and if it's not, it shouldn't
	 * matter ('du' will return values that are slightly too high)
	 */
	nblocks = (xattr->size + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
	xattr->nblocks = nblocks;
	
	if (xattr->nblocks > psblk->dzpi)
	{
		/* correct for the indirection block */
		nblocks++;
	}
	
	if (xattr->nblocks > psblk->ndbl)
	{
		/* correct for double indirection block */
		nblocks++;
		
		/* and single indirection blocks */
		nblocks += (xattr->nblocks - psblk->ndbl) / psblk->zpind;
	}
	
	if (xattr->nblocks > psblk->ndbl + (long) psblk->zpind * psblk->zpind)
	{
		/* correct for triple indir block */
		nblocks++;
		
		/* and double indirection blocks */
		nblocks += ((xattr->nblocks - psblk->ndbl
			- (long) psblk->zpind * psblk->zpind)
			/ ((long) psblk->zpind * psblk->zpind));
	}
	
	/* measured in blksize (BLOCK_SIZE) byte blocks */
	xattr->nblocks = nblocks;
	
	if (native_utc)
	{
		SET_XATTR_TD(xattr,m,rip.i_mtime);
		SET_XATTR_TD(xattr,a,rip.i_atime);
		SET_XATTR_TD(xattr,c,rip.i_ctime);
#if 0
		*((long *) &(xattr->mtime)) = rip.i_mtime;
		*((long *) &(xattr->atime)) = rip.i_atime;
		*((long *) &(xattr->ctime)) = rip.i_ctime;
#endif
	}
	else
	{
		SET_XATTR_TD(xattr,m,dostime(rip.i_mtime));
		SET_XATTR_TD(xattr,a,dostime(rip.i_atime));
		SET_XATTR_TD(xattr,c,dostime(rip.i_ctime));
#if 0
		*((long *) &(xattr->mtime)) = dostime (rip.i_mtime);
		*((long *) &(xattr->atime)) = dostime (rip.i_atime);
		*((long *) &(xattr->ctime)) = dostime (rip.i_ctime);
#endif
	}
	
	xattr->reserved2 = 0;
	xattr->reserved3[0] = 0;
	xattr->reserved3[1] = 0;
	
	return E_OK;
}

static long _cdecl
m_stat64 (fcookie *fc, STAT *ptr)
{
	SI *psblk = super_ptr[fc->dev];
	d_inode rip;
	long nblocks;
	
	read_inode (fc->index, &rip, fc->dev);
	
	/* Minix and gcc use different values for FIFO's */
	if ((rip.i_mode & I_TYPE) == I_NAMED_PIPE)
		ptr->mode = S_IFIFO | (rip.i_mode & ALL_MODES);
	else
		ptr->mode = rip.i_mode;
	
	/* We could potentially have trouble with symlinks too */
# if I_SYMLINK != S_IFLNK
	if ((rip.i_mode & I_TYPE) == I_SYMLINK)
		ptr->mode = S_IFLNK | (rip.i_mode & ALL_MODES);
# endif
	
	ptr->dev = fc->dev;
	ptr->ino = fc->index;
	
	/* Char and block special files need major/minor device nos filled in */
	if (IM_SPEC (rip.i_mode)) ptr->rdev = rip.i_zone[0];
	else ptr->rdev = 0;
	
        ptr->nlink = rip.i_nlinks;
        ptr->uid = rip.i_uid;
        ptr->gid = rip.i_gid;
        ptr->size = rip.i_size;
	ptr->blksize = BLOCK_SIZE;
	
	/* Note: the nblocks calculation is accurate only if the file is
	 * contiguous. It usually will be, and if it's not, it shouldn't
	 * matter ('du' will return values that are slightly too high)
	 */
	nblocks = (ptr->size + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
	ptr->blocks = nblocks;
	
	if (ptr->blocks > psblk->dzpi)
	{
		/* correct for the indirection block */
		nblocks++;
	}
	
	if (ptr->blocks > psblk->ndbl)
	{
		/* correct for double indirection block */
		nblocks++;
		
		/* and single indirection blocks */
		nblocks += (ptr->blocks - psblk->ndbl) / psblk->zpind;
	}
	
	if (ptr->blocks > psblk->ndbl + (long) psblk->zpind * psblk->zpind)
	{
		/* correct for triple indir block */
		nblocks++;
		
		/* and double indirection blocks */
		nblocks += ((ptr->blocks - psblk->ndbl
			- (long) psblk->zpind * psblk->zpind)
			/ ((long) psblk->zpind * psblk->zpind));
	}
	
	/* measured in 512 byte blocks */
	ptr->blocks = nblocks << (L_BS - 9);
	
	ptr->atime.high_time	= 0;
	ptr->atime.time		= rip.i_atime;
	ptr->atime.nanoseconds	= 0;
	
	ptr->mtime.high_time	= 0;
	ptr->mtime.time		= rip.i_mtime;
	ptr->mtime.nanoseconds	= 0;
	
	ptr->ctime.high_time	= 0;
	ptr->ctime.time		= rip.i_ctime;
	ptr->ctime.nanoseconds	= 0;
	
	ptr->flags = 0;
	ptr->gen = 0;
	bzero (ptr->res, sizeof (ptr->res));
	
	return E_OK;
}

/* the only settable attribute is FA_RDONLY; if the bit is set,
 * the mode is changed so that no write permission exists
 */
static long _cdecl
m_chattr (fcookie *file, int attr)
{
        long inum = file->index;
	int drive = file->dev;
	d_inode rip;
	
	if (super_ptr [drive]->s_flags & MS_RDONLY)
		return EROFS;
	
	if ((attr & FA_RDONLY) || (attr == 0))
	{
		read_inode (inum, &rip, drive);
		
		if (attr)
		{
			/* turn off write permission */
			rip.i_mode &= ~(0222);
			
			goto write;
		}
		else if ((rip.i_mode & 0222) == 0)
		{
			/* turn write permission back on */
			rip.i_mode |= ((rip.i_mode & 0444) >> 1);
			
			goto write;
		}
	}
	
	return E_OK;
	
write:
	rip.i_ctime = CURRENT_TIME;
	write_inode (inum, &rip, drive);
	sync (drive);
	
	return E_OK;
}

static long _cdecl
m_chown (fcookie *file, int uid, int gid)
{
	d_inode rip;
	
	if (super_ptr [file->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	read_inode (file->index, &rip, file->dev);
	
 	if (uid != -1) rip.i_uid = uid;
	if (gid != -1) rip.i_gid = gid;
	
	rip.i_ctime = CURRENT_TIME;
	write_inode (file->index, &rip, file->dev);
	
	sync (file->dev);
	return E_OK;
}

static long _cdecl
m_chmode (fcookie *file, unsigned int mode)
{
	d_inode rip;
	
	if (super_ptr [file->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	read_inode (file->index, &rip, file->dev);
	
	rip.i_mode = (rip.i_mode & I_TYPE) | (mode & ALL_MODES);                
	rip.i_ctime = CURRENT_TIME;
	write_inode (file->index, &rip, file->dev);

	sync (file->dev);
	return E_OK;
}

static long _cdecl
m_mkdir (fcookie *dir, const char *name, unsigned int mode)
{
	ushort newdir;
	d_inode rip, ripnew;
	long pos;
	int incr = super_ptr[dir->dev]->incr;
	dir_struct blank[incr * 2];
	
	if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	if ((pos = search_dir (name, dir->index, dir->dev, ADD)) < 0)
		return pos;
	
	read_inode (dir->index, &rip, dir->dev);
	if (rip.i_nlinks >= MINIX2_LINK_MAX)
		return EACCES;
	
	/* Get new inode */
	if (!(newdir = alloc_inode (dir->dev)))
		return EACCES;

	/* Set up inode */
	bzero (&ripnew, sizeof (ripnew));
	ripnew.i_mode = I_DIRECTORY | (mode & 0777);
	ripnew.i_uid = p_geteuid();
	ripnew.i_gid = p_getegid();
	ripnew.i_nlinks = 2;
	ripnew.i_mtime = CURRENT_TIME;
	ripnew.i_ctime = ripnew.i_mtime;
	ripnew.i_atime = ripnew.i_mtime;
	write_inode (newdir, &ripnew, dir->dev);

	/* Set up new directory */
	strcpy (blank[0].d_name, ".");
	blank[0].d_inum = newdir;
	strcpy (blank[incr].d_name, "..");
	blank[incr].d_inum = dir->index;
	
	if (l_write ((unsigned) newdir, -1L, (long)(DIR_ENTRY_SIZE * 2 * incr),
		blank, dir->dev) != (incr * DIR_ENTRY_SIZE * 2))
	{
		ripnew.i_mode = 0;
		ripnew.i_nlinks = 0;
		write_inode (newdir, &ripnew, dir->dev);
		free_inode (dir->dev, newdir);
		
		sync (dir->dev);
		return EACCES;
	}
	
	rip.i_nlinks++;
	write_inode (dir->index, &rip, dir->dev);
	l_write (dir->index, pos, 2L, &newdir, dir->dev);
	
	sync (dir->dev);
	return E_OK;
}

static long _cdecl
m_rmdir (fcookie *dir, const char *name)
{
	bufr temp;
	long chunk, left;
	long inum;
	int i, incr;
	d_inode rip, rip2;
	
	if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	if ((inum = search_dir (name, dir->index, dir->dev, FIND)) < 0)
		return inum;

	read_inode (inum, &rip, dir->dev);
	read_inode (dir->index, &rip2, dir->dev);
	if (!IS_DIR (rip)) return ENOENT;
	incr = super_ptr[dir->dev]->incr;
	
	/* Check if dir is actually empty */
	for (chunk = 0; (left = next_zone (&rip, chunk) / DIR_ENTRY_SIZE); chunk++)
	{
		long ztemp = find_zone (&rip, chunk, dir->dev, 0);
		read_zone (ztemp, &temp, dir->dev);
		for (i = 0; i < left; i += incr)
		   if (temp.bdir[i].d_inum
		       && (temp.bdir[i].d_name[0] != '.'
			   || temp.bdir[i].d_name[1] != 0)
		       && (temp.bdir[i].d_name[0] != '.'
			   || temp.bdir[i].d_name[1] != '.'
			   || temp.bdir[i].d_name[2] != 0))
			return EACCES ;
	}
	if (!inode_busy (inum, dir->dev, 1))
	{
		trunc_inode (&rip, dir->dev, 0L, 0);
		rip.i_mode = 0;
		free_inode (dir->dev, inum);
	}
	rip.i_nlinks = 0;
	write_inode (inum, &rip, dir->dev);
	read_inode (dir->index, &rip, dir->dev);
	rip.i_mtime = CURRENT_TIME;
	rip.i_nlinks--;
	write_inode (dir->index, &rip, dir->dev);
	search_dir (name, dir->index, dir->dev, KILL);

	sync (dir->dev);
	return 0;
}

static long _cdecl
m_creat (fcookie *dir, const char *name, unsigned int mode, int attr, fcookie *entry)
{
	long pos;
	d_inode ripnew;
	ushort newfile;
	char *ext;
	
	if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	/* Create dir entry */	
	if ((pos = search_dir (name, dir->index, dir->dev, ADD)) < 0)
	{	
		return pos;
	}

	/* Get new inode */
	if (!(newfile = alloc_inode (dir->dev)))
	{
		DEBUG (("Minix-FS (%c): m_creat: no free inodes.", DriveToLetter(dir->dev)));
		return EWRITE;
	}
	/* Set up inode */
	bzero (&ripnew, sizeof (d_inode));

	/* If  creating a file with approriate extensions 
	 * automatically give it execute permissions.
	 */
	if (do_trans (AEXEC_TOS, dir->dev) && (ext = strrchr (name, '.')))
	{
		ext++;
		if ( 
		/* Insert your favourite extensions here */
		  !(stricmp(ext,"TTP") && stricmp(ext,"PRG") 
		   && stricmp(ext,"APP") && stricmp(ext,"TOS") 
		   && stricmp(ext,"ACC") && stricmp(ext, "GTP")))
				mode |= 0111;
	}
	ripnew.i_mode = I_REGULAR | mode;
	ripnew.i_uid = p_geteuid();
	ripnew.i_gid = p_getegid();
	ripnew.i_nlinks = 1;

	ripnew.i_mtime = CURRENT_TIME;
	ripnew.i_atime = ripnew.i_mtime;
	ripnew.i_ctime = ripnew.i_mtime;

	write_inode (newfile, &ripnew, dir->dev);
	l_write (dir->index, pos, 2L, &newfile, dir->dev);

	entry->fs = dir->fs;
	entry->dev = dir->dev;
	entry->index = newfile;
	entry->aux = 0;
	
	sync (dir->dev);
	return 0;
}

/*
 * Unix-like unlink ... handle regulars, symlinks and specials.
 */

static long _cdecl
m_remove (fcookie *dir, const char *name)
{
	long inum, ret;
	char spec;	/* Special file */
	d_inode rip;
	
	if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	inum = search_dir (name, dir->index, dir->dev, FIND);
	if (inum < 0)
		return inum;
	
	read_inode (inum, &rip, dir->dev);
	if (!IS_REG (rip) && !IS_SYM (rip)) 
	{
		if (!IM_SPEC (rip.i_mode)) return EACCES;
		spec = 1;
	}
	else spec = 0;
	
	if ((ret = search_dir (name, dir->index, dir->dev, KILL)) < 0)
		return ret;
	
	if (--rip.i_nlinks == 0)
	{
		if(spec || !inode_busy (inum, dir->dev, 1)) /* Is inode busy ? */
		{
			if (!spec) trunc_inode (&rip, dir->dev, 0L, 0);
			rip.i_mode = 0;
			free_inode (dir->dev, inum);
		}
	}
	write_inode (inum, &rip, dir->dev);
	
	sync (dir->dev);
	return(0);
}

/* This function is inefficient, it uses the standard sys V method of 
 * finding out the pathname of the cwd : for each part of the path, search
 * the parent for a link with the same inode number as '..' , append this to the
 * path until we get to root dir, then reverse order of dirs. This way no 
 * temporary buffers are allocated which could overflow or kmalloc to fail ...
 */

/* In fact its so inefficient a mini-cache remembers the last call info */

static long _cdecl
m_getname (fcookie *root, fcookie *dir, char *pathname, int length)
{
	SI *psblk = super_ptr[dir->dev];
	long inum = dir->index;
	ushort dev = dir->dev;
	int incr = psblk->incr;
	short plength = 0;
	
	*pathname = 0;
	
	if ((dir->dev == root->dev) && (dir->index == root->index))
		return 0;
	
	while ((inum != root->index) && (inum != ROOT_INODE))
	{
		d_inode rip;
		long chunk;
		long left;
		ulong pinum;
		
		/* Parent inum */
		pinum = search_dir ("..", inum, dev, FIND); 
		if (pinum < 0)
		{
			/* If this happens we're in trouble */
			
			ALERT (("Minix-FS (%c): m_getname: no '..' in inode %lu", DriveToLetter(dev), inum));
			return pinum;
		}
		
		read_inode (pinum, &rip, dev);
		for (chunk = 0; (left = next_zone (&rip, chunk) / DIR_ENTRY_SIZE) && inum != pinum; chunk++)
		{
			UNIT *u = cget_zone (find_zone (&rip, chunk, dev, 0), dev);
			char tname [psblk->mfname + 8];
			long i;
			for (i = 0; i < left && inum != pinum; i += incr)
			{
				if (((bufr *) u->data)->bdir[i].d_inum == inum)
				{
					strncpy (tname, ((bufr *) u->data)->bdir[i].d_name, psblk->mfname);
					tname [psblk->mfname] = 0;
					
					strrev (tname);
					
					plength += strlen (tname) + 1;
					if (length <= plength)
						return EBADARG;
					
					strcat (pathname, tname);
					strcat (pathname, "\\");
					inum = pinum;
				}
			}
		}
		
		if ((left == 0) && (inum != pinum))
		{
			ALERT (("Minix-FS (%c): m_getname: inode %lu orphaned or bad '..'", DriveToLetter(dev), inum));
			return EINTERNAL;
		}
	}
	
	if ((inum == ROOT_INODE) && (root->index != ROOT_INODE))
	{
		DEBUG (("Minix-FS (%c): m_getname: Hmmmm root is not a parent of dir.", DriveToLetter(dev)));
		return EINTERNAL;
	}
	
	strrev (pathname);
	return E_OK;
}

/* m_rename, move a file or directory. Directories need special attention 
 * because if /usr/foo is moved to /usr/foo/bar then the filesystem will be
 * damaged by making the /usr/foo directory inaccessible. The sanity checking
 * performed is very simple but should cover all cases: Start at the parent
 * of the destination , check if this is the source inode , if not then 
 * move back to '..' and check again , repeatedly check until the root inode
 * is reached , if the source is ever seen on the way back to the root then
 * the rename is invalid , otherwise it should be OK.
 */

static long _cdecl
m_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	long finode, ret;
	d_inode rip;
	long pos;
	char dirmove;
	dirmove = 0;
	
	/* Check cross drives */
	if (olddir->dev != newdir->dev)
		return EXDEV;
	
	if (super_ptr [olddir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	/* Check new doesn't exist and path is otherwise valid */
	finode = search_dir (newname, newdir->index, newdir->dev, FIND);
	if (finode > 0) return EACCES;
	if (finode != ENOENT) return finode;

	/* Check old path OK */
	if ((finode = search_dir (oldname, olddir->index, olddir->dev, FIND)) < 0)
		return finode;

	read_inode (finode, &rip, olddir->dev);

	/* Sanity check movement of directories */
	if (IS_DIR (rip))
	{
	 	if (olddir->index != newdir->index)
		{
# ifdef MFS_NMOVE_DIR
			return EACCES;
# else
			d_inode riptemp;
			ret = is_parent (newdir->index, finode, olddir->dev);
			if (ret < 0) return ret;
			if (ret) return EINVAL;
			read_inode (newdir->index, &riptemp, newdir->dev);
			if (riptemp.i_nlinks == MINIX2_LINK_MAX) return EACCES;
			TRACE (("Minix-FS (%c): valid directory move", DriveToLetter(olddir->dev)));
			dirmove = 1;
# endif
		}
	}

	/* Create new entry */
	if ((pos = search_dir (newname, newdir->index, newdir->dev, ADD)) < 0)
		return pos;
	
	/* Delete old path */
	if ((finode = search_dir (oldname, olddir->index, olddir->dev, KILL)) < 0)
			return finode;
	{
		ushort ino = finode;
		l_write (newdir->index, pos, 2L, &ino, newdir->dev);
	}

	/* When moving directories, fixup things like '..' and nlinks of old and
	 * new dirs
	 */
	if (dirmove)
	{
		pos = search_dir ("..", finode, newdir->dev, POS);
		if (pos < 0) 
		{
			ALERT (("Minix-FS (%c): m_rename: no '..' in inode %ld.", DriveToLetter(olddir->dev), finode));
			return EACCES;
		}
		if (pos != DIR_ENTRY_SIZE * super_ptr[newdir->dev]->incr)
			ALERT (("Minix-FS (%c): m_rename: Unexpected '..' position in inode %ld.", DriveToLetter(olddir->dev), finode));
		{
			ushort ino = newdir->index;
			l_write (finode, pos, 2L, &ino, newdir->dev);
		}
		read_inode (olddir->index, &rip, olddir->dev);
		rip.i_nlinks--;
		write_inode (olddir->index, &rip, olddir->dev);
		
		read_inode (newdir->index, &rip, newdir->dev);
		rip.i_nlinks++;
		write_inode (newdir->index, &rip, newdir->dev);
	}
	
	sync (olddir->dev);
	return 0;
}

static long _cdecl
m_opendir (DIR *dirh, int flag)
{
	dirh->index = 0;
	return 0;
}

static long _cdecl
m_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	SI *psblk = super_ptr [dirh->fc.dev];
        d_inode rip;
	unsigned entry, chunk;
	long limit;
	int flag, incr;
	
	if (dirh->flags & TOS_SEARCH)
		flag = do_trans (DIR_TOS, dirh->fc.dev);
	else
		flag = 0;
	
	if (!dirh->fc.index) return EACCES;
	
	entry = dirh->index % NR_DIR_ENTRIES;
	chunk = dirh->index / NR_DIR_ENTRIES;
	
	read_inode (dirh->fc.index, &rip, dirh->fc.dev);
	incr = psblk->incr;

	while ((limit = next_zone (&rip, chunk) / DIR_ENTRY_SIZE))
	{
		UNIT *u = cget_zone (find_zone (&rip, chunk, dirh->fc.dev, 0), dirh->fc.dev);
		
		while (entry < limit)
	  	{
			dir_struct *try = &((bufr *) u->data)->bdir[entry];
			entry += incr;
			if (try->d_inum)
			{
				char tmpbuf[psblk->mfname + 8];
				char *tmpnam;
				
				tmpnam = tosify (try->d_name, flag, tmpbuf, psblk->mfname);
				
				if ((dirh->flags & TOS_SEARCH) == 0)
				{
					namelen -= sizeof (long);
					if (namelen <= 0) return EBADARG;
					unaligned_putl(name, (long) try->d_inum);
					name += sizeof (long);
	       			}

				strncpy (name, tmpnam, namelen);
				dirh->index = entry + chunk * NR_DIR_ENTRIES;
				
				/* set up a file cookie for this entry */
				fc->dev = dirh->fc.dev;
				fc->aux = 0;
				fc->index = (long) try->d_inum;
				fc->fs = &minix_filesys;
				
				if (strlen (tmpnam) >= namelen) 
					return EBADARG;
				
				/* If turbo mode set atime here: we'll only
				 * change the cache here so it wont cause
				 * lots of I/O
				 */
				if (WB_CHECK (psblk) && !(psblk->s_flags & MS_RDONLY))
					set_atime (&dirh->fc);
				
				return E_OK;
			}
		}
		
		if (entry != NR_DIR_ENTRIES) return ENMFILES;
		else entry = 0;
		
		chunk++;
	}
	
	return ENMFILES;
}

static long _cdecl
m_rewinddir (DIR *dirh)
{
	dirh->index=0;
	return 0;
}

static long _cdecl
m_closedir (DIR *dirh)
{
	SI *psblk = super_ptr [dirh->fc.dev];
	
	/*
	 * Access time is set here if we aren't in TURBO cache mode. Otherwise we
	 * would be sync'ing on every dir read which would be far too slow. See note
	 * in set_atime().
	 */
	if (!WB_CHECK (psblk) && !(psblk->s_flags & MS_RDONLY))
	{
		set_atime (&dirh->fc);
		sync (dirh->fc.dev);
	}
	
	dirh->fc.index = 0;
	return 0;
}

static long _cdecl
m_pathconf (fcookie *dir, int which)
{
	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return MINIX2_LINK_MAX;
		case DP_PATHMAX:	return UNLIMITED;
		case DP_NAMEMAX:	return super_ptr[dir->dev]->mfname;
		case DP_ATOMIC:		return BLOCK_SIZE;
		case DP_TRUNC:		return DP_AUTOTRUNC;
		case DP_CASE:		return DP_CASESENS;
		case DP_MODEATTR:	return (DP_ATTRBITS
						| DP_MODEBITS
						| DP_FT_DIR
						| DP_FT_REG
						| DP_FT_LNK
					);
		case DP_XATTRFIELDS:	return (DP_INDEX
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
		case DP_VOLNAMEMAX:	return 0;
	}
	
	return ENOSYS;
}

static long _cdecl
m_dfree (fcookie *dir, long int *buffer)
{
	SI *psblk = super_ptr[dir->dev];
	
	buffer[1] = psblk->sblk->s_zones - psblk->sblk->s_firstdatazn;
	buffer[0] = buffer[1] - count_bits (psblk->zbitmap, buffer[1] + 1) + 1;
	buffer[2] = BLOCK_SIZE;
	buffer[3] = 1L;
	
	return 0;
}

static long _cdecl
m_wlabel (fcookie *dir, const char *name)
{
	return EACCES;
}

static long _cdecl
m_rlabel (fcookie *dir, char *name, int namelen)
{
	return ENOENT;
}

/* Symbolic links ... basically similar to a regular file with one zone */

long _cdecl
m_symlink (fcookie *dir, const char *name, const char *to)
{
	bufr temp;
	d_inode rip;
	long pos;
	ushort newinode;

	if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	if (!*to)
	{
		DEBUG (("Minix-FS (%c): m_symlink: invalid null filename.", DriveToLetter(dir->dev)));
		return EACCES;
	}

	/* Strip U: prefix */
	if ((to[0] == 'u' || to[0] == 'U') && to[1] == ':' && to[2] == '\\')
		to += 2;

	if (strlen (to) >= SYMLINK_NAME_MAX)
	{
		DEBUG (("Minix-FS (%c): m_symlink: symbolic link name too long.", DriveToLetter(dir->dev)));
		return EBADARG;
	}

	if ((pos = search_dir (name, dir->index, dir->dev, ADD)) < 0) return pos;

	if (!(newinode = alloc_inode (dir->dev)))
	{
		DEBUG (("Minix-FS (%c): m_symlink: no free inodes.", DriveToLetter(dir->dev)));
		return EACCES;
	}
	

	bzero (&rip, sizeof (d_inode));
	rip.i_mode = I_SYMLINK | 0777;
	rip.i_size = strlen (to);
	rip.i_uid = p_geteuid ();
	rip.i_gid = p_getegid ();
	rip.i_mtime = CURRENT_TIME;
	rip.i_ctime = rip.i_mtime;
	rip.i_atime = rip.i_mtime;
	rip.i_nlinks = 1;

	if (!(rip.i_zone[0] = alloc_zone (dir->dev)))
	{
		free_inode (dir->dev, newinode);
		DEBUG (("Minix-FS (%c): m_symlink: no free zones.", DriveToLetter(dir->dev)));
		return EACCES;
	}
	btos_cpy ((char *) &temp, to);
 	write_zone (rip.i_zone[0], &temp, dir->dev);
	write_inode (newinode, &rip, dir->dev);
	l_write (dir->index, pos, 2L, &newinode, dir->dev);
	
	sync (dir->dev);
	return 0;
}

static long _cdecl
m_readlink (fcookie *file, char *buf, int len)
{
	bufr temp;
	long inum = file->index;
	d_inode rip;
	
	read_inode (inum, &rip, file->dev);
	if ((rip.i_mode & I_TYPE) != I_SYMLINK)
	{
		DEBUG (("Minix-FS (%c): m_readlink: attempted readlink on non-symlink.", DriveToLetter(file->dev)));
		return EACCES;
	}
	read_zone (rip.i_zone[0], &temp, file->dev);
	if (temp.bdata[0] == '/' && len > 2)
	{
	    *buf++ = 'u';
	    *buf++ = ':';
	    len -= 2;
	}
	if (stob_ncpy (buf, (char *) &temp, len))
	{
		DEBUG (("Minix-FS (%c): m_readlink: name too long.", DriveToLetter(file->dev)));
		return EBADARG;
	}
	
	TRACE (("Minix-FS (%c): m_readlink returned %s", DriveToLetter(file->dev), buf));
	return 0;
}

/* Minix hard-link, you can't make a hardlink to a directory ... it causes
 * too much trouble, use symbolic links instead.
 */

static long _cdecl
m_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	long finode;
	d_inode rip;
	long pos;
	
	/* Check cross drives */
	if (fromdir->dev != todir->dev)
		return EXDEV;
	
	if (super_ptr [fromdir->dev]->s_flags & MS_RDONLY)
		return EROFS;
	
	/* Check new doesn't exist and path is otherwise valid */
	finode = search_dir (toname, todir->index, todir->dev, FIND);
	if (finode > 0) return EACCES;
	if (finode != ENOENT) return finode;

	/* Check old path OK */
	if ((finode = search_dir (fromname, fromdir->index, fromdir->dev, FIND)) < 0)
		return finode;

	read_inode (finode, &rip, fromdir->dev);
	if ((!IS_REG (rip) && !IM_SPEC (rip.i_mode))
		|| (rip.i_nlinks >= MINIX2_LINK_MAX)) return EACCES;

	/* Create new entry */
	if ((pos = search_dir (toname, todir->index, todir->dev, ADD)) < 0)
		return pos;
	
	{
		ushort ino = finode;
		l_write (todir->index, pos, 2L, &ino, todir->dev);
	}
	rip.i_nlinks++;
	rip.i_ctime = CURRENT_TIME;
	write_inode (finode, &rip, fromdir->dev);
	
	sync (fromdir->dev);
	return 0;
}

static long _cdecl
m_fscntl (fcookie *dir, const char *name, int cmd, long int arg)
{
	FILEPTR *f;
	long inum;
	int uid, gid, id;
	d_inode rip;
	
	uid = p_geteuid ();
	gid = p_getegid ();
	
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "minix");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "minix-xfs");
				info->version = ((long) MFS_MAJOR << 16) | MFS_MINOR;
				info->type = FS_MINIX;
				strcpy (info->type_asc, "minix v2");
			}
			return E_OK;
		}
		case FS_USAGE:
		{
			SI *psblk = super_ptr[dir->dev];
			struct fs_usage *inf = (struct fs_usage *) arg;
			
			inf->blocksize = BLOCK_SIZE;
			inf->blocks = psblk->sblk->s_zones - psblk->sblk->s_firstdatazn;
			inf->free_blocks = inf->blocks - count_bits (psblk->zbitmap, inf->blocks + 1) + 1;
			inf->inodes = psblk->sblk->s_ninodes;
			inf->free_inodes = inf->inodes - count_bits (psblk->ibitmap, inf->inodes + 1) + 1;
			
			return E_OK;
		}
		case MFS_VERIFY:
		{
			*((long *) arg) = MFS_MAGIC;
			return E_OK;
		}
		case MFS_SYNC:
		/* Sync the filesystem */
		{
			sync_bitmaps (super_ptr[dir->dev]);
			bio.sync_drv (super_ptr[dir->dev]->di);
			
			TRACE (("Minix-FS (%c): Done sync", DriveToLetter(dir->dev)));
			return E_OK;
		}
		case MFS_CINVALID:
		/* Invalidate all cache entries for a given drive */
		{
			/* no longer supported */
			break;
		}
		case MFS_FINVALID:
		/* Invalidate all fileptrs for a given drive */
		{
			if (uid) return EACCES;
			id = p_getpid ();
			for (f = firstptr; f; f = f->next)
				if (f->fc.dev == dir->dev)
					m_close (f, id);
			return E_OK;
		}
		case MFS_INFO:
		{
			typedef struct mfs_info mfs_info;
			struct mfs_info
			{
				long	total_inodes;
				long	total_zones;
				long	free_inodes;
				long	free_zones;
				int	version;	/* Filesystem version 1 = V1 2 = V2 */
				int	increment;	/* Directory increment */
				long	res [4];	/* Reserved for future use */
			};
			
			SI *psblk = super_ptr[dir->dev];
			mfs_info *inf = (mfs_info *) arg;
			
			inf->total_zones = psblk->sblk->s_zones-psblk->sblk->s_firstdatazn;
			inf->total_inodes = psblk->sblk->s_ninodes;
			inf->version = 2;
			inf->increment = psblk->incr;		
			inf->free_inodes = inf->total_inodes - count_bits (psblk->ibitmap, inf->total_inodes + 1) + 1;
			inf->free_zones = inf->total_zones - count_bits (psblk->zbitmap, inf->total_zones + 1) + 1;
			
			return E_OK;
		}
		case MFS_IMODE:
		{
			if (uid) return EACCES;
			
			inum = search_dir (name, dir->index, dir->dev, FIND);
			if (inum < 0) return inum;
			read_inode (inum, &rip, dir->dev);
			rip.i_mode = arg;
			write_inode (inum, &rip, dir->dev);
			
			return E_OK;
		}
		case MFS_GTRANS:
		{
			*((long *) arg) = fs_mode[dir->dev];
			return E_OK;
		}
		case MFS_STRANS:
		{
			if (uid) return EACCES;
			fs_mode[dir->dev] = *((long *) arg);
			return E_OK;
		}
		case MFS_IADDR:
		{
			/* no longer supported */
			break;
		}
		case MFS_UPDATE:
		{
			/* no longer supported */
			break;
		}
		case FUTIME:
		case FUTIME_UTC:
		case FTRUNCATE:
		{
			fcookie fc;
			read_inode (dir->index, &rip, dir->dev);
			
			if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
				return EROFS;
			
			/* Have we got 'x' access for current dir ? */
 			if (check_mode (uid, gid, &rip, S_IXUSR)) 
		  		return EACCES;
		  	
			/* Lookup the entry */
			if ((inum = m_lookup (dir, name, &fc)))
				return inum;
			
			read_inode (fc.index, &rip, fc.dev);
			if ((cmd == FUTIME) || (cmd == FUTIME_UTC))
			{
				/* The owner or super-user can always touch,
				 * others only if timeptr == 0 and write
				 * permission.
				 */
				if (uid && uid != rip.i_uid /* && (arg || check_mode (uid, gid, &rip, S_IWUSR)) */)
					return EACCES;
				
				rip.i_ctime = CURRENT_TIME;
				
				if (arg)
				{	
					if (native_utc || (cmd == FUTIME_UTC))
					{
						long *timeptr = (long *) arg;
						
						rip.i_atime = timeptr[0];
						rip.i_mtime = timeptr[1];
					}
					else
					{
						MUTIMBUF *buf = (MUTIMBUF *) arg;
						
						rip.i_atime = unixtime (buf->actime, buf->acdate);
						rip.i_mtime = unixtime (buf->modtime, buf->moddate);
					}
				}
				else
				{
					rip.i_atime =
					rip.i_mtime = rip.i_ctime;
				}
				
				write_inode (fc.index, &rip, fc.dev);
				
				sync (fc.dev);
				return E_OK;
			}

			if (!IS_REG (rip))
				return EACCES;
			
			/* Need write access as well */
			if (check_mode (uid, gid, &rip, S_IWUSR))
				return EACCES;
			
			itruncate (fc.index, fc.dev, *((long *) arg));
			
	  		sync (fc.dev);
			return E_OK;
		}
		case MFS_LOPEN:
		{
			/* Structure for returning list of open inodes on device
			 */
			typedef struct openf_list openf_list;
			struct openf_list
			{
				long	limit;		/* Number of elements in flist */
				ushort	flist[1];	/* Inode list */
			};
			
			openf_list *flist = (openf_list *) arg;
			long fcount = 0;
			inum = 0;
			
			for (f = firstptr; f; f = f->next)
			{
				/* If same file or wrong device, skip */
				if (f->fc.dev != dir->dev || f->fc.index == inum)
					continue;
				inum = f->fc.index;
				flist->flist[fcount++] = inum;
				if (fcount == flist->limit)
					return EBADARG;
			}
			flist->flist[fcount] = 0;
			
			return fcount;
		}
		case MFS_MKNOD:
		{
		 	long pos;
			unsigned inm, mode;
			
		 	if (uid) return EACCES;
		 	mode = arg & 0xffff;
			
			if (super_ptr [dir->dev]->s_flags & MS_RDONLY)
				return EROFS;
			
		 	/* Char and block specials only at present */
		 	if (!IM_SPEC (mode)) return EBADARG;

		 	/* Create new name */
		 	pos = search_dir (name, dir->index, dir->dev, ADD);
		 	if (pos < 0) return pos;		 
		 	inm =  alloc_inode (dir->dev);
		 	if (!inm) return EWRITE;

		 	bzero (&rip, sizeof (d_inode));

			rip.i_mode = mode;
			rip.i_uid = uid;
			rip.i_gid = gid;
			rip.i_nlinks = 1;

			rip.i_mtime = CURRENT_TIME;
			rip.i_atime = rip.i_mtime;
			rip.i_ctime = rip.i_mtime;
			rip.i_zone[0] = arg >> 16;

			write_inode (inm, &rip, dir->dev);
			l_write (dir->index, pos, 2L, &inm, dir->dev);
			
			sync (dir->dev);
			return E_OK;
		}
		case V_CNTR_WP:
		{
			SI *s = super_ptr[dir->dev];
			long r;
			
			r = bio.config (dir->dev, BIO_WP, arg);
			if (r || (arg == ASK))
				return r;
			
			r = EINVAL;
			if (BIO_WP_CHECK (s->di) && !(s->s_flags & MS_RDONLY))
			{
				if (!(s->s_flags & S_NOT_CLEAN_MOUNTED))
				{
					s->sblk->s_state |= MINIX_VALID_FS;
					bio_MARK_MODIFIED (&bio, s->sunit);
				}
				
				bio.sync_drv (s->di);
				
				s->s_flags |= MS_RDONLY;
				ALERT (("MinixFS [%c]: remounted read-only!", DriveToLetter(dir->dev)));
				
				r = E_OK;
			}
			else if (s->s_flags & MS_RDONLY)
			{
				s->sblk->s_state &= ~MINIX_VALID_FS;
				bio_MARK_MODIFIED (&bio, s->sunit);
				
				bio.sync_drv (s->di);
				
				s->s_flags &= ~MS_RDONLY;
				ALERT (("MinixFS [%c]: remounted read/write!", DriveToLetter(dir->dev)));
				
				r = E_OK;
			}
			
			return r;
		}
		case V_CNTR_WB:
		{
			return bio.config (dir->dev, BIO_WB, arg);
		}
	}
	
	return ENOSYS;
}

/*
 * the kernel calls this when it detects a disk change.
 */

static long _cdecl
m_dskchng (int drv, int mode)
{
	SI *s = super_ptr [drv];
	long change = 1;
	FILEPTR *f, **last;
	
	TRACE (("Minix-FS (%c): m_dskchng: enter (mode = %i)", DriveToLetter(drv), mode));
	
	if (mode == 0)
		change = BIO_DSKCHNG (s->di);
	
	if (change == 0)
	{
		/* no change */
		TRACE (("Minix-FS (%c): m_dskchng: leave no change!", DriveToLetter(drv)));
		return change;
	}
	
	/* Since the disk has changed always invalidate cache
	 * only the bitmaps are directly cached
	 */
	if (s->idirty)
	{
		ALERT (("Minix-FS: Inode bitmap not written out when drive %c invalidated", DriveToLetter(drv)));
	}
	if (s->zdirty)
	{
		ALERT (("Minix-FS: Zone bitmap not written out when drive %c invalidated", DriveToLetter(drv)));
	}
	
	s->idirty =	0;
	s->zdirty = 0;
	
	
	/* free the DI (invalidate also the cache units) */
	bio.free_di (s->di);
	
	/* free allocated memory */
	kfree (s->ibitmap);
	kfree (s);
	
	super_ptr [drv] = 0;
	
	/* Free any memory associated to file pointers of this drive. */
	last = &firstptr;
	for (f = *last; f != 0; f = *last)
	{
		if (f->fc.dev == drv)
		{
			f_cache *fch = (f_cache *) f->devinfo;
			
			/*
			 * The lock structure is shared between the fileptr's.
			 * Make sure that it is freed only once.
			 */
			if (!f->next || f->next->fc.dev != drv || f->next->fc.index != f->fc.index)
			{
				LOCK *lck, *nextlck;
				nextlck = *fch->lfirst;
				while ((lck = nextlck) != 0)
				{
					nextlck = lck->next;
					kfree (lck);
				}
				kfree (fch->lfirst);
			}
			kfree (fch);
			
			/* Remove this fileptr from the list. */
			*last = f->next;
			f->next = 0;
		}
		else
			last = &f->next;
	}
	
	return change;
}

static long _cdecl
m_release (fcookie *fc)
{
	return E_OK;
}

static long _cdecl
m_dupcookie (fcookie *dest, fcookie *src)
{
	ushort tmpaux = dest->aux;
	
	*dest = *src;
	if (restore_dev != - 1 && (tmpaux & (AUX_DEV|AUX_DRV)) == (restore_dev|AUX_DRV))
		dest->dev = tmpaux & AUX_DEV;
	
	return E_OK;
}

static long _cdecl
m_sync (void)
{
	int i;
	
	for (i = 0; i < NUM_DRIVES; i++)
	{
		register SI *s = super_ptr[i];
		if (s)
		{
			sync_bitmaps (s);
		}
	}
	
	/* buffer cache automatically synced */
	return E_OK;
}

static long _cdecl
m_mknod (fcookie *dir, const char *name, ulong mode)
{
	return ENOSYS;
}

static long _cdecl
m_unmount (int drv)
{
	SI *s = super_ptr[drv];
	
	if (!(s->s_flags & MS_RDONLY) && !(s->s_flags & S_NOT_CLEAN_MOUNTED))
	{
		s->sblk->s_state |= MINIX_VALID_FS;
		bio_MARK_MODIFIED (&bio, s->sunit);
	}
	else
	{
		DEBUG (("can't unmount cleanly"));
	}
	
	/* sync bitmaps */
	sync_bitmaps (s);
	
	/* sync the buffer cache */
	bio.sync_drv (s->di);
	
	/* invalidate the drv */
	m_dskchng (drv, 1);
	
	return E_OK;
}
