/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corp.
 * All rights reserved.
 */

/*
 * various file system interface things
 */

# include "filesys.h"
# include "global.h"

# include "arch/mprot.h"
# include "libkern/libkern.h"
# include "mint/basepage.h"
# include "mint/signal.h"

# include "biosfs.h"
# include "block_IO.h"
# include "fatfs.h"
# include "kernfs.h"
# include "pipefs.h"
# include "procfs.h"
# include "ramfs.h"
# include "shmfs.h"
# include "tosfs.h"
# include "unifs.h"

# include "bios.h"
# include "dosdir.h"
# include "dosfile.h"
# include "dosmem.h"
# include "kmemory.h"
# include "memory.h"
# include "module.h"
# include "proc.h"
# include "signal.h"
# include "time.h"

# include <osbind.h>


#if 1
#define PATH2COOKIE_DB(x) TRACE(x)
#else
#define PATH2COOKIE_DB(x) DEBUG(x)
#endif

# ifdef DEBUG_INFO
# define DMA_DEBUG(x)	FORCE x
# else
# define DMA_DEBUG(x)
# endif


# ifdef NONBLOCKING_DMA
static void
xfs_block_level_0 (FILESYS *fs, ushort dev, const char *func)
{
	while (fs->lock)
	{
		fs->sleepers++;
		DMA_DEBUG (("level 0: sleep on %lx, %c (%s, %i)", fs, 'A'+dev, func, fs->sleepers));
		sleep (IO_Q, (long) fs);
		fs->sleepers--;
	}
	
	fs->lock = 1;
}

static void
xfs_deblock_level_0 (FILESYS *fs, ushort dev, const char *func)
{
	fs->lock = 0;
	
	if (fs->sleepers)
	{
		DMA_DEBUG (("level 0: wake on %lx, %c (%s, %i)", fs, 'A'+dev, func, fs->sleepers));
		wake (IO_Q, (long) fs);
	}
}

static void
xfs_block_level_1 (FILESYS *fs, ushort dev, const char *func)
{
	register ulong bit = 1UL << dev;
	
	while (fs->lock & bit)
	{
		fs->sleepers++;
		DMA_DEBUG (("level 1: sleep on %lx, %c (%s, %i)", fs, 'A'+dev, func, fs->sleepers));
		sleep (IO_Q, (long) fs);
		fs->sleepers--;
	}
	
	fs->lock |= bit;
}

static void
xfs_deblock_level_1 (FILESYS *fs, ushort dev, const char *func)
{
	fs->lock &= ~(1UL << dev);
	
	if (fs->sleepers)
	{
		DMA_DEBUG (("level 1: wake on %lx, %c (%s, %i)", fs, 'A'+dev, func, fs->sleepers));
		wake (IO_Q, (long) fs);
	}
}

static volatile ushort xfs_sema_lock = 0;
static volatile ushort xfs_sleepers = 0;

void
xfs_block (FILESYS *fs, ushort dev, const char *func)
{
	if (fs->fsflags & FS_EXT_2)
	{
		(*fs->block)(fs, dev, func);
	}
	else
	{
		while (xfs_sema_lock)
		{
			xfs_sleepers++;
			DMA_DEBUG (("[%c: -> %lx] sleep on xfs_sema_lock (%s, %i)", 'A'+dev, fs, func, xfs_sleepers));
			sleep (IO_Q, (long) &xfs_sema_lock);
			xfs_sleepers--;
		}
		
		xfs_sema_lock = 1;
	}
}

void
xfs_deblock (FILESYS *fs, ushort dev, const char *func)
{
	if (fs->fsflags & FS_EXT_2)
	{
		(*fs->deblock)(fs, dev, func);
	}
	else
	{
		xfs_sema_lock = 0;
		
		if (xfs_sleepers)
		{
			DMA_DEBUG (("[%c: -> %lx] wake on xfs_sema_lock (%s, %i)", 'A'+dev, fs, func, xfs_sleepers));
			wake (IO_Q, (long) &xfs_sema_lock);
		}
	}
}
# endif

static void
xfs_blocking_init (FILESYS *fs)
{
# ifdef NONBLOCKING_DMA
	if (fs->fsflags & FS_EXT_2)
	{
		if (!(fs->fsflags & FS_REENTRANT_L1))
		{
			fs->block = xfs_block_level_0;
			fs->deblock = xfs_deblock_level_0;
		}
		else if (!(fs->fsflags & FS_REENTRANT_L2))
		{
			fs->block = xfs_block_level_1;
			fs->deblock = xfs_deblock_level_1;
		}
		else
		{
			fs->block = NULL;
			fs->deblock = NULL;
		}
	}
# endif
}

INLINE long
xfs_sync (FILESYS *fs)
{
# ifdef NONBLOCKING_DMA
	if (!(fs->fsflags & FS_REENTRANT_L2))
	{
		register long r;
		
		if ((fs->fsflags & FS_EXT_2) && (fs->fsflags & FS_REENTRANT_L1))
		{
			while (fs->lock)
			{
				fs->sleepers++;
				DMA_DEBUG (("special sync: sleep on %lx, %i (%s, %i)", fs, "xfs_sync", fs->sleepers));
				sleep (IO_Q, (long) fs);
				fs->sleepers--;
			}
			
			fs->lock |= 0xffffffff;
		}
		else
			xfs_lock (fs, 0, "xfs_sync");
		
		r = (*fs->sync)();
		
		if ((fs->fsflags & FS_EXT_2) && (fs->fsflags & FS_REENTRANT_L1))
		{
			fs->lock = 0;
			
			if (fs->sleepers)
			{
				DMA_DEBUG (("special sync: wake on %lx (%s, %i)", fs, "xfs_sync", fs->sleepers));
				wake (IO_Q, (long) fs);
			}
		}
		else
			xfs_unlock (fs, 0, "xfs_sync");
		
		return r;
	}
	else
# endif
		return (*fs->sync)();
}

long
getxattr (FILESYS *fs, fcookie *fc, XATTR *xattr)
{
	STAT stat;
	long r;
	
	assert (fs->fsflags & FS_EXT_3);
	
	r = xfs_stat64 (fs, fc, &stat);
	if (!r)
	{
		xattr->mode	= stat.mode;
		xattr->index	= stat.ino;
		xattr->dev	= stat.dev;
		xattr->rdev	= stat.rdev;
		xattr->nlink	= stat.nlink;
		xattr->uid	= stat.uid;
		xattr->gid	= stat.gid;
		xattr->size	= stat.size;
		xattr->blksize	= stat.blksize;
		xattr->nblocks	= (stat.blksize < 512) ? stat.blocks :
					stat.blocks / (stat.blksize >> 9);
		
		*((long *) &(xattr->mtime)) = stat.mtime.time;
		*((long *) &(xattr->atime)) = stat.atime.time;
		*((long *) &(xattr->ctime)) = stat.ctime.time;
		
		xattr->attr	= 0;
		
		/* fake attr field a little bit */
		if (S_ISDIR (stat.mode))
			xattr->attr = FA_DIR;
		else if (!(stat.mode & 0222))
			xattr->attr = FA_RDONLY;;
		
		xattr->reserved2 = 0;
		xattr->reserved3[0] = 0;
		xattr->reserved3[1] = 0;
	}
	
	return r;
}

