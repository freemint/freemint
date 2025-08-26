/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corp.
 * All rights reserved.
 *
 */

/*
 * various file system interface things
 */

# include "filesys.h"

# include "libkern/libkern.h"

# include "mint/dcntl.h"
# include "mint/filedesc.h"
# include "mint/signal.h"

# include "arch/mprot.h"
# include "arch/tosbind.h"

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
# include "info.h"
# include "ipc_socketdev.h"
# include "k_fds.h"
# include "k_prot.h"
# include "kmemory.h"
# include "memory.h"
# include "module.h"
# include "proc.h"
# include "signal.h"
# include "time.h"

# ifdef WITH_HOSTFS
# include "xfs/hostfs/hostfs_xfs.h"
# include "xfs/hostfs/hostfs.h"
# endif
# ifdef WITH_ARANYMFS
# include "xfs/aranym/aranym_fsdev.h"
# endif


#if 1
#define PATH2COOKIE_DB(x) TRACE(x)
#else
#define PATH2COOKIE_DB(x) DEBUG(x)
#endif


static void
xfs_dismiss (FILESYS *fs);

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

	TRACE (("init_drive (%c)", DriveToLetter(i)));

	assert (i >= 0 && i < NUM_DRIVES);

	drives[i] = NULL;		/* no file system */
	if (dlockproc[i])
		return;

	for (fs = active_fs; fs; fs = fs->next)
	{
		DEBUG(("init_drive: fs %p, drv %d", fs, i));

		r = xfs_root (fs, i, &root_dir);
		if (r == 0)
		{
			drives[i] = root_dir.fs;
			release_cookie (&root_dir);
			DEBUG(("init_drive: drv %d is fs %p", i, fs));
			break;
		} else
		{
			DEBUG(("init_drive(%d): %ld", i, r));
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
	{
		short drv = TRAP_Dgetdrv();
		dosdrvs = TRAP_Dsetdrv(drv) | sys_b_drvmap();
	}

# ifdef OLDTOSFS
	xfs_add (&tos_filesys);
# endif
	xfs_add (&fatfs_filesys);
# ifdef WITH_HOSTFS
	/* after fatfs_filesys to pick hostfs when
	 * enabled on already FATFS occupied drive */
	xfs_add (&hostfs_filesys);
# endif
	xfs_add (&bios_filesys);
	xfs_add (&pipe_filesys);
	xfs_add (&proc_filesys);
# ifndef NO_RAMFS
	xfs_add (&ramfs_filesys);
# endif
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
# ifndef NO_RAMFS
	ramfs_init ();
# endif

	/* initialize the shared memory file system */
	shmfs_init ();

	/* initialize the unified file system */
	unifs_init ();

	/* initialize the main file system */
	fatfs_init ();

# ifdef WITH_HOSTFS
	/* initialize the hostfs file system */
	{
		FILESYS *fs = hostfs_init ();
		if ( fs )
			hostfs_mount_drives( fs );
		else
			xfs_dismiss( &hostfs_filesys );
	}
# endif

# ifdef WITH_ARANYMFS
	/* initialize the arafs file system */
	{
		FILESYS *fs = aranymfs_init ();
		if ( fs )
			arafs_mount_drives( fs );
		else
			xfs_dismiss( &arafs_filesys );
	}
# endif

	UNUSED (xfs_dismiss); /* Maybe */
}

# ifdef DEBUG_INFO
char *
xfs_name (fcookie *fc)
{
	static char buf [SPRINTF_MAX];
	long r;

	buf [0] = '\0';

	TRACE(("xfs_name: call xfs_fscntl.. buf = %p, fc %p, fs %p", &buf, fc, fc->fs));
	if (fc->fs)
		r = xfs_fscntl (fc->fs, fc, buf, MX_KER_XFSNAME, (long)&buf);
	else {
		r = 0;
		ksprintf(buf, sizeof(buf), "unknown fs (%p)", fc->fs);
	}
	TRACE(("xfs_name: xfs_fctnl returned %lx", r));
	if (r)
		ksprintf (buf, sizeof (buf), "unknown (%p -> %li)", fc->fs, r);

	return buf;
}

/* uk: go through the list of file systems and call their sync() function
 *     if they wish to.
 */

long
_s_ync (void)
{
	FILESYS *fs;

	ALERT (MSG_fsys_syncing);

	/* syncing filesystems */
	for (fs = active_fs; fs; fs = fs->next)
	{
		if (fs->fsflags & FS_DO_SYNC)
			(*fs->sync)();
	}

	/* always syncing buffercache */
	bio_sync_all ();

	ALERT (MSG_fsys_syncing_done);

	return 0;
}
# endif

long _cdecl
sys_s_ync (void)
{
	FILESYS *fs;

	TRACE ((MSG_fsys_syncing));

	/* syncing filesystems */
	for (fs = active_fs; fs; fs = fs->next)
	{
		if (fs->fsflags & FS_DO_SYNC)
			xfs_sync (fs);
	}

	/* always syncing buffercache */
	bio_sync_all ();

	TRACE ((MSG_fsys_syncing_done));
	return 0;
}

long _cdecl
sys_fsync (short fh)
{
	/* dummy function at the moment */
	return sys_s_ync ();
}

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

/* dismiss a file system which failed to initialize */
static void
xfs_dismiss (FILESYS *fs)
{
	FILESYS **p;

	for (p = &active_fs; *p; p = &(*p)->next)
	{
		if (*p == fs)
		{
			*p = (*p)->next;
			fs->next = NULL;

			return;
		}
	}
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

		for (i = MIN_HANDLE; i < p->p_fd->nfiles; i++)
		{
			FILEPTR *f;

			f = p->p_fd->ofiles [i];
			p->p_fd->ofiles [i] = NULL;
			if (f) do_close (p, f);
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
_changedrv (ushort d, const char *function)
{
	PROC *p;
	int i;
	FILEPTR *f;
	FILESYS *fs;
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
		struct filedesc *fd = p->p_fd;
		struct cwd *cwd = p->p_cwd;

		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;

		if (!fd || !cwd)
			FATAL (ERR_fsys_inv_fdcwd, function);

		/* invalidate all open files on this device */
		for (i = MIN_HANDLE; i < fd->nfiles; i++)
		{
			f = fd->ofiles[i];

			if (!f || (f == (FILEPTR *) 1))
				continue;

# ifdef OLDSOCKDEVEMU
			if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
			if (f->dev == &sockdev)
# endif
				continue;

			/* it's a regular file */

			if (f->fc.dev != d)
				continue;

			/* and it's on the changed dev */

			if (!warned)
			{
				ALERT (MSG_fsys_files_were_open, d, p->name);
				warned++;
			}

/* we set f->dev to NULL to indicate to do_pclose that this is an
 * emergency close, and that it shouldn't try to make any
 * calls to the device driver since the file has gone away
 */
			f->dev = NULL;
			do_close (p, f);
/* we could just zero the handle, but this could lead to confusion if
 * a process doesn't realize that there's been a media change, Fopens
 * a new file, and gets the same handle back. So, we force the
 * handle to point to /dev/null.
 */

			fd->ofiles[i] = (FILEPTR *) 1;

			r = FP_ALLOC (p, &f);
			if (!r)
			{
				r = do_open (&f, "U:\\DEV\\NULL", O_RDWR, 0, NULL);
				if (r)
				{
					fd->ofiles[i] = NULL;
					FP_FREE (f);
				}
				else
					fd->ofiles[i] = f;
			}
			else
				fd->ofiles[i] = NULL;
		}

		/* terminate any active directory searches on the drive */
		for (i = 0; i < NUM_SEARCH; i++)
		{
			dirh = &fd->srchdir[i];
			if (fd->srchdta[i] && dirh->fc.fs && dirh->fc.dev == d)
			{
				TRACE (("closing search for process %d", p->pid));
				release_cookie (&dirh->fc);
				dirh->fc.fs = NULL;
				fd->srchdta[i] = 0;
			}
		}

		for (dirh = fd->searches; dirh; dirh = dirh->next)
		{
			/* If this search is on the changed drive, release
			 * the cookie, but do *not* free it, since the
			 * user could later call closedir on it.
			 */
			if (dirh->fc.fs && dirh->fc.dev == d)
			{
				release_cookie (&dirh->fc);
				dirh->fc.fs = NULL;
			}
		}

		if (d >= NUM_DRIVES)
			continue;

		/* change any active directories on the device to the (new) root */
		fs = drives[d];
		if (fs)
		{
			r = xfs_root (fs, d, &dir);
			if (r != E_OK) dir.fs = NULL;
		}
		else
		{
			dir.fs = NULL;
			dir.dev = d;
		}

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (cwd->root[i].dev == d)
			{
				release_cookie (&cwd->root[i]);
				dup_cookie (&cwd->root[i], &dir);
			}
			if (cwd->curdir[i].dev == d)
			{
				release_cookie (&cwd->curdir[i]);
				dup_cookie (&cwd->curdir[i], &dir);
			}
		}

		/* hmm, what we can do if the drive changed
		 * that hold our root dir?
		 */
		if (cwd->root_dir && cwd->rootdir.dev == d)
		{
			release_cookie (&cwd->rootdir);
			cwd->rootdir.fs = NULL;
			kfree (cwd->root_dir);
			cwd->root_dir = NULL;

			post_sig (p, SIGKILL);
		}

		release_cookie (&dir);
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
		TRACE (("drive %c not yet initialized", DriveToLetter(d)));
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
	r = sys_b_mediach (d);
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
#ifdef OLDTOSFS
		/* The kernel for the Hatari emulator (minthat.prg) is compiled
		 * with OLDTOSFS to enable access to hard drives emulated only on
		 * GEMDOS level. However, in that case there is no BIOS device
		 * for drive C:, either, and we must ignore ENODEV.
		 */
		if (d >= 2 && r == ENODEV)
#else
		if (d > 2 && r == ENODEV)
#endif
			return 0;	/* assume no change */
		else
			return r;
	}

	if (r == 1)
	{
		/* drive _may_ have changed */
		r = sys_b_rwabs (0, tmpbuf, 1, 0, d, 0L);	/* check the BIOS */
		if (r != ECHMEDIA)
		{
			/* nope, no change */
			TRACE (("rwabs returned %ld", r));

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
			changedrv (d); /* yes -- do the change */
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

long _cdecl
relpath2cookie(struct proc *p, fcookie *relto, const char *path, char *lastname,
	       fcookie *res, int depth)
{
	char newpath[16];

	struct cwd *cwd = p->p_cwd;

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

	if (*path == '\0')
		return ENOENT;

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

	*lastname = '\0';

	PATH2COOKIE_DB (("relpath2cookie(%s, dolast=%d, depth=%d [relto %p, %i])",
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
		strcpy(newpath, "U:\\DEV\\");
		newpath[7] = path[0];
		newpath[8] = path[1];
		newpath[9] = path[2];

		if ((path[0] == 'N' || path[0] == 'n') &&
		    (path[1] == 'U' || path[1] == 'u') &&
		    (path[2] == 'L' || path[2] == 'l'))
		{
			/* the device file is u:\dev\null */
			newpath[10] = 'l';
			newpath[11] = '\0';
		} else
		if ((path[0] == 'M' || path[0] == 'm') &&
		    (path[1] == 'I' || path[1] == 'i') &&
		    (path[2] == 'D' || path[2] == 'd'))
		{
			/* the device file is u:\dev\midi */
			newpath[10] = 'i';
			newpath[11] = '\0';
		} else {
			/* add the NULL terminator */
			newpath[10] = '\0';
		}

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
	if (path[2] != ':' && path[1] == ':' && !cwd->root_dir)
	{
		drv = DriveFromLetter(path[0]);
		if (drv < 0)
			goto nodrive;

# if 1
		/* if root_dir is set drive references are forbidden
		 */
		if (cwd->root_dir)
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
		drv = cwd->curdrv;
	}

	/* see if the path is rooted from '\\'
	 */
	if (DIRSEP (*path))
	{
		while (DIRSEP (*path))
			path++;

		/* if root_dir is set this is our start point
		 */
		if (cwd->root_dir)
			dup_cookie (&dir, &cwd->rootdir);
		else
			dup_cookie (&dir, &cwd->root[drv]);
	}
	else
	{
		if (i)
		{
			/* an explicit drive letter was given
			 */
			dup_cookie (&dir, &cwd->curdir[drv]);
		}
		else
		{
			PATH2COOKIE_DB (("relpath2cookie: using relto (%p, %li, %i) for dir", relto->fs, relto->index, relto->dev));
			dup_cookie (&dir, relto);
		}
	}

	if (!dir.fs && !cwd->root_dir)
	{
		changedrv (dir.dev);
		dup_cookie (&dir, &cwd->root[drv]);
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

		PATH2COOKIE_DB (("relpath2cookie(1): returning %ld", r));
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
						dup_cookie (&dir, &cwd->root[UNIDRV]);
						DEBUG(("path2cookie: restarting from mount point"));
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

		PATH2COOKIE_DB (("relpath2cookie(2): returning %ld", r));
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
		/* we must have a directory, since there are more things
		 * in the path
		 */
		if (!S_ISDIR(xattr.mode))
		{
			PATH2COOKIE_DB (("relpath2cookie: not a directory, returning ENOTDIR"));
			release_cookie (&dir);
			r = ENOTDIR;
			break;
		}

		/* we must also have search permission for the directory
		 */
		if (denyaccess (p->p_cred->ucr, &xattr, S_IXOTH))
		{
			DEBUG (("search permission in directory denied"));
			release_cookie (&dir);
			/* r = ENOTDIR; */
			r = EACCES;
			break;
		}

		/* skip slashes
		 */
		while (DIRSEP (*path))
			path++;

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

			*s = '\0';
		}

		/* if there are no more names in the path, and we don't want
		 * to actually look up the last name, then we're done
		 */
		if (dolast == 0)
		{
			register const char *s = path;

			while (DIRSEP (*s))
				s++;
				
			if (!*s) {
				PATH2COOKIE_DB (("relpath2cookie: no more path, breaking"));
				*res = dir;
				PATH2COOKIE_DB (("relpath2cookie: *res = [%p, %i]", res->fs, res->dev));
				break;
			}
		}

		if (cwd->root_dir)
		{
			if (samefile (&dir, &cwd->rootdir)
				&& lastname[0] == '.'
				&& lastname[1] == '.'
				&& lastname[2] == '\0')
			{
				PATH2COOKIE_DB (("relpath2cookie: can't leave root [%s] -> forward to '.'", cwd->root_dir));

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
					dup_cookie (&dir, &cwd->root[UNIDRV]);
					TRACE(("path2cookie: restarting from mount point"));
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
				*res = mounteddir;
			}
			else
			{
				release_cookie (&dir);
				break;
			}
		}
		else if (r)
		{
			release_cookie (&dir);
			/*
			 * TOS programs might expect ENOTDIR
			 */
			if (r == ENOENT && dolast == 0 && p->domain == DOM_TOS)
				r = ENOTDIR;
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
		if (S_ISLNK(xattr.mode) && (*path || dolast > 1))
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
				r = relpath2cookie (p, &dir, linkstuff, follow_links, res, depth + 1);
				release_cookie (&dir);
				if (r)
				{
					DEBUG (("error following symbolic link"));
					break;
				}
			}
			dir = *res;
			xfs_getxattr (res->fs, res, &xattr);
		}
		else
		{
			TRACE(("relpath2cookie: lookup ok, mode 0x%x", xattr.mode));

			release_cookie (&dir);
			dir = *res;
		}
	}

	PATH2COOKIE_DB (("relpath2cookie(3): returning %ld", r));
	return r;
}

long _cdecl
path2cookie(struct proc *p, const char *path, char *lastname, fcookie *res)
{
	struct cwd *cwd = p->p_cwd;

	/* AHDI sometimes will keep insisting that a media change occured;
	 * we limit the number of retrys to avoid hanging the system
	 */
# define MAX_TRYS 4
	int trycnt = MAX_TRYS - 1;

	fcookie *dir = &cwd->curdir[cwd->curdrv];
	long r;

restart:
	r = relpath2cookie(p, dir, path, lastname, res, 0);
	if (r == ECHMEDIA && trycnt--)
	{
		DEBUG(("path2cookie: restarting due to media change"));
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
void _cdecl
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
		xfs_dupcookie (fs, newc, oldc);
	else
		*newc = *oldc;
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
		if (oldrm == O_EXEC)
			oldrm = O_RDONLY;
		oldsm = list->flags & O_SHMODE;

		if (oldsm == O_DENYW || oldsm == O_DENYRW)
		{
		 	if (newrm != O_RDONLY)
		 	{
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
			struct filedesc *fd = get_curproc()->p_fd;
			int i;

			for (i = MIN_HANDLE; i < fd->nfiles; i++)
				if (fd->ofiles[i] == list)
					goto found;

			/* old file pointer is not open by this process */
			DEBUG (("O_COMPAT file was opened for writing by another process"));
			return 1;

		found:
			;	/* everything is OK */
		}
	}

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
denyaccess(struct ucred *cred, XATTR *xattr, ushort perm)
{
	ushort mode;

	/* the super-user can do anything! */
	if (cred->euid == 0)
		return 0;

	mode = xattr->mode;
	if (cred->euid == xattr->uid)
		perm = perm << 6;
	else if (cred->egid == xattr->gid)
		perm = perm << 3;
	else if (groupmember(cred, xattr->gid))
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
LOCK *
denylock(ushort pid, LOCK *list, LOCK *lck)
{
	LOCK *t;
	ulong tstart, tend;
	ulong lstart, lend;
	int ltype;

	ltype = lck->l.l_type;
	lstart = lck->l.l_start;

	if (lck->l.l_len == 0)
		lend = 0xffffffffL;
	else
		lend = lstart + lck->l.l_len - 1;

	for (t = list; t; t = t->next)
	{
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
dir_access(struct ucred *cred, fcookie *dir, ushort perm, ushort *mode)
{
	XATTR xattr;
	long r;

	r = xfs_getxattr(dir->fs, dir, &xattr);
	if (r)
	{
		DEBUG(("dir_access: file system returned %ld", r));
		return r;
	}

	if (!S_ISDIR(xattr.mode))
	{
		DEBUG(("file is not a directory"));
		return ENOTDIR;
	}

	if (denyaccess(cred, &xattr, perm))
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

	while ((c = *name++) != 0)
		if (c == '*' || c == '?')
			return 1;

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

	if (src[0] == '.')
	{
		if (src[1] == 0)
		{
			strcpy(dest, ".       .   ");
			return;
		}

		if (src[1] == '.' && src[2] == 0)
		{
			strcpy(dest, "..      .   ");
			return;
		}
	}

	if (src[0] == '*' && src[1] == '.' && src[2] == '*' && src[3] == 0)
	{
		dest[0] = '*';
		dest[1] = 0;
		return;
	}

	for (i = 0; i < 8; i++)
	{
		c = *src++;

		if (!c || c == '.')
			break;
		if (c == '*')
			fill = c = '?';

		*dest++ = toupper((int)c & 0xff);
	}

	while (i++ < 8)
		*dest++ = fill;

	*dest++ = '.';
	i = 0;
	fill = ' ';
	while (c && c != '.')
		c = *src++;

	if (c)
	{
		for( ;i < 3; i++)
		{
			c = *src++;

			if (!c || c == '.')
				break;

			if (c == '*')
				c = fill = '?';

			*dest++ = toupper((int)c & 0xff);
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
pat_match(const char *name, const char *template)
{
	char expname[TOS_NAMELEN+1];
	register char *s;
	register char c;

	if (*template == '*')
		return 1;

	copy8_3(expname, name);

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
samefile(fcookie *a, fcookie *b)
{
	if (a->fs == b->fs && a->dev == b->dev && a->index == b->index)
		return 1;

	return 0;
}
