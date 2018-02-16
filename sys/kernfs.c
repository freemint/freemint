/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Guido Flohr <guido@freemint.de>
 * All rights reserved.
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
 *
 * Author: Guido Flohr <guido@freemint.de>
 * Started: 1999-10-24
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Kernel system info filesystem.
 *
 * This file is way too long and the functions indent too deep.  Besides
 * it should be rewritten from scratch to implement a more object-oriented
 * approach, maybe.
 * In fact this file is not only too long but it should be split up
 * into separate files in a separate subdirectory but the kernel build
 * process currently doesn't allow this, thus making it difficult to
 * think of meaningful names.
 */

# include "kernfs.h"
# include "global.h"

# if WITH_KERNFS

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "mint/credentials.h"
# include "mint/dcntl.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "arch/mprot.h"
# include "arch/kernfs_mach.h"

# include "dev-null.h"
# include "filesys.h"
# include "kernget.h"
# include "kmemory.h"
# include "memory.h"
# include "nullfs.h"
# include "proc.h"
# include "procfs.h"
# include "signal.h"
# include "time.h"
# include "unifs.h"
# include "util.h"
# include "xbios.h"


# ifdef DEBUG_INFO
# define KERN_ASSERT(x)		assert x
# else
# define KERN_ASSERT(x)
# endif


/* Basic file system operations */
static long	_cdecl kern_root	(int drv, fcookie *fc);
static long	_cdecl kern_lookup	(fcookie *dir, const char *name, fcookie *fc);
static DEVDRV *	_cdecl kern_getdev	(fcookie *file, long *devspecial);
static long	_cdecl kern_stat64	(fcookie *file, STAT *stat);
static long	_cdecl kern_getname	(fcookie *relto, fcookie *dir, char *pathname, int size);
static long	_cdecl kern_readdir	(DIR *dirh, char *name, int namelen, fcookie *fc);
static long	_cdecl kern_pathconf	(fcookie *dir, int which);
static long	_cdecl kern_dfree	(fcookie *dir, long *buf);
static long	_cdecl kern_readlabel	(fcookie *dir, char *name, int namelen);
static long	_cdecl kern_readlink	(fcookie *dir, char *buf, int len);
static long	_cdecl kern_fscntl	(fcookie *dir, const char *name, int cmd, long arg);

/* Device driver functions */
static long	_cdecl kern_open	(FILEPTR *f);
static long	_cdecl kern_close	(FILEPTR *f, int pid);
static long	_cdecl kern_read	(FILEPTR *f, char *buf, long bytes);
static long	_cdecl kern_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl kern_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long	_cdecl kern_lseek	(FILEPTR *f, long where, int whence);


FILESYS kern_filesys =
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
	FS_REENTRANT_L1 	|
	FS_REENTRANT_L2		|
	FS_EXT_1		|
	FS_EXT_2		|
	FS_EXT_3		,

	kern_root,
	kern_lookup, null_creat, kern_getdev, NULL,
	null_chattr, null_chown, null_chmode,
	null_mkdir, null_rmdir, null_remove, kern_getname, null_rename,
	null_opendir, kern_readdir, null_rewinddir, null_closedir,
	kern_pathconf, kern_dfree, null_writelabel, kern_readlabel,
	null_symlink, kern_readlink, null_hardlink, kern_fscntl, null_dskchng,
	NULL, /* release */
	NULL, /* dupcookie */
	NULL, /* sync */

	/* FS_EXT_1 */
	null_mknod, null_unmount,

	/* FS_EXT_2
	 */

	/* FS_EXT_3 */
	kern_stat64,

	0, 0, 0, 0, 0,
	NULL, NULL

};

static DEVDRV kern_device =
{
	kern_open, null_write, kern_read, kern_lseek, kern_ioctl,
	kern_datime, kern_close, null_select, null_unselect
};



/* The inode numbers are organized as follows:
 *
 * 1 - file system root /sys.
 * 2 - loadavg
 * 3 - ... see ROOTDIR_*
 *
 * For the per process directories the inode numbers are:
 *
 * /sys/<PID>: 		(pid << 16 | 0x2)
 * /sys/<PID>/status: 	(pid << 16 | 0x3)
 * /sys/<PID>/mem: 	(pid << 16 | 0x4)
 * /sys/<PID>/cwd:	(pid << 16 | 0x5)
 * /sys/<PID>/root:	(pid << 16 | 0x6)
 * /sys/<PID>/exe:	(pid << 16 | 0x7)
 * /sys/<PID>/fd:	(pid << 16 | 0x8)
 * /sys/<PID>/environ:	(pid << 16 | 0x9)
 * /sys/<PID>/cmdline:	(pid << 16 | 0xa)
 * /sys/<PID>/stat:	(pid << 16 | 0xb)
 * /sys/<PID>/statm:	(pid << 16 | 0xc)
 * /sys/<PID>/meminfo:	(pid << 16 | 0xd)
 * /sys/<PID>/fname:	(pid << 16 | 0xe)
 *
 * For the file handles directory:
 *
 * /sys/<PID>/fd/<HANDLE>:	(pid << 16 | 0x100 | handle)
 */


typedef struct kentry KENTRY;
struct kentry
{
	ulong	inode;
	ulong	mode;
	char	*name;
	long	(*get)();
};

typedef struct ktab	KTAB;
struct ktab
{
	KENTRY	*tab;
	long	size;
};


# define ROOTDIR_ROOT		0x00
# define ROOTDIR_LOADAVG	0x02
# define ROOTDIR_UPTIME		0x03
# define ROOTDIR_BOOTLOG		0x04
/* advice */
/* help */
# define ROOTDIR_SELF		0x06
# define ROOTDIR_CPUINFO	0x07
# define ROOTDIR_DEVICES	0x08
# define ROOTDIR_DMA		0x09
# define ROOTDIR_FILESYSTEMS	0x0a
# define ROOTDIR_MEMDEBUG	0x0b
# define ROOTDIR_MEMINFO	0x0c
# define ROOTDIR_VERSION	0x0d
# define ROOTDIR_COOKIEJAR	0x0e
# define ROOTDIR_HZ		0x0f
# define ROOTDIR_TIME		0x10
# define ROOTDIR_WELCOME	0x11
# define ROOTDIR_BUILDINFO	0x12
# define ROOTDIR_STAT       	0x13
# define ROOTDIR_SYSDIR		0x14