# if 0
long
getstat64 (FILESYS *fs, fcookie *fc, STAT *ptr)
{
	XATTR xattr;
	long r;
	
	assert (fs->getxattr);
	
	r = xfs_getxattr (fs, fc, &xattr);
	if (!r)
	{
		stat->dev	= xattr.dev;
		stat->ino	= xattr.index;
		stat->mode	= xattr.mod;
		stat->nlink	= xattr.nlink;
		stat->uid	= xattr.uid;
		stat->gid	= xattr.gid;
		stat->rdev	= xattr.rdev;
		
		/* no native UTC extension
		 * -> convert to unix UTC
		 */
		stat->atime.time = unixtime (xattr.atime, xattr.adate) + timezone;
		stat->mtime.time = unixtime (xattr.mtime, xattr.mdate) + timezone;
		stat->ctime.time = unixtime (xattr.ctime, xattr.cdate) + timezone;
		
		stat->size	= xattr.size;
		stat->blocks	= (xattr.blksize < 512) ? xattr.nblocks :
					xattr.nblocks * (xattr.blksize >> 9);
		stat->blksize	= xattr.blksize;
		
		stat->flags	= 0;
		stat->gen	= 0;
		
		bzero (stat->res, sizeof (stat->res));
	}
	
	return r;
}
# endif

FILESYS *active_fs;
FILESYS *drives [NUM_DRIVES];

/* "aliased" drives are different names
 * for real drives/directories
 * if drive d is an alias for c:\usr,
 * then alias_drv[3] == 2 (the real
 * drive) and aliases has bit (1L << 3)
 * set.
 * NOTE: if aliasdrv[d] is 0, then d is not an aliased drive,
 * otherwise d is aliased to drive aliasdrv[d]-1
 * (e.g. if drive A: is aliased to B:\FOO, then
 * aliasdrv[0] == 'B'-'A'+1 == 2). Always remember to
 * compensate for the extra 1 when dereferencing aliasdrv!
 */
int aliasdrv[NUM_DRIVES];

/* vector of valid drives, according to GEMDOS */
/* note that this isn't necessarily the same as what the BIOS thinks of
 * as valid
 */
long dosdrvs;

/*
 * Initialize a specific drive. This is called whenever a new drive
 * is accessed, or when media change occurs on an old drive.
 * Assumption: at this point, active_fs is a valid pointer
 * to a list of file systems.
 */

static void
init_drive (int i)
{
	long r;
	FILESYS *fs;
	fcookie root_dir;
	
	TRACE (("init_drive (%c)", i+'A'));
	
	assert (i >= 0 && i < NUM_DRIVES);
	
	drives[i] = 0;		/* no file system */
	if (dlockproc[i])
		return;
	
	for (fs = active_fs; fs; fs = fs->next)
	{
		r = xfs_root (fs, i, &root_dir);
		if (r == 0)
		{
			drives[i] = root_dir.fs;
			release_cookie (&root_dir);
			break;
		}
	}
}

/*
 * initialize the file system
 */

void
init_filesys (void)
{
	int i;
	
	active_fs = NULL;
	
	/* init data structures */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		drives[i] = NULL;
		aliasdrv[i] = 0;
	}
	
	/* get the vector of connected GEMDOS drives */
	dosdrvs = Dsetdrv (Dgetdrv ()) | drvmap ();
	
	
# ifdef OLDTOSFS
	xfs_add (&tos_filesys);
# endif
	xfs_add (&fatfs_filesys);
	xfs_add (&bios_filesys);
	xfs_add (&pipe_filesys);
	xfs_add (&proc_filesys);
	xfs_add (&ramfs_filesys);
	xfs_add (&shm_filesys);
# ifdef WITH_KERNFS
	xfs_add (&kern_filesys);
# endif
	xfs_add (&uni_filesys);
	
	
	/* initialize the BIOS file system */
	biosfs_init ();
	
	/* initialize the proc file system */
	procfs_init ();
	
	/* initialize the ramdisk file system */
	ramfs_init ();
	
	/* initialize the shared memory file system */
	shmfs_init ();
	
	/* initialize the unified file system */
	unifs_init ();
	
	/* initialize the main file system */
	fatfs_init ();
}

# ifdef DEBUG_INFO
char *
xfs_name (fcookie *fc)
{
	static char buf [SPRINTF_MAX];
	long r;
	
	buf [0] = '\0';
	
	r = xfs_fscntl (fc->fs, fc, buf, MX_KER_XFSNAME, (long) buf);
	if (r)
		ksprintf (buf, sizeof (buf), "unknown (%lx -> %li)", fc->fs, r);
	
	return buf;
}

/* uk: go through the list of file systems and call their sync() function
 *     if they wish to.
 */

long
_s_ync (void)
{
	FILESYS *fs;
	
	ALERT ("Syncing file systems...");
	
	/* syncing filesystems */
	for (fs = active_fs; fs; fs = fs->next)
	{
		if (fs->fsflags & FS_DO_SYNC)
			(*fs->sync)();
	}
	
	/* always syncing buffercache */
	bio_sync_all ();
	
	ALERT ("Syncing done.");
	return 0;
}
# endif

long _cdecl
s_ync (void)
{
	FILESYS *fs;
	
	TRACE (("Syncing file systems..."));
	
	/* syncing filesystems */
	for (fs = active_fs; fs; fs = fs->next)
	{
		if (fs->fsflags & FS_DO_SYNC)
			xfs_sync (fs);
	}
	
	/* always syncing buffercache */
	bio_sync_all ();
	
	TRACE (("Syncing done."));
	return 0;
}

long _cdecl
sys_fsync (int fh)
{
	/* dummy function at the moment */
	return s_ync ();
}

/*
 * load file systems from disk
 * this routine is called after process 0 is set up, but before any user
 * processes are run
 */

void
xfs_add (FILESYS *fs)
{
	if (fs)
	{
		xfs_blocking_init (fs);
		
		fs->next = active_fs;
		active_fs = fs;
	}
}

static void *
callout_init (void *initfunction, struct kerinfo *k)
{
	register void *ret __asm__("d0");
	
	__asm__ volatile
	(
		"moveml d3-d7/a3-a6,sp@-;"
		"movl	%2,sp@-;"
		"movl   %1,a0;"
		"jsr    a0@;"
		"addqw  #4,sp;"
		"moveml sp@+,d3-d7/a3-a6;"
		: "=r"(ret)				/* outputs */
		: "g"(initfunction), "r"(k)		/* inputs  */
		: "d0", "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */
		  AND_MEMORY
	);
	
	return ret;
}

static void
load_xfs (const char *path, const char *name)
{
	BASEPAGE *b;
	long r;
	
	DEBUG (("load_xfs: enter (%s, %s)", path, name));
	
	b = load_module (path, &r);
	if (b)
	{
		FILESYS *fs;
		
		DEBUG (("load_xfs: initializing %s, bp = %lx, size = %lx", path, b, b->p_tlen + b->p_dlen + b->p_blen));
		
		fs = callout_init ((void *) b->p_tbase, &kernelinfo);
		if (fs)
		{
			DEBUG (("load_xfs: %s loaded OK (%lx)", path, fs));
			
			/* link it into the list of drivers
			 * 
			 * uk: but only if it has not installed itself
			 *     via Dcntl() after checking if file
			 *     system is already installed, so we know
			 *     for sure that each file system in at
			 *     most once in the chain (important for
			 *     removal!)
			 * also note: this doesn't preclude loading
			 * two different instances of the same file
			 * system driver, e.g. it's perfectly OK to
			 * have a "cdromy1.xfs" and "cdromz2.xfs";
			 * the check below just makes sure that a
			 * given instance of a file system is
			 * installed at most once. I.e., it prevents
			 * cdromy1.xfs from being installed twice.
			 */
			if ((FILESYS *) 1L != fs)
			{
				FILESYS *f;
				
				for (f = active_fs; f; f = f->next)
					if (f == fs)
						break;
				
				if (!f)
				{
					/* we ran completly through the list
					 */
					
					DEBUG (("load_xfs: xfs_add (%lx)", fs));
					xfs_add (fs);
				}
			}
		}
		else
		{
			kfree (b);
			DEBUG (("load_xfs: %s returned null", path));
		}
	}
}

/*
 * uk: load device driver in files called *.xdd (external device driver)
 *     from disk
 * maybe this should go into biosfs.c ??
 *
 * this routine is called after process 0 is set up, but before any user
 * processes are run, but before the loadable file systems come in,
 * so they can make use of external device drivers
 */
static void
load_xdd (const char *path, const char *name)
{
	BASEPAGE *b;
	long r;
	
	
	DEBUG (("load_xdd: enter (%s, %s)", path, name));
	
	b = load_module (path, &r);
	if (b)
	{
		DEVDRV *dev;
		
		DEBUG (("load_xdd: initializing %s, bp = %lx, size = %lx", path, b, b->p_tlen + b->p_dlen + b->p_blen));
		
		dev = callout_init ((void *) b->p_tbase, &kernelinfo);
		if (dev)
		{
			DEBUG (("load_xdd: %s loaded OK", path));
			
			if ((DEVDRV *) 1L != dev)
			{
				/* we need to install the device driver ourselves */
				
				char dev_name[PATH_MAX];
				struct dev_descr the_dev;
				int i;
				long r;
				
				DEBUG (("load_xdd: installing %s itself!\7", path));
				
				bzero (&the_dev, sizeof (the_dev));
				the_dev.driver = dev;
				
				strcpy (dev_name, "u:\\dev\\");
				i = strlen (dev_name);
				
				while (*name && *name != '.')
					dev_name [i++] = tolower (*name++);
				
				dev_name [i] = '\0';
				
				DEBUG (("load_xdd: final -> %s", dev_name));
				
				r = d_cntl (DEV_INSTALL, dev_name, (long) &the_dev);
				if (r <= 0)
				{
					kfree (b);
					DEBUG (("load_xdd: fail, d_cntl(%s) = %li", dev_name, r));
				}
			}
		}
		else
		{
			kfree (b);
			DEBUG (("load_xdd: %s returned null", path));
		}
	}
}

static long
_d_opendir (DIR *dirh, const char *name)
{
	long r;
	
	r = path2cookie (name, follow_links, &dirh->fc);
	if (r == E_OK)
	{
		dirh->index = 0;
		dirh->flags = 0;
		
		r = xfs_opendir (dirh->fc.fs, dirh, 0);
		if (r) release_cookie (&dirh->fc);
	}
	
	return r;
}

static void
_d_closedir (DIR *dirh)
{
	if (dirh->fc.fs)
	{
		xfs_closedir (dirh->fc.fs, dirh);
		release_cookie (&dirh->fc);
	}
}

static long
_d_readdir (DIR *dirh, char *buf, int len)
{
	fcookie fc;
	long r;
	
	if (!dirh->fc.fs)
		return EBADF;
	
	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (!r) release_cookie (&fc);
	
	return r;
}

static const char *paths [] =
{
	".",
	"\\MINT",
	"\\MULTITOS"
};

static const char *types [] =
{
	".xdd",
	".xfs"
};

static void (*loads [])(const char *, const char *) =
{
	load_xdd,
	load_xfs
};

void
load_modules (long type)
{
	long i;
	
	DEBUG (("load_modules: enter (%li)", type));
	
	if ((type < 0) || (type >= (sizeof (types) / sizeof (*types))))
	{
		DEBUG (("load_modules: leave (invalid type)"));
		return;
	}
	
	for (i = 0; i < (sizeof (paths) / sizeof (*paths)); i++)
	{
		char buf [128];
		long len;
		char *name;
		DIR dirh;
		long r;
		
		strcpy (buf, paths [i]);
		len = strlen (buf);
		buf [len++] = '\\';
		buf [len] = '\0';
		name = buf + len;
		len = 128 - len;
		
		r = _d_opendir (&dirh, buf);
		DEBUG (("load_modules: d_opendir (%s) = %li", buf, r));
		
		if (r == 0)
		{
			r = _d_readdir (&dirh, name, len);
			DEBUG (("load_modules: d_readdir = %li (%s)", r, name+4));
			
			while (r == 0)
			{
				r = strlen (name+4) - 4;
				if ((r > 0) && !stricmp (name+4 + r, types [type]))
				{
					char *ptr1 = name;
					char *ptr2 = name+4;
					long len2 = len - 1;
					
					while (*ptr2)
					{
						*ptr1++ = *ptr2++;
						len2--; assert (len2);
					}
					
					*ptr1 = '\0';
					
					DEBUG (("load_modules: load \"%s\" (%s) [%s]", buf, name, types [type]));
					(*(loads [type]))(buf, name);
					DEBUG (("load_modules: load done \"%s\" (%s) [%s]", buf, name, types [type]));
				}
				
				r = _d_readdir (&dirh, name, len);
				DEBUG (("load_modules: d_readdir = %li (%s)", r, name+4));
			}
			
			_d_closedir (&dirh);
		}
	}
	
	DEBUG (("load_modules: finished, i = %li", i));
}