static KENTRY __rootdir [] =
{
	{ ROOTDIR_ROOT,		S_IFDIR | 0555,	".",		kern_get_unimplemented	},
	{ ROOTDIR_ROOT,		S_IFDIR | 0555,	"..",		kern_get_unimplemented	},
	{ ROOTDIR_BOOTLOG,	S_IFREG | 0444,	"bootlog",	kern_get_bootlog},
	{ ROOTDIR_BUILDINFO,	S_IFREG | 0444,	"buildinfo",	kern_get_buildinfo	},
	{ ROOTDIR_COOKIEJAR,	S_IFREG | 0444,	"cookiejar",	kern_get_cookiejar	},
	{ ROOTDIR_CPUINFO,	S_IFREG | 0444,	"cpuinfo",	kern_get_cpuinfo	},
	{ ROOTDIR_DEVICES,	S_IFREG | 0444,	"devices",	kern_get_unimplemented	},
	{ ROOTDIR_DMA,		S_IFREG | 0444,	"dma",		kern_get_unimplemented	},
	{ ROOTDIR_FILESYSTEMS,	S_IFREG | 0444,	"filesystems",	kern_get_filesystems	},
	{ ROOTDIR_HZ,		S_IFREG | 0444,	"hz",		kern_get_hz		},
	{ ROOTDIR_LOADAVG,	S_IFREG | 0444,	"loadavg",	kern_get_loadavg	},
# ifdef DEBUG_INFO
	{ ROOTDIR_MEMDEBUG,	S_IFREG | 0444,	"memdebug",	kern_get_memdebug	},
# endif
	{ ROOTDIR_MEMINFO,	S_IFREG | 0444,	"meminfo",	kern_get_meminfo	},
	{ ROOTDIR_SELF,		S_IFLNK | 0777,	"self",		kern_get_unimplemented	},
	{ ROOTDIR_STAT,		S_IFREG | 0444,	"stat",		kern_get_stat		},
	{ ROOTDIR_SYSDIR,	S_IFREG | 0444, "sysdir",	kern_get_sysdir		},
	{ ROOTDIR_TIME,		S_IFREG | 0444,	"time",		kern_get_time		},
	{ ROOTDIR_UPTIME,	S_IFREG | 0444,	"uptime",	kern_get_uptime		},
	{ ROOTDIR_VERSION,	S_IFREG | 0444,	"version",	kern_get_version	},
	{ ROOTDIR_WELCOME,	S_IFREG | 0444,	"welcome",	kern_get_welcome	}
};

static KTAB _rootdir =
{
	__rootdir,
	sizeof (__rootdir) / sizeof (__rootdir[0])
};

static KTAB *rootdir = &_rootdir;


# define PROCDIR_DIR    	0x02
# define PROCDIR_STATUS 	0x03
# define PROCDIR_MEM    	0x04
# define PROCDIR_CWD		0x05
# define PROCDIR_ROOT		0x06
# define PROCDIR_EXE		0x07
# define PROCDIR_FD		0x08
# define PROCDIR_ENVIRON	0x09
# define PROCDIR_CMDLINE	0x0a
# define PROCDIR_STAT		0x0b
# define PROCDIR_STATM		0x0c
# define PROCDIR_MEMINFO	0x0d
# define PROCDIR_FNAME		0x0e

static KENTRY __procdir [] =
{
	{ PROCDIR_DIR,		S_IFDIR | 0555,	".",		kern_get_unimplemented	},
	{ ROOTDIR_ROOT,		S_IFDIR | 0555,	"..",		kern_get_unimplemented	},
	{ PROCDIR_CMDLINE,	S_IFREG | 0444,	"cmdline",	kern_procdir_get_cmdline},
	{ PROCDIR_CWD,		S_IFLNK | 0700,	"cwd",		kern_get_unimplemented	},
	{ PROCDIR_ENVIRON,	S_IFREG | 0400,	"environ",	kern_procdir_get_environ},
	{ PROCDIR_EXE,		S_IFLNK | 0500,	"exe",		kern_get_unimplemented	},
	{ PROCDIR_FD,		S_IFDIR | 0500,	"fd",		kern_get_unimplemented	},
	{ PROCDIR_FNAME,	S_IFREG | 0444,	"fname",	kern_procdir_get_fname	},
	{ PROCDIR_MEM,		S_IFREG | 0600,	"mem",		kern_get_unimplemented	},
	{ PROCDIR_MEMINFO,	S_IFREG | 0444,	"meminfo",	kern_procdir_get_meminfo},
	{ PROCDIR_ROOT,		S_IFLNK | 0700,	"root",		kern_get_unimplemented	},
	{ PROCDIR_STAT,		S_IFREG | 0444,	"stat",		kern_procdir_get_stat	},
	{ PROCDIR_STATM,	S_IFREG | 0444,	"statm",	kern_procdir_get_statm	},
	{ PROCDIR_STATUS,	S_IFREG | 0444,	"status",	kern_procdir_get_status	}
};

static KTAB _procdir =
{
	__procdir,
	sizeof (__procdir) / sizeof (__procdir[0])
};

static KTAB *procdir = &_procdir;


static KENTRY *
search_inode (KTAB *k, long inode)
{
	int i;

	for (i = 0; i < k->size; i++)
		if (k->tab[i].inode == inode)
			return &k->tab[i];

	return NULL;
}