void
close_filesys (void)
{
	PROC *p;
	
	TRACE (("close_filesys"));
	
	/* close every open file */
	for (p = proclist; p; p = p->gl_next)
	{
		int i;
		
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		
		for (i = MIN_HANDLE; i < MAX_OPEN; i++)
		{
			FILEPTR *f;
			
			f = p->handle [i];
			if (f)
			{
				if (p->wait_q == TSR_Q || p->wait_q == ZOMBIE_Q)
					ALERT ("Open file for dead process?");
				
				do_pclose (p, f);
			}
		}
	}
}

/*
 * "media change" routine: called when a media change is detected on device
 * d, which may or may not be a BIOS device. All handles associated with
 * the device are closed, and all directories invalidated. This routine
 * does all the dirty work, and is called automatically when
 * disk_changed detects a media change.
 */

void _cdecl 
changedrv (ushort d)
{
	PROC *p;
	int i;
	FILEPTR *f;
	FILESYS *fs;
	SHTEXT *stext, **old;
	extern SHTEXT *text_reg;	/* in mem.c */
	DIR *dirh;
	fcookie dir;
	int warned = (d & 0xf000) == PROC_RDEV_BASE;
	long r;
	
	TRACE (("changedrv (%u)", d));
	
	/* if an aliased drive, change the *real* device */
	if (d < NUM_DRIVES && aliasdrv[d])
		d = aliasdrv[d] - 1;
	
	/* re-initialize the device, if it was a BIOS device */
	if (d < NUM_DRIVES)
	{
		fs = drives[d];
		if (fs)
		{
			TRACE (("changedrv: force change"));
			xfs_dskchng (fs, d, 1);
		}
		
		init_drive (d);
	}
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		
		/* invalidate all open files on this device */
		for (i = MIN_HANDLE; i < MAX_OPEN; i++)
		{
			if (((f = p->handle[i]) != 0) &&
				(f != (FILEPTR *)1) && (f->fc.dev == d))
			{
			    if (!warned)
			    {
				ALERT ("Files were open on a changed drive (0x%x)!", d);
				warned++;
			    }

/* we set f->dev to NULL to indicate to do_pclose that this is an
 * emergency close, and that it shouldn't try to make any
 * calls to the device driver since the file has gone away
 */
			    f->dev = NULL;
			    (void)do_pclose(p, f);
/* we could just zero the handle, but this could lead to confusion if
 * a process doesn't realize that there's been a media change, Fopens
 * a new file, and gets the same handle back. So, we force the
 * handle to point to /dev/null.
 */
			    p->handle[i] =
				do_open("U:\\DEV\\NULL", O_RDWR, 0, NULL, NULL);
			}
		}
		
		/* terminate any active directory searches on the drive */
		for (i = 0; i < NUM_SEARCH; i++)
		{
			dirh = &p->srchdir[i];
			if (p->srchdta[i] && dirh->fc.fs && dirh->fc.dev == d)
			{
				TRACE (("closing search for process %d", p->pid));
				release_cookie (&dirh->fc);
				dirh->fc.fs = 0;
				p->srchdta[i] = 0;
			}
		}

		for (dirh = p->searches; dirh; dirh = dirh->next)
		{
			/* If this search is on the changed drive, release
			 * the cookie, but do *not* free it, since the
			 * user could later call closedir on it.
			 */
			if (dirh->fc.fs && dirh->fc.dev == d)
			{
				release_cookie (&dirh->fc);
				dirh->fc.fs = 0;
			}
		}
		
		if (d >= NUM_DRIVES)
			continue;
		
		/* change any active directories on the device to the (new) root */
		fs = drives[d];
		if (fs)
		{
			r = xfs_root (fs, d, &dir);
			if (r != E_OK) dir.fs = 0;
		}
		else
		{
			dir.fs = 0; dir.dev = d;
		}
		
		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (p->root[i].dev == d)
			{
				release_cookie (&p->root[i]);
				dup_cookie (&p->root[i], &dir);
			}
			if (p->curdir[i].dev == d)
			{
				release_cookie (&p->curdir[i]);
				dup_cookie (&p->curdir[i], &dir);
			}
		}
		
		/* hmm, what we can do if the drive changed
		 * that hold our root dir?
		 */
		if (p->root_dir)
		{
			release_cookie (&p->root_fc);
			p->root_fc.fs = 0;
			kfree (p->root_dir);
			p->root_dir = NULL;
			
			post_sig (p, SIGKILL);
		}
		
		release_cookie (&dir);
	}
	
	/* free any file descriptors associated with shared text regions */
	for (old = &text_reg; 0 != (stext = *old);)
	{
		f = stext->f;
		if (f->fc.dev == d)
		{
			f->dev = NULL;
			do_pclose(rootproc, f);
			stext->f = 0;
			
			/* free region if unattached */
			if (stext->text->links == 0xffff)
			{
				stext->text->links = 0;
				stext->text->mflags &= ~(M_SHTEXT|M_SHTEXT_T);
				free_region (stext->text);
				*old = stext->next;
				kfree (stext);
				
				continue;
			}
			
			/* else clear `sticky bit' */
			stext->text->mflags &= ~M_SHTEXT_T;
		}
		old = &stext->next;
	}
}

/*
 * check for media change: if the drive has changed, call changedrv to
 * invalidate any open files and file handles associated with it, and
 * call the file system's media change routine.
 * returns: 0 if no change, 1 if change, negative number for error
 */