static KENTRY *
search_name (KTAB *k, const char *name)
{
# if 1
	KENTRY *low = &k->tab[2];
	KENTRY *high = k->tab + k->size - 1;

	while (low <= high)
	{
		if (strcmp (low->name, name) == 0)
			return low;

		low++;
	}

	return NULL;
# else
	KENTRY *low = &k->tab[2];
	KENTRY *high = k->tab + k->size - 1;
	KENTRY *mid;
	int c;

	while (low <= high)
	{
		mid = low + ((high - low) >> 1);

		c = strcmp (mid->name, name);
		if (c == 0)
			return mid;

		if (c < 0)	low = mid + 1;
		else		high = mid - 1;
	}

	return NULL;
# endif
}


static int
follow_link_denied (PROC *p, const char *function)
{
	int deny = get_curproc()->p_cred->ucr->euid
			&& (get_curproc()->p_cred->ucr->euid ^ p->p_cred->suid)
			&& (get_curproc()->p_cred->ucr->euid ^ p->p_cred->ruid)
			&& (get_curproc()->p_cred->ruid ^ p->p_cred->suid)
			&& (get_curproc()->p_cred->ruid ^ p->p_cred->ruid);

	if (deny)
		DEBUG (("%s: can't follow links for pid %d: Access denied", function, p->pid));

	return deny;
}


static long _cdecl
kern_root (int drv, fcookie *fc)
{
	DEBUG (("kern_root (%d) (KERNDRV = %d)", drv, KERNDRV));

	if (drv == KERNDRV)
	{
		fc->fs = &kern_filesys;
		fc->dev = drv;
		fc->index = 0L;

		return E_OK;
	}

	DEBUG (("kern_root: oops: wrong pseudo-drive"));

	fc->fs = NULL;
	return EINTERNAL;
}

INLINE long
kern_proc_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	KENTRY *t;

	TRACE (("kern_proc_lookup in [%u:%ld] for %s", dir->dev, dir->index, name));

	/* 1 - itself */
	if (!*name || (name[0] == '.' && name[1] == '\0'))
	{
		*fc = *dir;
		return E_OK;
	}

	fc->fs = &kern_filesys;
	fc->dev = KERNDRV;
	fc->aux = 0;

	/* 2 - parent dir */
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
	{
		fc->index = ROOTDIR_ROOT;
		return E_OK;
	}

	t = search_name (procdir, name);
	if (t)
	{
		fc->index = dir->index & 0xffff0000;
		fc->index |= t->inode;
		return E_OK;
	}

	DEBUG (("kern_proc_lookup in [%u:%ld] for %s: No such file or directory",  dir->dev, dir->index, name));
	return ENOENT;
}

INLINE long
kern_fddir_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	long desc;
	short pid = (dir->index & 0xffff0000) >> 16;
	PROC *p;

	TRACE (("kern_fddir_lookup in [%u:%ld] for %s", dir->dev, dir->index, name));

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_fddir_lookup: r.i.p., pid %d", pid));
		return ENOENT;
	}

	fc->dev = KERNDRV;
	fc->fs = &kern_filesys;
	fc->aux = 0;

	if (name[0] == '.' && name[1] == '\0')
		*fc = *dir;
	else if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
		fc->index = (dir->index & 0xffff0000) | PROCDIR_DIR;
	else if (strtonumber (name, &desc, 0, 0) == 0)
	{
		char cdesc;

		if (desc < 0 || desc > 255)
		{
			DEBUG (("kern_fd_lookup: pid %d, invalid descriptor name: %d", pid, (int) desc));
			return ENOENT;
		}

		cdesc = (desc & 0xff);

		if (!p->p_fd)
		{
			DEBUG (("kern_fddir_lookup: pid %d, no fd table", pid));
			return ENOENT;
		}

		if (cdesc < -5 || cdesc >= p->p_fd->nfiles
			|| p->p_fd->ofiles[(int) (cdesc)] == NULL)
		{
			DEBUG (("kern_fddir_lookup: pid %d, invalid descriptor: %d", pid, (int) cdesc));
			return ENOENT;
		}

		fc->index = (dir->index & 0xffff0000) | 0x100 | (desc & 0xff);
	}
	else
	{
		DEBUG (("kern_fddir_lookup: /sys/fd/%d/%s: No such file or directory", (int) pid, name));
		return ENOENT;
	}

	return E_OK;
}

static long _cdecl
kern_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	KENTRY *t;
	long pid;

	TRACE (("kern_lookup in [%u:%ld] for %s", dir->dev, dir->index, name));

	if ((dir->index & 0xffff0000) && (dir->index & 0x0000ffff) == PROCDIR_FD)
		return kern_fddir_lookup (dir, name, fc);

	if (dir->index & 0xffff0000)
		return kern_proc_lookup (dir, name, fc);

	/* 1 - itself */
	if (!*name || (name[0] == '.' && name[1] == '\0'))
	{
		KERN_ASSERT ((dir->index == 0));

		*fc = *dir;
		return E_OK;
	}

	/* 2 - parent dir */
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
	{
		KERN_ASSERT ((dir->index == 0));

		*fc = *dir;
		return EMOUNT;
	}

	t = search_name (rootdir, name);
	if (t)
	{
		fc->fs = &kern_filesys;
		fc->dev = KERNDRV;
		fc->aux = 0;
		fc->index = t->inode;

		return E_OK;
	}

	if (strtonumber (name, &pid, 0, 0) == 0)
	{
		PROC *p = pid2proc (pid);

		if (p == NULL || pid <= 0L)
		{
			DEBUG (("kern_lookup in [%u:%ld]: No such process: %ld", dir->dev, dir->index, pid));
			return ENOENT;
		}

		fc->fs = &kern_filesys;
		fc->dev = KERNDRV;
		fc->aux = 0;
		fc->index = (pid << 16) | PROCDIR_DIR;

		return E_OK;
	}

	DEBUG (("kern_lookup in [%u:%ld] for %s failed", dir->dev, dir->index, name));
	return ENOENT;
}

static DEVDRV * _cdecl
kern_getdev (fcookie *fc, long *devspecial)
{
	return &kern_device;
}

INLINE long
kern_proc_stat64 (fcookie *file, STAT *stat)
{
	int pid = file->index >> 16;
	PROC *p;
	KENTRY *t;

	TRACE (("kern_proc_stat ([%u:%ld], %i)", file->dev, file->index, pid));

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_proc_stat: no such process id %d", pid));
		return ENOENT;
	}

	t = search_inode (procdir, file->index & 0xffff);
	if (!t) return ENOENT;

	synch_timers ();

	stat->dev = KERNDRV;
	stat->ino = file->index;
	stat->mode = t->mode;
	stat->rdev = KERNDRV;
	stat->nlink = 1;
	stat->uid = p->p_cred->ruid;
	stat->gid = p->p_cred->rgid;
	stat->size = 0;
	stat->blksize = 1;
	stat->blocks = 0;

	stat->atime.high_time = 0;
	stat->atime.time = xtime.tv_sec;
	stat->atime.nanoseconds = xtime.tv_usec;

	stat->mtime.high_time = 0;
	stat->mtime.time = p->started.tv_sec;
	stat->mtime.nanoseconds = p->started.tv_usec;

	stat->ctime.high_time = 0;
	stat->ctime.time = p->started.tv_sec;
	stat->ctime.nanoseconds = p->started.tv_usec;

	stat->flags = 0;
	stat->gen = 0;

	mint_bzero (stat->res, sizeof (stat->res));

	switch (file->index & 0xffff)
	{
		case PROCDIR_DIR:
		{
			stat->nlink = 2;
			break;
		}
	}

	return E_OK;
}

INLINE long
kern_fddir_stat64 (fcookie *file, STAT *stat)
{
	int pid = file->index >> 16;
	PROC *p;

	TRACE (("kern_fddir_stat ([%u:%ld], %i)", file->dev, file->index, pid));

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_fddir_stat: no such process id %i", pid));
		return ENOENT;
	}

	synch_timers ();

	stat->dev = KERNDRV;
	stat->ino = file->index;
	stat->mode = S_IFLNK | 0500;
	stat->rdev = KERNDRV;
	stat->nlink = 1;
	stat->uid = p->p_cred->ruid;
	stat->gid = p->p_cred->rgid;
	stat->size = 0;
	stat->blksize = 1;
	stat->blocks = 0;
	stat->mtime.time = p->started.tv_sec;
	stat->mtime.nanoseconds = p->started.tv_usec;
	stat->atime.time = xtime.tv_sec;
	stat->atime.nanoseconds = xtime.tv_usec;
	stat->ctime.time = p->started.tv_sec;
	stat->ctime.nanoseconds = p->started.tv_usec;
	stat->flags = 0;
	stat->gen = 0;

	mint_bzero (stat->res, sizeof (stat->res));

	if ((file->index & 0xffff0000) == PROCDIR_FD)
		stat->mode = S_IFDIR | 0500;
	else if ((file->index & 0xffff0000) == PROCDIR_DIR)
		stat->mode = S_IFDIR | 0555;
	else
	{
		char desc = (char) file->index;

		if ((file->index & 0x0000ff00) != 0x100)
			return ENOENT;

		if (!p->p_fd)
		{
			DEBUG (("kern_fddir_stat64: pid %d, no fd table", pid));
			return ENOENT;
		}

		if (desc < -5 || desc >= p->p_fd->nfiles)
			return ENOENT;

		if (*(p->p_fd->ofiles + desc) == NULL)
			return ENOENT;
	}

	return E_OK;
}

static long _cdecl
kern_stat64 (fcookie *file, STAT *stat)
{
	KENTRY *t;

	TRACE (("kern_stat64 ([%u:%ld])", file->dev, file->index));

	if ((file->index & 0xffff0000) && (file->index & 0xff00) == 0x100)
		return kern_fddir_stat64 (file, stat);

	if (file->index & 0xffff0000)
		return kern_proc_stat64 (file, stat);

	t = search_inode (rootdir, file->index);
	if (!t) return ENOENT;

	synch_timers ();

	stat->dev = KERNDRV;
	stat->ino = file->index;
	stat->rdev = KERNDRV;
	stat->mode = t->mode;
	stat->nlink = 1;
	stat->uid = 0;
	stat->gid = 0;
	stat->size = 0;
	stat->blocks = 0;
	stat->blksize = 1;
	stat->mtime.time = rootproc->started.tv_sec;
	stat->mtime.nanoseconds = rootproc->started.tv_usec;
	stat->atime.time = xtime.tv_sec;
	stat->atime.nanoseconds = xtime.tv_usec;
	stat->ctime.time = rootproc->started.tv_sec;
	stat->ctime.nanoseconds = rootproc->started.tv_usec;
	stat->flags = 0;
	stat->gen = 0;

	mint_bzero (stat->res, sizeof (stat->res));

	switch (file->index)
	{
		case ROOTDIR_ROOT:
		{
			stat->nlink = 2;
			stat->mtime.time = procfs_stmp.tv_sec;
			stat->mtime.nanoseconds = procfs_stmp.tv_usec;

			break;
		}
		case ROOTDIR_SELF:
		{
			stat->uid = get_curproc()->p_cred->ruid;
			stat->gid = get_curproc()->p_cred->rgid;

			break;
		}
	}

	return E_OK;
}

/* This function makes the assumption that it never gets called with
 * arguments RELTO and DIR where DIR is not really a subdirectory
 * of RELTO.  Example: RELTO must not refer to "/sys/25/fd" if
 * DIR refers to "/sys/17".  That would require to return something
 * like "../../17".
 */