long
disk_changed (ushort d)
{
	long r;
	FILESYS *fs;
	static char tmpbuf[8192];
	
	TRACE (("disk_changed (%u)", d));
	
	/* watch out for aliased drives */
	if (d < NUM_DRIVES && aliasdrv[d])
		d = aliasdrv[d] - 1;
	
	/* for now, only check BIOS devices */
	if (d >= NUM_DRIVES)
		return 0;
	
	/* has the drive been initialized yet? If not, then initialize it and return
	 * "no change"
	 */
	fs = drives[d];
	if (!fs)
	{
		TRACE (("drive %c not yet initialized", d+'A'));
		changedrv (d);
		return 0;
	}
	
	/* fn: other strategie if FS_OWN_MEDIACHANGE is set:
	 *     xfs check itself for a mediachange (dskchng do this)
	 */
	if (fs->fsflags & FS_OWN_MEDIACHANGE)
	{
		/* ask xfs */
		long i = xfs_dskchng (fs, d, 0);
		
		/* if drive changed, invalidate the drive */
		if (i)
		{
			drives[d] = 0;
			changedrv (d);
		}
		
		return i;
	}
	
	/* We have to do this stuff no matter what, because someone may have installed
	 * vectors to force a media change...
	 * PROBLEM: AHDI may get upset if the drive isn't valid.
	 * SOLUTION: don't change the default PSEUDODRIVES setting!
	 */
	
	TRACE (("calling mediach (%d)", d));
	r = mediach (d);
	TRACE (("mediach (%d) == %li", d, r));
	
	if (r < 0)
	{
		/* KLUDGE: some .XFS drivers don't install BIOS vectors, and
		 * so we'll always get ENODEV back from them. This isn't
		 * recommended (since there are other programs than MiNT that
		 * may ask for BIOS functions from any installed drives). This
		 * is a temporary work-around until those .XFSes are changed
		 * to either install BIOS vectors or to use the new U: Dcntl()
		 * calls to install themselves.
		 * Note that ENODEV must be tested for drives A-C, or else
		 * booting may not work properly.
		 */
		if (d > 2 && r == ENODEV)
			return 0;	/* assume no change */
		else
			return r;
	}
	
	if (r == 1)
	{
		/* drive _may_ have changed */
		r = rwabs (0, tmpbuf, 1, 0, d, 0L);	/* check the BIOS */
		if (r != ECHMEDIA)
		{
			/* nope, no change */
			TRACE (("rwabs returned %d", r));
			
			return (r < 0) ? r : 0;
		}
		r = 2;	/* drive was definitely changed */
	}
	
	if (r == 2)
	{
		TRACE (("definite media change"));
		
		/* get filesystem associated with drive */
		fs = drives[d];
		
		/* does the fs agree that it changed? */
		if (xfs_dskchng (fs, d, 0))
		{
			drives[d] = 0;
			changedrv (d);	/* yes -- do the change */
			return 1;
		}
	}
	
	return 0;
}

/*
 * routines for parsing path names
 */

/*
 * relpath2cookie converts a TOS file name into a file cookie representing
 * the directory the file resides in, and a character string representing
 * the name of the file in that directory. The character string is
 * copied into the "lastname" array. If lastname is NULL, then the cookie
 * returned actually represents the file, instead of just the directory
 * the file is in.
 *
 * note that lastname, if non-null, should be big enough to contain all the
 * characters in "path", since if the file system doesn't want the kernel
 * to do path name parsing we may end up just copying path to lastname
 * and returning the current or root directory, as appropriate
 *
 * "relto" is the directory relative to which the search should start.
 * if you just want the current directory, use path2cookie instead.
 *
 */

long
relpath2cookie (fcookie *relto, const char *path, char *lastname, fcookie *res, int depth)
{
	static char newpath[16] = "U:\\DEV\\";
	
	char temp2[PATH_MAX];
	char linkstuff[PATH_MAX];
	
	fcookie dir;
	int drv;
	XATTR xattr;
	long r;
	
	/* dolast: 0 == return a cookie for the directory the file is in
	 *         1 == return a cookie for the file itself, don't follow links
	 *	   2 == return a cookie for whatever the file points at
	 */
	int dolast = 0;
	int i = 0;
	
	if (!path)
		return ENOTDIR;
	
	if (!lastname)
	{
		dolast = 1;
		lastname = temp2;
	}
	else if (lastname == follow_links)
	{
		dolast = 2;
		lastname = temp2;
	}
	
	*lastname = 0;
	
	PATH2COOKIE_DB (("relpath2cookie(%s, dolast=%d, depth=%d [relto %lx, %i])",
		path, dolast, depth, relto->fs, relto->dev));
	
	if (depth > MAX_LINKS)
	{
		DEBUG (("Too many symbolic links"));
		return ELOOP;
	}
	
	/* special cases: CON:, AUX:, etc. should be converted to U:\DEV\CON,
	 * U:\DEV\AUX, etc.
	 */
# if 1
	if (path[0] && path[1] && path[2] && (path[3] == ':') && !path[4])
# else
	if (strlen (path) == 4 && path[3] == ':')
# endif
	{
		strncpy (newpath+7, path, 3);
		path = newpath;
	}
	
	/* first, check for a drive letter
	 * 
	 * BUG: a '\' at the start of a symbolic link is relative to the
	 * current drive of the process, not the drive the link is located on
	 */
	/* The check if the process runs chroot used to be inside the
	 * conditional (triggering ENOTDIR for drive specs.  IMHO its
	 * cleaner to interpret it as a regular filename instead.
	 * Rationale: The path "c:/auto" is actually the same as
	 * "/c:/auto".  If the process' root directory is not "/" but
	 * maybe "/home/ftp" then we should interpret the same filename
	 * now as "/home/ftp/c:/auto".
	 */
	if (path[1] == ':' && !curproc->root_dir)
	{
		char c = tolower (path[0]);
		
		if (c >= 'a' && c <= 'z')
			drv = c - 'a';
		else if (c >= '1' && c <= '6')
			drv = 26 + (c - '1');
		else
			goto nodrive;
		
# if 1
		/* if root_dir is set drive references are forbidden
		 */
		if (curproc->root_dir)
			return ENOTDIR;
# endif
		
		path += 2;
		
		/* remember that we saw a drive letter
		 */
		i = 1;
	}
	else
	{
nodrive:
		drv = curproc->curdrv;
	}
	
	/* see if the path is rooted from '\\'
	 */
	if (DIRSEP (*path))
	{
		while (DIRSEP (*path))
			path++;
		
		/* if root_dir is set this is our start point
		 */
		if (curproc->root_dir)
			dup_cookie (&dir, &curproc->root_fc);
		else
			dup_cookie (&dir, &curproc->root[drv]);
	}
	else
	{
		if (i)
		{
			/* an explicit drive letter was given
			 */
			dup_cookie (&dir, &curproc->curdir[drv]);
		}
		else
		{
			PATH2COOKIE_DB (("relpath2cookie: using relto (%lx, %li, %i) for dir", relto->fs, relto->index, relto->dev));
			dup_cookie (&dir, relto);
		}
	}
	
	if (!dir.fs && !curproc->root_dir)
	{
		changedrv (dir.dev);
		dup_cookie (&dir, &curproc->root[drv]);
	}
	
	if (!dir.fs)
	{
		DEBUG (("relpath2cookie: no file system: returning ENXIO"));
		return ENXIO;
	}
	
	/* here's where we come when we've gone across a mount point
	 */
restart_mount:
	
	/* see if there has been a disk change; if so, return ECHMEDIA.
	 * path2cookie will restart the search automatically; other functions
	 * that call relpath2cookie directly will have to fail gracefully.
	 * Note that this check has to be done _before_ testing if the
	 * remaining path is empty, as disk changes may otherwise get lost
	 * when accessing a root directory, depending on the active
	 * filesystem.
	 */
	if ((r = disk_changed (dir.dev)) != 0)
	{
		release_cookie (&dir);
		if (r > 0)
			r = ECHMEDIA;
		
		PATH2COOKIE_DB (("relpath2cookie: returning %d", r));
		return r;
	}
	
	if (!*path)
	{
		/* nothing more to do
		 */
		PATH2COOKIE_DB (("relpath2cookie: no more path, returning 0"));
		
		*res = dir;
		return 0;
	}
	
	
	if (dir.fs->fsflags & FS_KNOPARSE)
	{
		if (!dolast)
		{
			PATH2COOKIE_DB (("fs is a KNOPARSE, nothing to do"));
			
			strncpy (lastname, path, PATH_MAX-1);
			lastname[PATH_MAX - 1] = 0;
			r = 0;
			*res = dir;
		}
		else
		{
			PATH2COOKIE_DB (("fs is a KNOPARSE, calling lookup"));
			
			r = xfs_lookup (dir.fs, &dir, path, res);
			if (r == EMOUNT)
			{
				/* hmmm... a ".." at a mount point, maybe
				 */
				fcookie mounteddir;
				
				r = xfs_root (dir.fs, dir.dev, &mounteddir);
				if (r == 0 && drv == UNIDRV)
				{
					if (dir.fs == mounteddir.fs
						&& dir.index == mounteddir.index
						&& dir.dev == mounteddir.dev)
					{
						release_cookie (&dir);
						release_cookie (&mounteddir);
						dup_cookie (&dir, &curproc->root[UNIDRV]);
						TRACE (("path2cookie: restarting from mount point"));
						goto restart_mount;
					}
				}
				else
				{
					if (r == 0)
						release_cookie (&mounteddir);
					
					r = 0;
				}
			}
			
			release_cookie (&dir);
		}
		
		PATH2COOKIE_DB (("relpath2cookie: returning %ld", r));
		return r;
	}
	
	
	/* parse all but (possibly) the last component of the path name
	 * 
	 * rules here: at the top of the loop, &dir is the cookie of
	 * the directory we're in now, xattr is its attributes, and res is
	 * unset at the end of the loop, &dir is unset, and either r is
	 * nonzero (to indicate an error) or res is set to the final result
	 */
	r = xfs_getxattr (dir.fs, &dir, &xattr);
	if (r)
	{
		DEBUG (("couldn't get directory attributes"));
		release_cookie (&dir);
		return EINTERNAL;
	}
	
	while (*path)
	{
		/*  skip slashes
		 */
		while (DIRSEP (*path))
			path++;
		
		/* now we must have a directory, since there are more things
		 * in the path
		 */
		if ((xattr.mode & S_IFMT) != S_IFDIR)
		{
			PATH2COOKIE_DB (("relpath2cookie: not a directory, returning ENOTDIR"));
			release_cookie (&dir);
			r = ENOTDIR;
			break;
		}
		
		/* we must also have search permission for the directory
		 */
		if (denyaccess (&xattr, S_IXOTH))
		{
			DEBUG (("search permission in directory denied"));
			release_cookie (&dir);
			r = ENOTDIR;
			break;
		}
		
		/* if there's nothing left in the path, we can break here
		 */
		if (!*path)
		{
			PATH2COOKIE_DB (("relpath2cookie: no more path, breaking (1)"));
			*res = dir;
			break;
		}
		
		/* next, peel off the next name in the path
		 */
		{
			register int len;
			register char c, *s;
			
			len = 0;
			s = lastname;
			c = *path;
			while (c && !DIRSEP (c))
			{
				if (len++ < PATH_MAX)
					*s++ = c;
				c = *++path;
			}
			
			*s = 0;
		}
		
		/* if there are no more names in the path, and we don't want
		 * to actually look up the last name, then we're done
		 */
		if (dolast == 0 && !*path)
		{
			PATH2COOKIE_DB (("relpath2cookie: no more path, breaking (2)"));
			*res = dir;
			PATH2COOKIE_DB (("relpath2cookie: *res = [%lx, %i]", res->fs, res->dev));
			break;
		}
		
		if (curproc->root_dir)
		{
			if (samefile (&dir, &curproc->root_fc)
				&& lastname[0] == '.'
				&& lastname[1] == '.'
				&& lastname[2] == '\0')
			{
				PATH2COOKIE_DB (("relpath2cookie: can't leave root [%s] -> forward to '.'", curproc->root_dir));
				
				lastname[1] = '\0';
			}
		}
		
		PATH2COOKIE_DB (("relpath2cookie: looking up [%s]", lastname));
		
		r = xfs_lookup (dir.fs, &dir, lastname, res);
		if (r == EMOUNT)
		{
			fcookie mounteddir;
			
			r = xfs_root (dir.fs, dir.dev, &mounteddir);
			if (r == 0 && drv == UNIDRV)
			{
				if (samefile (&dir, &mounteddir))
				{
					release_cookie (&dir);
					release_cookie (&mounteddir);
					dup_cookie (&dir, &curproc->root[UNIDRV]);
					TRACE( ("path2cookie: restarting from mount point"));
					goto restart_mount;
				}
				else if (r == 0)
				{
					r = EINTERNAL;
					release_cookie (&mounteddir);
					release_cookie (&dir);
					break;
				}
			}
			else if (r == 0)
			{
				release_cookie (&mounteddir);
			}
			else
			{
				release_cookie (&dir);
				break;
			}
		}
		else if (r)
		{
			if (r == ENOENT && *path)
			{
				/* the "file" we didn't find was treated as a
				 * directory
				 */
				r = ENOTDIR;
			}
			release_cookie (&dir);
			break;
		}
		
		/* read the file attribute
		 */
		r = xfs_getxattr (res->fs, res, &xattr);
		if (r != 0)
		{
			DEBUG (("path2cookie: couldn't get file attributes"));
			release_cookie (&dir);
			release_cookie (res);
			break;
		}
		
		/* check for a symbolic link
		 * - if the file is a link, and we're following links, follow it
		 */
		if ((xattr.mode & S_IFMT) == S_IFLNK && (*path || dolast > 1))
		{
# if WITH_KERNFS
			/* The symbolic links on the kern filesystem have
			 * their own ideas about following links.
			 */
			if (res->fs == &kern_filesys)
			{
				release_cookie (&dir);
				
				depth++;
				r = kern_follow_link (res, depth);
				if (r)
				{
					release_cookie (res);
					break;
				}
			}
			else
# endif
			{
				r = xfs_readlink (res->fs, res, linkstuff, PATH_MAX);
				release_cookie (res);
				if (r)
				{
					DEBUG (("error reading symbolic link"));
					release_cookie (&dir);
					break;
				}
				r = relpath2cookie (&dir, linkstuff, follow_links, res, depth + 1);
				release_cookie (&dir);
				if (r)
				{
					DEBUG (("error following symbolic link"));
					break;
				}
			}
			dir = *res;
			(void) xfs_getxattr (res->fs, res, &xattr);
		}
		else
		{
			release_cookie (&dir);
			dir = *res;
		}
	}
	
	PATH2COOKIE_DB (("relpath2cookie: returning %ld", r));
	return r;
}