static long _cdecl
kern_getname (fcookie *relto, fcookie *dir, char *pathname, int size)
{
	char rootname[32] = { '\0' };  /* Always enough */
	char *crs = rootname;
	ulong len;
	int depth = 0;
	int i;

	TRACE (("kern_getname, relto: 0x%lx, dir: 0x%lx",
		(ulong) relto->index, (ulong) dir->index));

	if (relto->index == dir->index)
	{
		if (size <= 0)
			return ENAMETOOLONG;
		*pathname = '\0';
		return 0;
	}

	*crs++ = '\\';

	if (relto->index & 0xffff0000)
	{
		depth = 1;
		if (relto->index & 0x0000ff00)
			depth++;
	}

	if (dir->index & 0xffff0000)
	{
		ksprintf (crs, sizeof (rootname), "%ld", (dir->index >> 16) & 0xffff);
		while (*crs != 0)
			crs++;

		if (dir->index & 0x0000ff00)
		{
			*crs++ = '\\';
			*crs++ = 'f';
			*crs++ = 'd';
			*crs++ = '\0';
		}
	}

	/* Skip DEPTH backslashes */
	crs = rootname;
	for (i = 0; depth != 0 && i <= depth; i++)
	{
		while (*crs++ != '\\');
		crs++;
	}

	len = strlen (crs);
	if (len >= size)
	{
		DEBUG (("kern_getname, relto: 0x%lx, dir: 0x%lx, rootname: %s, crs: %s, name too long",
			(ulong) relto->index,
			(ulong) dir->index,
			rootname, crs));

		return ENAMETOOLONG;
	}

	TRACE (("kern_getname, relto: 0x%lx, dir: 0x%lx, rootname: %s, crs: %s",
		(ulong) relto->index, (ulong) dir->index,
		rootname, crs));

	strncpy_f (pathname, rootname, size);

	return E_OK;
}

INLINE long
kern_proc_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	char *src;
	ulong len;

	TRACE (("kern_pr_readdir: index: %d", dirh->index));

	/* simple check */
	if (namelen < 4)
		return EBADARG;

	/* Almost always fits */
	fc->fs = &kern_filesys;
	fc->dev = KERNDRV;
	fc->aux = 0;

	if ((dirh->index >= 0) && (dirh->index < procdir->size))
	{
		KENTRY *t = &procdir->tab[dirh->index];

		fc->index = t->inode;
		src = t->name;
	}
	else
		return ENMFILES;

	if (fc->index != ROOTDIR_ROOT)
		fc->index |= dirh->fc.index << 16;

	if (!(dirh->flags & TOS_SEARCH))
	{
		memcpy (name, &fc->index, 4);

		name += 4;
		namelen -= 4;
	}

	/* The MiNT documentation appendix E states that we always have
	 * to stuff as many bytes as possible into the buffer.
	 */
	strncpy_f (name, src, namelen);

	len = strlen (src);
	if (len >= namelen)
	{
		DEBUG (("kern_pr_readdir, dirh->index: %d, name: %s: name too long", dirh->index, src));
		return EBADARG;
	}

	TRACE (("kern_pr_readdir, dirh->index: %d, name: %s", dirh->index, name));
	dirh->index++;

	return E_OK;
}

INLINE long
kern_fddir_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	char buf[13];
	ulong len;
	short pid = dirh->fc.index >> 16;
	PROC *p;

	/* simple check */
	if (namelen < 4)
		return EBADARG;

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_fddir_readdir: r.i.p., pid %d", pid));
		return ENOENT;
	}

	TRACE (("kern_fddir_readdir: index: %d in [%04u]:%lu", dirh->index,
		dirh->fc.dev, (ulong) dirh->fc.index));

	fc->fs = &kern_filesys;
	fc->dev = KERNDRV;
	fc->aux = 0;

	/* We map dirh->index like this to files:
	 * 0 - "."
	 * 1 - ".."
	 * 2 - "
	 * 3 - "-4" (252)
	 * ...
	 * 6 - "-1" (255)
	 * 7 - "0"
	 * ...
	 */

	switch (dirh->index)
	{
		case 0:
		{
			buf[0] = '.';
			buf[1] = '\0';
			fc->index = dirh->fc.index;
			break;
		}
		case 1:
		{
			buf[0] = '.';
			buf[1] = '.';
			buf[2] = '\0';
			fc->index = (dirh->fc.index & 0xffff0000) | PROCDIR_DIR;
			break;
		}
		default:
		{
			if (!p->p_fd)
			{
				DEBUG (("kern_fddir_readdir: pid %d, no fd table", pid));
				return ENMFILES;
			}

			while (dirh->index < p->p_fd->nfiles + 7)
			{
				long desc = ((long) (dirh->index)) - 7;
				if (p->p_fd->ofiles[desc] != NULL)
				{
					ksprintf (buf, sizeof (buf), "%lu", (ulong) (desc & 0xff));
					fc->index =
						(dirh->fc.index & 0xffff0000) |
						0x100 | (desc & 0xff);
					break;
				}

				dirh->index++;
			}

			if (dirh->index >= p->p_fd->nfiles + 7)
				return ENMFILES;

			break;
		}
	}

	if (!(dirh->flags & TOS_SEARCH))
	{
		memcpy (name, &fc->index, 4);

		name += 4;
		namelen -= 4;
	}

	/* The MiNT documentation appendix E states that we always have
	 * to stuff as many bytes as possible into the buffer.
	 */
	strncpy_f (name, buf, namelen);

	len = strlen (buf);
	if (len >= namelen)
	{
		DEBUG (("kern_fddir_readdir: /sys/%d/fd/%s: Name too long", pid, buf));
		return EBADARG;
	}

	TRACE (("kern_fddir_readdir, dirh->index: %d, name: %s", dirh->index, name));
	dirh->index++;

	return E_OK;
}