long
path2cookie (const char *path, char *lastname, fcookie *res)
{
	/* AHDI sometimes will keep insisting that a media change occured;
	 * we limit the number of retrys to avoid hanging the system
	 */
# define MAX_TRYS 4
	int trycnt = MAX_TRYS - 1;
	
	fcookie *dir = &curproc->curdir[curproc->curdrv];
	long r;
	
restart:
	r = relpath2cookie (dir, path, lastname, res, 0);
	if (r == ECHMEDIA && trycnt--)
	{
		DEBUG (("path2cookie: restarting due to media change"));
		goto restart;
	}
	
	return r;
}

/*
 * release_cookie: tell the file system owner that a cookie is no
 * longer in use by the kernel
 *
 * release_cookie doesn't release anymore unless there is no entry in
 * the cookie cache.  Otherwise, we just let the cookie get released
 * through clobber_cookie when the cache fills or is killed through the
 * routine above - EKL
 */
void
release_cookie (fcookie *fc)
{
	if (fc)
	{
		FILESYS *fs;
		
		fs = fc->fs;
		if (fs && fs->release)
			xfs_release (fs, fc);
	}
}

/*
 * Make a new cookie (newc) which is a duplicate of the old cookie
 * (oldc). This may be something the file system is interested in,
 * so we give it a chance to do the duplication; if it doesn't
 * want to, we just copy.
 */

void
dup_cookie (fcookie *newc, fcookie *oldc)
{
	FILESYS *fs;
	
	fs = oldc->fs;
	if (fs && fs->dupcookie)
		(void) xfs_dupcookie (fs, newc, oldc);
	else
		*newc = *oldc;
}

/*
 * new_fileptr, dispose_fileptr: allocate (deallocate) a file pointer
 */

FILEPTR *
new_fileptr (void)
{
	FILEPTR *f;
	
	f = kmalloc (sizeof (*f));
	if (f) bzero (f, sizeof (*f));
	
	return f;
}

void
dispose_fileptr (FILEPTR *f)
{
	if (f->links != 0)
		FATAL ("dispose_fileptr: f->links == %d", f->links);
	
	kfree (f);
}

/*
 * denyshare(list, f): "list" points at the first FILEPTR in a
 * chained list of open FILEPTRS referring to the same file;
 * f is a newly opened FILEPTR. Every FILEPTR in the given list is
 * checked to see if its "open" mode (in list->flags) is compatible with
 * the open mode in f->flags. If not (for example, if f was opened with
 * a "read" mode and some other file has the O_DENYREAD share mode),
 * then 1 is returned. If all the open FILEPTRs in the list are
 * compatible with f, then 0 is returned.
 * This is not as complicated as it sounds. In practice, just keep a
 * list of open FILEPTRs attached to each file, and put something like
 * 	if (denyshare(thisfile->openfileptrlist, newfileptr))
 *		return EACCES;
 * in the device open routine.
 */

int _cdecl 
denyshare (FILEPTR *list, FILEPTR *f)
{
	int newrm, newsm;	/* new read and sharing mode */
	int oldrm, oldsm;	/* read and sharing mode of already opened file */
	extern MEMREGION *tofreed;
	MEMREGION *m = tofreed;
	int i;
	
	newrm = f->flags & O_RWMODE;
	newsm = f->flags & O_SHMODE;
	
	/* O_EXEC gets treated the same as O_RDONLY for our purposes
	 */
	if (newrm == O_EXEC)
		newrm = O_RDONLY;
	
	/* New meaning for O_COMPAT: deny write access to all _other_
	 * processes.
	 */
	for ( ; list; list = list->next)
	{
		oldrm = list->flags & O_RWMODE;
		if (oldrm == O_EXEC) oldrm = O_RDONLY;
		oldsm = list->flags & O_SHMODE;
		
		if (oldsm == O_DENYW || oldsm == O_DENYRW)
		{
		 	if (newrm != O_RDONLY)
		 	{
				/* conflict because of unattached shared text region? */
				if (!m && NULL != (m = find_text_seg (list)))
				{
					if (m->links == 0xffff)
						continue;
					m = 0;
				}
				
				DEBUG (("write access denied"));
				return 1;
			}
		}
		
		if (oldsm == O_DENYR || oldsm == O_DENYRW)
		{
			if (newrm != O_WRONLY)
			{
				DEBUG (("read access denied"));
				return 1;
			}
		}
		
		if (newsm == O_DENYW || newsm == O_DENYRW)
		{
			if (oldrm != O_RDONLY)
			{
				DEBUG (("couldn't deny writes"));
				return 1;
			}
		}
		
		if (newsm == O_DENYR || newsm == O_DENYRW)
		{
			if (oldrm != O_WRONLY)
			{
				DEBUG (("couldn't deny reads"));
				return 1;
			}
		}
		
		/* If either sm == O_COMPAT, then we check to make sure
		 * that the file pointers are owned by the same process
		 * (O_COMPAT means "deny writes to any other processes").
		 * This isn't quite the same as the Atari spec, which says
		 * O_COMPAT means "deny access to other processes." We should
		 * fix the spec.
		 */
		if ((newsm == O_COMPAT && newrm != O_RDONLY && oldrm != O_RDONLY)
			|| (oldsm == O_COMPAT && newrm != O_RDONLY))
		{
			for (i = MIN_HANDLE; i < MAX_OPEN; i++)
				if (curproc->handle[i] == list)
					goto found;
			
			/* old file pointer is not open by this process */
			DEBUG (("O_COMPAT file was opened for writing by another process"));
			return 1;
			
		found:
			;	/* everything is OK */
		}
	}
	
	/* cannot close shared text regions file here... have open do it. */
	if (m)
		tofreed = m;
	
	return 0;
}