static long _cdecl
kern_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	char buf[32];		/* Should be enough */
	char *src;
	ulong len;

	TRACE (("kern_readdir: index: %d, namelen: %d", dirh->index, namelen));

	if ((dirh->fc.index & 0xffff0000) && (dirh->fc.index & 0xffff) == PROCDIR_FD)
		return kern_fddir_readdir (dirh, name, namelen, fc);

	if (dirh->fc.index & 0xffff0000)
		return kern_proc_readdir (dirh, name, namelen, fc);

	/* simple check */
	if (namelen < 4)
		return EBADARG;

	/* Almost always fits */
	fc->fs = &kern_filesys;
	fc->dev = KERNDRV;
	fc->aux = 0;

	if (dirh->index < ROOTDIR_ROOT)
		dirh->index = ROOTDIR_ROOT;

	if (dirh->index < rootdir->size)
	{
		KENTRY *t = &rootdir->tab[dirh->index];

		fc->index = t->inode;
		src = t->name;
	}
	else
	{
		/* We somehow have to map the 16 bit dirh->index to our 32 bit
		 * inode numbers.  We assign index 32767 (max short) to pid
		 * 999 (max pid) and count back.
		 */
# define FIRST_PROCDIR_INDEX	31769
# define LAST_PROCDIR_INDEX	32767

		PROC *p;

		if (dirh->index < FIRST_PROCDIR_INDEX)
			dirh->index = FIRST_PROCDIR_INDEX;

		for (;;)
		{
			p = pid2proc (dirh->index - FIRST_PROCDIR_INDEX + 1);

			/* Don't be smart and put that into the for condition
			 * as "dirh->index <= LAST_PROCDIR_INDEX".  You will
			 * run into an endless loop.
			 */
			if (p || dirh->index == LAST_PROCDIR_INDEX)
				break;

			dirh->index++;
		}

		if (!p)
			return ENMFILES;

		ksprintf (buf, sizeof (buf), "%d", (int) p->pid);

		fc->index = ((long) p->pid) << 16 | PROCDIR_DIR;
		src = buf;
	}

	if (!(dirh->flags & TOS_SEARCH))
	{
		memcpy (name, &fc->index, 4);

		name += 4;
		namelen -= 4;
	}

	/* The MiNT documentation appendix E states that we always have
	 * to stuff as many bytes as possible into the buffer.
	 */
	strncpy_f (name, src, namelen);

	len = strlen (src);
	if (len >= namelen)
	{
		DEBUG (("kern_readdir, dirh->index: %d, name: %s, name too long", dirh->index, src));
		return EBADARG;
	}

	TRACE (("kern_readdir, dirh->index: %d, name: %s", dirh->index, name));
	dirh->index++;

	return E_OK;
}

static long _cdecl
kern_pathconf (fcookie *dir, int which)
{
	UNUSED (dir);

	TRACE (("kern_pathconf (%d)", which));

	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return UNLIMITED;
		case DP_PATHMAX:	return UNLIMITED;
		case DP_NAMEMAX:	return 64;   /* Don't scare the caller */
		case DP_ATOMIC:		return 1024;
		case DP_TRUNC:		return DP_AUTOTRUNC;
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
		case DP_VOLNAMEMAX:	return 0;
	}

	DEBUG (("kern_pathconf: unknown opcode 0x%04x", (unsigned) which));
	return ENOSYS;
}

static long _cdecl
kern_dfree (fcookie *dir, long *buf)
{
	long size;
	long secsiz = ROUND (1);

	/* "sector" size is the size of the smallest amount of memory that
	 * can be allocated. see mem.h for the definition of ROUND
	 */

	TRACE (("kern_dfree"));
	UNUSED (dir);

	/* Number of free clusters */
	size = tot_rsize (core, 0) + tot_rsize (alt, 0);
	*buf++ = size / secsiz;

	/* Total number of clusters */
	size = tot_rsize (core, 1) + tot_rsize (alt, 1);
	*buf++ = size / secsiz;

	/* Sector size (bytes) */
	*buf++ = secsiz;

	/* Cluster size (in sectors) */
	*buf = 1;

	return E_OK;
}

static long _cdecl
kern_readlabel (fcookie *dir, char *name, int namelen)
{
	UNUSED (dir);

	if (sizeof "Sysinfo" >= namelen)
	{
		DEBUG (("kern_readlabel: zut alors! name too long"));
		return ENAMETOOLONG;
	}

	strncpy (name, "Sysinfo", namelen);
	return E_OK;
}

INLINE long
kern_proc_readlink (fcookie *dir, char *name, int namelen)
{
	short pid = dir->index >> 16;
	PROC *p;
	struct cwd *cwd;
	char buf[64];
	ulong len;

	TRACE (("kern_proc_readlink ([%u:%ld])", dir->dev, dir->index));

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_proc_readlink ([%u:%ld]): pid %d: No such process", dir->dev, dir->index, pid));
		return ENOENT;
	}

	cwd = p->p_cwd;

	switch (dir->index & 0x0000ffff)
	{
		case PROCDIR_CWD:
		{
			if (follow_link_denied (p, __FUNCTION__))
				return EACCES;
			ksprintf (buf, sizeof (buf), "[%04u]:%lu",
				cwd->curdir[cwd->curdrv].dev,
				(ulong) cwd->curdir[cwd->curdrv].index);
			break;
		}
		case PROCDIR_EXE:
		{
			if (follow_link_denied (p, __FUNCTION__))
				return EACCES;
			ksprintf (buf, sizeof (buf), "[%04u]:%lu",
				p->exe.dev,
				(ulong) p->exe.index);
			break;
		}
		case PROCDIR_ROOT:
		{
			if (follow_link_denied (p, __FUNCTION__))
				return EACCES;
			ksprintf (buf, sizeof (buf), "[%04u]:%lu", cwd->rootdir.dev,
				(ulong) cwd->rootdir.index);
			break;
		}
		default:
		{
			DEBUG (("kern_pr_readlink ([%u:%ld]): not a symlink", dir->dev, dir->index));
			return EINVAL;
		}
	}

	strncpy_f (name, buf, namelen);

	len = strlen (buf);
	if (len >= namelen)
	{
		DEBUG (("kern_proc_readlink, dir->index: %ld, name: %s: name too long", dir->index, buf));
		return EBADARG;
	}

	TRACE (("kern_proc_readlink, dir->index: %ld, name: %s", dir->index, name));
	return E_OK;
}