/*
 * denyaccess(XATTR *xattr, unsigned perm): checks to see if the access
 * specified by perm (which must be some combination of S_IROTH, S_IWOTH,
 * and S_IXOTH) should be granted to the current process
 * on a file with the given extended attributes. Returns 0 if access
 * by the current process is OK, 1 if not.
 */

int
ngroupmatch (int group)
{
	int i;
	
	for (i = 0; i < curproc->ngroups; i++)
		if (curproc->ngroup[i] == group)
			return 1;
	
	return 0;
}

int
denyaccess (XATTR *xattr, unsigned int perm)
{
	unsigned mode;
	
	/* the super-user can do anything! */
	if (curproc->euid == 0)
		return 0;
	
	mode = xattr->mode;
	if (curproc->euid == xattr->uid)
		perm = perm << 6;
	else if (curproc->egid == xattr->gid)
		perm = perm << 3;
	else if (ngroupmatch (xattr->gid))
		perm = perm << 3;
	
	if ((mode & perm) != perm)
		/* access denied */
		return 1;
	
	return 0;
}

/*
 * Checks a lock against a list of locks to see if there is a conflict.
 * This is a utility to be used by file systems, somewhat like denyshare
 * above. Returns 0 if there is no conflict, or a pointer to the
 * conflicting LOCK structure if there is.
 *
 * Conflicts occur for overlapping locks if the process id's are
 * different and if at least one of the locks is a write lock.
 *
 * NOTE: we assume before being called that the locks have been converted
 * so that l_start is absolute. not relative to the current position or
 * end of file.
 */

LOCK * _cdecl 
denylock (LOCK *list, LOCK *lck)
{
	LOCK *t;
	unsigned long tstart, tend;
	unsigned long lstart, lend;
	int pid = curproc->pid;
	int ltype;

	ltype = lck->l.l_type;
	lstart = lck->l.l_start;

	if (lck->l.l_len == 0)
		lend = 0xffffffffL;
	else
		lend = lstart + lck->l.l_len - 1;

	for (t = list; t; t = t->next) {
		tstart = t->l.l_start;
		if (t->l.l_len == 0)
			tend = 0xffffffffL;
		else
			tend = tstart + t->l.l_len - 1;

	/* look for overlapping locks */
		if (tstart <= lstart && tend >= lstart && t->l.l_pid != pid &&
		    (ltype == F_WRLCK || t->l.l_type == F_WRLCK))
			break;
		if (lstart <= tstart && lend >= tstart && t->l.l_pid != pid &&
		    (ltype == F_WRLCK || t->l.l_type == F_WRLCK))
			break;
	}
	return t;
}

/*
 * check to see that a file is a directory, and that write permission
 * is granted; return an error code, or 0 if everything is ok.
 */
long
dir_access (fcookie *dir, unsigned int perm, unsigned int *mode)
{
	XATTR xattr;
	long r;
	
	r = xfs_getxattr (dir->fs, dir, &xattr);
	if (r)
	{
		DEBUG (("dir_access: file system returned %ld", r));
		return r;
	}
	
	if ((xattr.mode & S_IFMT) != S_IFDIR )
	{
		DEBUG (("file is not a directory"));
		return ENOTDIR;
	}
	
	if (denyaccess (&xattr, perm))
	{
		DEBUG(("no permission for directory"));
		return EACCES;
	}
	
	*mode = xattr.mode;
	
	return 0;
}

/*
 * returns 1 if the given name contains a wildcard character 
 */

int
has_wild(const char *name)
{
	char c;

	while ((c = *name++) != 0) {
		if (c == '*' || c == '?') return 1;
	}
	return 0;
}

/*
 * void copy8_3(dest, src): convert a file name (src) into DOS 8.3 format
 * (in dest). Note the following things:
 * if a field has less than the required number of characters, it is
 * padded with blanks
 * a '*' means to pad the rest of the field with '?' characters
 * special things to watch for:
 *	"." and ".." are more or less left alone
 *	"*.*" is recognized as a special pattern, for which dest is set
 *	to just "*"
 * Long names are truncated. Any extensions after the first one are
 * ignored, i.e. foo.bar.c -> foo.bar, foo.c.bar->foo.c.
 */

void
copy8_3(char *dest, const char *src)
{
	char fill = ' ', c;
	int i;

	if (src[0] == '.') {
		if (src[1] == 0) {
			strcpy(dest, ".       .   ");
			return;
		}
		if (src[1] == '.' && src[2] == 0) {
			strcpy(dest, "..      .   ");
			return;
		}
	}
	if (src[0] == '*' && src[1] == '.' && src[2] == '*' && src[3] == 0) {
		dest[0] = '*';
		dest[1] = 0;
		return;
	}

	for (i = 0; i < 8; i++) {
		c = *src++;
		if (!c || c == '.') break;
		if (c == '*') {
			fill = c = '?';
		}
		*dest++ = toupper(c);
	}
	while (i++ < 8) {
		*dest++ = fill;
	}
	*dest++ = '.';
	i = 0;
	fill = ' ';
	while (c && c != '.')
		c = *src++;

	if (c) {
		for( ;i < 3; i++) {
			c = *src++;
			if (!c || c == '.') break;
			if (c == '*')
				c = fill = '?';
			*dest++ = toupper(c);
		}
	}
	while (i++ < 3)
		*dest++ = fill;
	*dest = 0;
}

/*
 * int pat_match(name, patrn): returns 1 if "name" matches the template in
 * "patrn", 0 if not. "patrn" is assumed to have been expanded in 8.3
 * format by copy8_3; "name" need not be. Any '?' characters in patrn
 * will match any character in name. Note that if "patrn" has a '*' as
 * the first character, it will always match; this will happen only if
 * the original pattern (before copy8_3 was applied) was "*.*".
 *
 * BUGS: acts a lot like the silly TOS pattern matcher.
 */

int
pat_match (const char *name, const char *template)
{
	char expname [TOS_NAMELEN+1];
	register char *s;
	register char c;
	
	if (*template == '*')
		return 1;
	
	copy8_3 (expname, name);
	
	s = expname;
	while ((c = *template++) != 0)
	{
		if (c != *s && c != '?')
			return 0;
		s++;
	}
	
	return 1;
}

/*
 * int samefile(fcookie *a, fcookie *b): returns 1 if the two cookies
 * refer to the same file or directory, 0 otherwise
 */

int
samefile (fcookie *a, fcookie *b)
{
	if (a->fs == b->fs && a->dev == b->dev && a->index == b->index)
		return 1;
	
	return 0;
}