INLINE long
kern_fddir_readlink (fcookie *file, char *name, int namelen)
{
	short pid = file->index >> 16;
	PROC *p;
	struct filedesc *fd;
	char buf[64];
	ulong len;
	long desc;

	TRACE (("kern_fddir_readlink ([%u:%ld])", file->dev, file->index));

	p = pid2proc (pid);
	if (!p)
	{
		DEBUG (("kern_fddir_readlink: /sys/%d: No such file or directory", pid));
		return ENOENT;
	}

	fd = p->p_fd;
	if (!fd)
	{
		DEBUG (("kern_fddir_readlink: pid %d, no fd table", pid));
		return ENOENT;
	}

	if (follow_link_denied (p, __FUNCTION__))
		return EACCES;

	/* This cast will preserve the sign */
	desc = (char) file->index;

	if (desc < -5 || desc >= fd->nfiles || (*(fd->ofiles + desc)) == NULL)
	{
		DEBUG (("kern_fddir_readlink: /sys/%d/fd/%d: Invalid descriptor", pid, (int) desc));
		return ENOENT;
	}

	ksprintf (buf, sizeof (buf), "[%04u]:%lu",
			(unsigned) fd->ofiles[desc]->fc.dev,
			(ulong) fd->ofiles[desc]->fc.index);

	strncpy_f (name, buf, namelen);

	len = strlen (buf);
	if (len >= namelen)
	{
		DEBUG (("kern_fddir_readlink: /sys/%d/fd/<desc> -> %s: Name too long", pid, buf));
		return EBADARG;
	}

	return E_OK;
}

static long _cdecl
kern_readlink (fcookie *file, char *name, int namelen)
{
	TRACE (("kern_readlink ([%u:%ld])", file->dev, file->index));

	if ((file->index & 0xffff0000) && (file->index & 0xff00) == 0x100)
		return kern_fddir_readlink (file, name, namelen);

	if (file->index & 0xffff0000)
		return kern_proc_readlink (file, name, namelen);

	if (file->index == ROOTDIR_SELF)
	{
		char buf[5];
		int len;

		ksprintf (buf, sizeof (buf), "%d", get_curproc()->pid);
		strncpy_f (name, buf, namelen);

		len = strlen (buf);

		if (len >= namelen)
		{
			DEBUG (("kern_readlink ([%u:%ld]) -> %s, name too long",
				file->dev, file->index, buf));
			return ENAMETOOLONG;
		}

		TRACE (("kern_pr_readlink, dir->index: %ld", file->index));
		return E_OK;
	}

	DEBUG (("kern_readlink ([%u:%ld]): not a symlink", file->dev, file->index));
	return EINVAL;
}

static long _cdecl
kern_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	UNUSED (dir);
	UNUSED (name);

	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "kern");
			break;;
		}
		case FS_INFO:
		{
			struct fs_info *info;

			info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "kern");
				info->version = ((long) MINT_MAJ_VERSION << 16) | MINT_MIN_VERSION;
				info->type = 0;
				strcpy (info->type_asc, "kern pseudo-filesystem");
			}

			break;
		}
		default:
		{
			DEBUG (("kern_fscntl: unknown opcode 0x%04x", (unsigned) cmd));
			return ENOSYS;
		}
	}

	return E_OK;
}


/* Main device driver */

static long _cdecl
kern_open (FILEPTR *f)
{
	PROC *p = NULL;

	TRACE (("kern_open: inode: %lu, flags: 0x%04x", f->fc.index, f->flags));

	if ((f->flags & O_RWMODE) != O_RDONLY)
	{
		DEBUG (("kern_open: read-only"));
		return EACCES;
	}

	if (f->fc.index & 0xffff0000)
	{
		short pid = f->fc.index >> 16;

		TRACE (("kern_open (proc): inode: %lu, flags: 0x%04x", f->fc.index & 0xffff, f->flags));

		p = pid2proc (pid);
		if (!p)
		{
			DEBUG (("kern_open (proc): pid %d: No such process", pid));
			return ENOENT;
		}
	}

	if (f->links <= 1)
	{
		KENTRY *t;

		if (p)	t = search_inode (procdir, f->fc.index & 0xffff);
		else	t = search_inode (rootdir, f->fc.index);

		if (!t || !(t->mode & S_IFREG))
			return EACCES;

		f->devinfo = (long) t;
	}

	return E_OK;
}

static long _cdecl
kern_close (FILEPTR *f, int pid)
{
	UNUSED (pid);

	TRACE (("kern_close: inode: %lu, flags: 0x%04x", f->fc.index, f->flags));

	if (f->links <= 0)
		f->devinfo = 0;

	return E_OK;
}

static long _cdecl
kern_read (FILEPTR *f, char *buf, long bytes)
{
	PROC *p = NULL;
	KENTRY *t = (KENTRY *) f->devinfo;
	SIZEBUF *info = NULL;
	long bytes_read = 0;
	long ret;
	char *crs;

	TRACE (("kern_read: inode: %lu, read %ld bytes", f->fc.index, bytes));

	if (bytes < 0)
	{
		DEBUG (("kern_read (inode %lu): attempt to read %ld bytes", f->fc.index, bytes));
		return EINVAL;
	}

	if (!t)
	{
		ALERT ("kernfs: internal problem, t == NULL");
		return 0;
	}

	if (f->fc.index & 0xffff0000)
	{
		short pid = f->fc.index >> 16;

		TRACE (("kern_read (proc): inode: %lu, flags: 0x%04x", f->fc.index & 0xffff, f->flags));

		p = pid2proc (pid);
		if (!p)
		{
			DEBUG (("kern_read (proc): pid %d: No such process", pid));

			/* XXX - any better error code? */
			return ENOENT;
		}
	}

	/* fill the buffer */
	if (p)	ret = (*t->get)(&info, p);
	else	ret = (*t->get)(&info);

	if (ret || !info)
		return ret;

	if (bytes + f->pos > info->len)
		bytes = info->len - f->pos;

	crs = info->buf + f->pos;

	while (bytes-- > 0)
	{
		*buf++ = *crs++;
		bytes_read++;
	}

	/* free the buffer */
	kfree (info);

	f->pos += bytes_read;
	return bytes_read;
}

static long _cdecl
kern_ioctl (FILEPTR *f, int mode, void *buf)
{
	DEBUG (("kern_ioctl: inode: %lu, cmd: %d", f->fc.index, mode));

	switch (mode)
	{
		case FIONREAD:
		{
			*((long *) buf) = 1;
			break;
		}
		case FIONWRITE:
		{
			*((long *) buf) = 0;
			break;
		}
		default:
		{
			DEBUG (("kern_ioctl: inode: %lu, invalid opcode: %d",  f->fc.index, mode));
			return EINVAL;
		}
	}

	return E_OK;
}

/* Maybe lseek is overkill altogether on our files */
static long _cdecl
kern_lseek (FILEPTR *f, long where, int whence)
{
	long newpos;

	DEBUG (("kern_lseek: inode: %lu, where: %ld, whence: %d",
	        f->fc.index, where, whence));

	switch (whence)
	{
		case SEEK_SET:
			newpos = where;
			break;
		case SEEK_CUR:
			newpos = f->pos + where;
			break;
		case SEEK_END:
			newpos = 0 + where;
			break;
		default:
			DEBUG (("kern_lseek: inode: %lu, invalid whence argument: %d", f->fc.index, whence));
			return EINVAL;
	}

	if (newpos < 0)
	{
		DEBUG (("kern_lseek: inode: %lu, invalid position %ld for whence %d", f->fc.index, where, whence));
		return EINVAL;
	}

	f->pos = newpos;
	return newpos;
}

static long _cdecl
kern_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	DEBUG (("kern_datime: inode: %lu, %s mode", f->fc.index, flag ? "write" : "read"));

	switch (flag)
	{
		case 0:
		{
			short pid = f->fc.index >> 16;
			PROC *p = pid2proc (pid);

			if (p == NULL)
				p = rootproc;

			*(long *) timeptr = p->started.tv_sec;
			break;
		}
		case 1:
		{
			DEBUG (("kern_datime: get your fingers off here!"));
			return EPERM;
		}
		default:
			return EBADARG;
	}

	return E_OK;
}

/* If FC is a cookie for a symbolic link on the sys pseudo-filesystem
 * resolve the link into FC.  Returns 0 on success or a negative error
 * number on failure.  Possible error conditions:
 *
 * EINVAL - Not a symbolic link.  This can actuall not happen because
 *	    the cookie passed to us must be the result of a kernel lookup.
 * ELOOP  - The maximum link count was reached.
 * EACCES - The caller doesn't have appropriate privileges to follow
 *	    the link.
 * ENOENT - The process that was associated with the cookie died
 *	    or the file that a soft link in /sys/<PID>/fd points
 *	    to has been closed (this assuming that the kernel
 *	    wouldn't even ask us to follow the link if it didn't
 *	    exist earlier).
 *
 * If the target of the symbolic link is another symbolic link than
 * all
 *
 * There are not that many symlinks on the sys filesystem:
 *
 * - /sys/<PID>/self:		 Trivial case.  Do the work for
 *				 relpath2cookie and return.
 * - /sys/<PID>/cwd,
 *   /sys/<PID>/exe,
 *   /sys/<PID>/root:		 Duplicate the corresponding cookie from the
 *		        	 proc structure.
 * - /sys/<PID>/fd/<DESCRIPTOR>: Duplicate the cookie for the open
 *				 descriptor.
 *
 * NOTE: It is sufficient to check whether the caller has permission to follow
 * the link (remember that all links on this filesystem - except for /sys/self -
 * have the special permissions 0500).  Either the caller has superuser
 * privileges and doesn't have to give a fuck about other folks' rights,
 * or it is the owner of the link.  Thus, you can never gain more rights
 * than you have by opening links in /sys.
 *
 * For security considerations on the /sys/self/root link, see the
 * discussion in the manpage sys(5).
 */
int
kern_follow_link (fcookie *fc, int depth)
{
	long pid;		/* PID associated with the descriptor */
	PROC *p;		/* Associated proc structure */
	fcookie *src = NULL;	/* The cookie we have to duplicate */
	struct filedesc *fd;
	struct cwd *cwd;

	TRACE (("kern_follow_link ([%u:%ld]), depth %d", fc->dev, fc->index, depth));

	if (depth > MAX_LINKS)
	{
		DEBUG (("kern_follow_link: Too many symbolic links"));
		return ELOOP;
	}

	if (fc->index == ROOTDIR_SELF)
	{
		fc->index = (((long) get_curproc()->pid) << 16) | PROCDIR_DIR;
		return 0;
	}

	pid = fc->index >> 16;
	p = pid2proc (pid);
	if (p == NULL)
	{
		DEBUG (("kern_follow_link: pid %ld: No such process", pid));
		return ENOENT;
	}

	fd = p->p_fd;
	cwd = p->p_cwd;

	if (!fd || !cwd)
		return ENOENT;

	if ((fc->index & 0xff00) == 0x100)
	{
		char desc = fc->index & 0xff;

		if (desc < -5 || desc >= fd->nfiles || *(fd->ofiles + desc) == NULL)
		{
			DEBUG (("kern_follow_link: pid %ld has closed descriptor %d", pid, (int) desc));
			return ENOENT;
		}

		src = &(*(fd->ofiles + desc))->fc;
	}
	else if ((fc->index & 0xffff) == PROCDIR_CWD)
	{
		src = cwd->curdir + cwd->curdrv;
	}
	else if ((fc->index & 0xffff) == PROCDIR_EXE)
	{
		src = &p->exe;
	}
	else if ((fc->index & 0xffff) == PROCDIR_ROOT)
	{
		src = &cwd->rootdir;
	}

	if (src == NULL)
	{
		DEBUG (("kern_follow_link: internal error: inode 0x%08lx not a symbolic link", (ulong) fc->index));
		return EINVAL;
	}

	if (follow_link_denied (p, __FUNCTION__))
		return EACCES;

	/* This happens with "root" very often */
	if (src->dev == 0 && src->index == 0)
		src = &cwd->root[UNIDRV];

	release_cookie (fc);
	dup_cookie (fc, src);

	TRACE (("kern_follow_link returning cookie for [%u:%ld]", fc->dev, fc->index));
	return 0;
}

# endif /* WITH_KERNFS */
