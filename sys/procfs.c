/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 * 
 * PROC pseudo-filesystem routines
 * 
 * basically just to allow 'ls -l X:' to give a list of active processes
 * some things to note:
 * process names are given as name.XXX, where 'XXX' is the pid of the
 *   process
 * process attributes depend on the run queue as follows:
 *   RUNNING:	0x00		(normal)
 *   READY:	0x01		(read-only)
 *   WAIT:	0x20		(archive bit)
 *   IOBOUND:	0x21		(archive bit+read-only)
 *   ZOMBIE:	0x22		(archive+hidden)
 *   TSR:	0x02		(hidden)
 *   STOP:	0x24		(archive bit+system)
 * the general principle is: inactive processes have the archive bit (0x20)
 * set, terminated processes have the hidden bit (0x02) set, stopped processes
 * have the system bit (0x04) set, and the read-only bit is used to
 * otherwise distinguish states (which is unfortunate, since it would be
 * nice if this bit corresponded with file permissions).
 * 
 */

# include "procfs.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/pathconf.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/signal.h"
# include "mint/stat.h"

# include "arch/cpu.h"		/* cpush */
# include "arch/mprot.h"

# include "dev-null.h"
# include "filesys.h"
# include "k_prot.h"
# include "kerinfo.h"
# include "memory.h"
# include "nullfs.h"
# include "proc.h"
# include "signal.h"
# include "time.h"
# include "util.h"


static PROC *	name2proc		(const char *name);

static long	_cdecl proc_root	(int drv, fcookie *fc);
static long	_cdecl proc_lookup	(fcookie *dir, const char *name, fcookie *fc);
static long	_cdecl proc_getxattr	(fcookie *fc, XATTR *xattr);
static long _cdecl proc_chown (fcookie *fc, int uid, int gid);
static long	_cdecl proc_stat64	(fcookie *fc, STAT *ptr);
static long	_cdecl proc_remove	(fcookie *dir, const char *name);
static long	_cdecl proc_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl proc_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
static long	_cdecl proc_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl proc_pathconf	(fcookie *dir, int which);
static long	_cdecl proc_dfree	(fcookie *dir, long *buf);
static DEVDRV *	_cdecl proc_getdev	(fcookie *fc, long *devsp);
static long	_cdecl proc_fscntl	(fcookie *dir, const char *name, int cmd, long arg);

static long	_cdecl proc_open	(FILEPTR *f);
static long	_cdecl proc_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl proc_read	(FILEPTR *f, char *buf, long bytes);
static long	_cdecl proc_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl proc_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl proc_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl proc_close	(FILEPTR *f, int pid);
static long	_cdecl proc_readlabel	(fcookie *dir, char *name, int namelen);


struct timeval procfs_stmp;

void
procfs_init (void)
{
	procfs_stmp = xtime;
}


FILESYS proc_filesys =
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
	FS_LONGPATH		|
	FS_NO_C_CACHE		|
	FS_REENTRANT_L1		|
	FS_REENTRANT_L2		|
	FS_EXT_2		|
	FS_EXT_3		,
	
	proc_root,
	proc_lookup, null_creat, proc_getdev, proc_getxattr,
	null_chattr, proc_chown, null_chmode,
	null_mkdir, null_rmdir, proc_remove, proc_getname, proc_rename,
	null_opendir, proc_readdir, null_rewinddir, null_closedir,
	proc_pathconf, proc_dfree, null_writelabel, proc_readlabel,
	null_symlink, null_readlink, null_hardlink, proc_fscntl, null_dskchng,
	NULL, NULL,
	NULL,
	
	/* FS_EXT_1 */
	NULL, NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	proc_stat64,
	
	0, 0, 0, 0, 0,
	NULL, NULL
};

DEVDRV proc_device =
{
	proc_open,
	proc_write, proc_read, proc_lseek,
	proc_ioctl, proc_datime,
	proc_close,
	null_select, null_unselect,
	NULL, NULL
};


static PROC *
name2proc (const char *name)
{
	const char *pstr;
	char c;
	int i;

	pstr = name;
	while ((c = *name++) != 0)
	{
		if (c == '.')
			pstr = name;
	}
	
	if (!isdigit (*pstr) && *pstr != '-')
		return 0;
	
	i = atol (pstr);
	if (i == -1)
		return get_curproc();
	else if (i == -2)
		i = get_curproc()->ppid;
	
	return pid2proc (i);
}

static struct proc *
getproc (long index)
{
	register struct proc *check = (struct proc *) index;
	register struct proc *p;
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p == check)
			return p;
	}
	
	return NULL;
}

long _cdecl 
proc_root (int drv, fcookie *fc)
{
	if (drv == PROCDRV)
	{
		fc->fs = &proc_filesys;
		fc->dev = drv;
		fc->index = 0L;
		return E_OK;
	}
	
	fc->fs = 0;
	return EINTERNAL;
}

static long _cdecl 
proc_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	PROC *p;
	
	if (dir->index != 0)
	{
		DEBUG (("proc_lookup: bad directory"));
		return ENOTDIR;
	}
	
	/* special case: an empty name in a directory means that directory
	 * so does "."
	 */
	if (!*name || (name[0] == '.' && name[1] == 0))
	{
		*fc = *dir;
		return E_OK;
	}
	
	/* another special case: ".." could be a mount point */
	if (!strcmp (name, ".."))
	{
		*fc = *dir;
		return EMOUNT;
	}
	
	p = name2proc (name);
	if (!p)
	{
		DEBUG (("proc_lookup: name not found"));
		return ENOENT;
	}
	
	fc->index = (long) p;
	fc->fs = &proc_filesys;
	fc->dev = PROC_RDEV_BASE | p->pid;
	
	return E_OK;
}

/* attributes corresponding to queues
 */
static int p_attr [NUM_QUEUES] =
{
	0,							/* "RUNNING" */
	FA_RDONLY,					/* "READY" */
	FA_CHANGED,					/* "WAITING" */
	FA_CHANGED | FA_RDONLY,		/* "IOBOUND" */
	FA_CHANGED | FA_HIDDEN,		/* "ZOMBIE" */
	FA_HIDDEN,					/* "TSR" */
	FA_CHANGED | FA_SYSTEM,		/* "STOPPED" */
	FA_CHANGED | FA_RDONLY		/* "SELECT" (same as IOBOUND) */
};

/*
 * FIXME: this is duplicate to proc_stat64,
 * except for the attr field popuplated from p_attr
 */
static long _cdecl 
proc_getxattr (fcookie *fc, XATTR *xattr)
{
	struct proc *p = getproc (fc->index);
	
	xattr->nblocks = 1;
	xattr->blksize = 1;
	if (!p)
	{
		/* the root directory */
		
		xattr->index = 0;
		xattr->dev = xattr->rdev = PROCDRV;
		xattr->nlink = 1;
		xattr->uid = xattr->gid = 0;
		xattr->size = xattr->nblocks = 0;
		xattr->nlink = 2;

		SET_XATTR_TD(xattr,a,xtime.tv_sec);
		SET_XATTR_TD(xattr,m,procfs_stmp.tv_sec);
		SET_XATTR_TD(xattr,c,rootproc->started.tv_sec);
		xattr->mode = S_IFDIR | DEFAULT_DIRMODE;
		xattr->attr = FA_DIR;
		
		return E_OK;
	}
	
	xattr->index = p->pid;
	xattr->dev = xattr->rdev = PROC_RDEV_BASE | p->pid;
	xattr->nlink = 1;
	xattr->uid = p->p_cred->ucr->euid;
	xattr->gid = p->p_cred->ucr->egid;
	xattr->size = xattr->nblocks = memused (p);

	SET_XATTR_TD(xattr,a,xtime.tv_sec);
	SET_XATTR_TD(xattr,m,p->started.tv_sec);
	SET_XATTR_TD(xattr,c,p->started.tv_sec);
	xattr->mode = S_IFMEM | S_IRUSR | S_IWUSR;
	xattr->attr = p_attr[p->wait_q];
	
	return E_OK;
}

static long _cdecl 
proc_chown (fcookie *fc, int uid, int gid)
{
	struct proc *p = getproc (fc->index);
	
	if (!p)
		return EACCES;
	
	if (uid != -1) p->p_cred->ruid = p->p_cred->ucr->euid = uid;
	if (gid != -1) p->p_cred->rgid = p->p_cred->ucr->egid = gid;
	
	return E_OK;
}

static long _cdecl
proc_stat64 (fcookie *fc, STAT *ptr)
{
	struct proc *p = getproc (fc->index);
	
	mint_bzero (ptr, sizeof (*ptr));
	
	ptr->blocks = 1;
	ptr->blksize = 1;
	ptr->nlink = 1;
	if (!p)
	{
		/* the root directory */
		
		ptr->dev = ptr->rdev = PROCDRV;
		ptr->mode = S_IFDIR | DEFAULT_DIRMODE;
		
		ptr->atime.high_time = 0;
		ptr->atime.time = xtime.tv_sec;
		
		ptr->mtime.high_time = 0;
		ptr->mtime.time = procfs_stmp.tv_sec;
		ptr->mtime.nanoseconds = 0;	
		
		ptr->ctime.high_time = 0;
		ptr->ctime.time = rootproc->started.tv_sec;
		ptr->ctime.nanoseconds = 0;	
		ptr->nlink = 2;
		
		return E_OK;
	}
	
	ptr->ino = p->pid;
	ptr->dev = ptr->rdev = PROC_RDEV_BASE | p->pid;
	ptr->uid = p->p_cred->ucr->euid;
	ptr->gid = p->p_cred->ucr->egid;
	ptr->size = ptr->blocks = memused(p);
	ptr->mode = S_IFMEM | S_IRUSR | S_IWUSR;

	ptr->atime.high_time = 0;
	ptr->atime.time = xtime.tv_sec;
	ptr->atime.nanoseconds = 0;	
	
	ptr->mtime.high_time = 0;
	ptr->mtime.time = p->started.tv_sec;
	ptr->mtime.nanoseconds = 0;	
	
	ptr->ctime.high_time = 0;
	ptr->ctime.time = p->started.tv_sec;
	ptr->ctime.nanoseconds = 0;	
	
	return E_OK;
}

static long _cdecl 
proc_remove (fcookie *dir, const char *name)
{
	PROC *t;
	
	if (dir->index != 0)
		return ENOTDIR;
	
	t = name2proc (name);
	if (!t)
		return ENOENT;
	
	/* this check is necessary because the Fdelete code checks for
	 * write permission on the directory, not on individual
	 * files
	 * 
	 * Draco: it was
	 *	if (curproc->euid && curproc->ruid != p->ruid)
	 * what allowed regular users to remove root processes using ftpd.
	 * (coz daemons start with root privileges, then downgrade themselves,
	 * nevertheless, ruid remains a zero).
	 */
	if (!suser (get_curproc()->p_cred->ucr) && get_curproc()->p_cred->ucr->euid != t->p_cred->ucr->euid)
	{
		DEBUG(("proc_remove: wrong user"));
		return EACCES;
	}
	
	post_sig (t, SIGTERM);
	check_sigs ();		/* it might have been us */
	
	return E_OK;
}

static long _cdecl 
proc_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
	PROC *p;
	char buffer[PNAMSIZ + 5]; /* enough if proc names no longer than 8 chars */
	
	UNUSED (root);
	
	if (dir->index == 0)
	{
		*buffer = 0;
	}
	else
	{
		p = getproc (dir->index);
		if (!p) return EBADARG;
		
		ksprintf (buffer, sizeof (buffer), "%s.%03d", p->name, p->pid);
	}
	
	if (strlen (buffer) < size)
	{
		strcpy (pathname, buffer);
		return E_OK;
	}
	
	return EBADARG;
}

static long _cdecl 
proc_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	PROC *p;
	int i;
	
	if (olddir->index != 0 || newdir->index != 0)
		return ENOTDIR;
	
	p = name2proc (oldname);
	if (!p)
		return ENOENT;
	
	oldname = p->name;
	for (i = 0; i < PNAMSIZ; i++)
	{
		if (*newname == 0 || *newname == '.')
		{
			*oldname = 0;
			break;
		}
		
		*oldname++ = *newname++;
	}
	
	return E_OK;
}

static long _cdecl 
proc_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	PROC *p;
	int i;
	short pid;
	char namebuf[PNAMSIZ + 4 + 1];

	pid = 0;
	for (;;)
	{
		i = dirh->index++;
		
		if (i == 0)
		{
			strcpy(namebuf, ".");
			p = 0;
			break;
		}
		if (i == 1)
		{
			strcpy(namebuf, "..");
			p = 0;
			break;
		}
		/* BUG: we shouldn't have the magic value "MAXPID" for
		 * maximum proc pid
		 */
		if (i >= MAXPID + 2)
		{
			return ENMFILES;
		}
		
		p = pid2proc (i - 2);
		if (p)
		{
			pid = p->pid;
			ksprintf (namebuf, sizeof(namebuf), "%s.%03d", p->name, pid);
			break;
		}
	}

	fc->index = (long) p;
	fc->fs = &proc_filesys;
	fc->dev = PROC_RDEV_BASE | pid;
	
	if (!(dirh->flags & TOS_SEARCH))
	{
		namelen -= 4;
		if (namelen <= 0)
			return EBADARG;
		
		*((long *) name) = (long) pid;
		name += 4;
	}
	
	/* The MiNT documentation appendix E states that we always have
	 * to stuff as many bytes as possible into the buffer.
	 */
	strncpy_f (name, namebuf, namelen);
	if (strlen (namebuf) >= namelen)
		return EBADARG;
	
	return E_OK;
}

static long _cdecl 
proc_pathconf (fcookie *dir, int which)
{
	UNUSED(dir);
	
	switch (which)
	{
		case DP_INQUIRE:	return DP_XATTRFIELDS;
		case DP_IOPEN:		return UNLIMITED;	/* no internal limit on open files */
		case DP_MAXLINKS:	return 1;		/* we don't have hard links */
		case DP_PATHMAX:	return PATH_MAX;	/* max. path length */
		case DP_NAMEMAX:	return PNAMSIZ + 4;	/* max. length of individual name */
								/* the "+4" is for the pid: ".123" */
		case DP_ATOMIC:		return UNLIMITED;	/* all writes are atomic */
		case DP_TRUNC:		return DP_DOSTRUNC;	/* file names are truncated to 8.3 */
		case DP_CASE:		return DP_CASEINSENS;	/* case preserved, but ignored */
		case DP_MODEATTR:	return (0777L << 8) | DP_FT_DIR | DP_FT_MEM;
		case DP_XATTRFIELDS:	return (DP_INDEX
						| DP_DEV
						| DP_NLINK
						| DP_UID
						| DP_GID
						| DP_BLKSIZE
						| DP_SIZE
						| DP_NBLOCKS
					);
	}
	
	return ENOSYS;
}

static long _cdecl 
proc_dfree (fcookie *dir, long *buf)
{
	long size;
	
	/* "sector" size is the size of the smallest amount of memory that
	 * can be allocated. see mem.h for the definition of ROUND
	 */
	long secsiz = ROUND(1);
	
	UNUSED(dir);
	
	size = tot_rsize(core, 0) + tot_rsize(alt, 0);
	*buf++ = size/secsiz;			/* number of free clusters */
	size = tot_rsize(core, 1) + tot_rsize(alt, 1);
	*buf++ = size/secsiz;			/* total number of clusters */
	*buf++ = secsiz;			/* sector size (bytes) */
	*buf = 1;				/* cluster size (in sectors) */
	
	return E_OK;
}

static long _cdecl 
proc_readlabel (fcookie *dir, char *name, int namelen)
{
	UNUSED (dir);
	
	if (sizeof "Processes" >= namelen)
		return EBADARG;
	
	strcpy (name, "Processes");
	
	return E_OK;
}

static DEVDRV * _cdecl 
proc_getdev (fcookie *fc, long *devsp)
{
	*devsp = fc->index;
	return &proc_device;
}

static long _cdecl
proc_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{	
	UNUSED (dir); 
	UNUSED (name);
	
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "proc");
			return E_OK;
		}
	}
	
	return ENOSYS;
}

/*
 * PROC device driver
 */

/*
 * BUG: file locking and the O_SHMODE restrictions are not implemented
 * for processes
 */

static long _cdecl 
proc_open (FILEPTR *f)
{
	UNUSED (f);
	
	return E_OK;
}

static long _cdecl 
proc_write (FILEPTR *f, const char *buf, long nbytes)
{
	struct proc *p = getproc (f->devinfo);
	MEMREGION *m;
	unsigned long where;
	long bytes_written, txtsize;
	int prot_hold;

	where = f->pos;

	TRACE(("proc_write to pid %d: %ld bytes to %lx", p->pid, nbytes, where));

	if (p->pid == get_curproc()->ppid)
	{
		DEBUG(("proc_write: pid %d, attempt to write to parent memory", get_curproc()->pid));
		return EACCES;
	}

	/* Hmmm, maybe we are writing to the process descriptor */
	if ((unsigned long) p <= where && where < (unsigned long)(p + 1))
	{
# if 1
		return EACCES;
# else
		short save_id[7];
		ushort save_flags;
		long save_limit[4];
		
		bytes_written = (unsigned long)(p + 1) - where;
		
		if (bytes_written > nbytes)
			bytes_written = nbytes;
		
		/* Some code against hacking superuser privileges in MiNT :-)
		 * (draco)
		 */
		
		if (secure_mode && get_curproc()->euid)
		{
			save_id[0] = p->rgid;
			save_id[1] = p->ruid;
			save_id[2] = p->egid;
			save_id[3] = p->euid;
			save_id[4] = p->suid;
			save_id[5] = p->sgid;
			save_id[6] = p->auid;

			save_flags = p->p_mem->memflags;

			save_limit[0] = p->maxmem;
			save_limit[1] = p->maxdata;
			save_limit[2] = p->maxcore;
			save_limit[3] = p->maxcpu;

			quickmovb((void *)where, buf, bytes_written);

			/* There are other system calls to set this stuff, so
			 * if someone changes these settings here, it may be an attempt
			 * to workaround the usual privilege protection code.
			 * So we restore all this unconditionally.
			 */

			p->rgid = save_id[0];
			p->ruid = save_id[1];
			p->egid = save_id[2];
			p->euid = save_id[3];
			p->suid = save_id[4];
			p->sgid = save_id[5];
			p->auid = save_id[6];

			/* Here be picky only on the F_OS_SPECIAL flag */
			if ((p->p_mem->memflags & F_OS_SPECIAL) && !(save_flags & F_OS_SPECIAL))
				p->p_mem->memflags &= ~(F_OS_SPECIAL);

			p->maxmem = save_limit[0];
			p->maxdata = save_limit[1];
			p->maxcore = save_limit[2];
			p->maxcpu = save_limit[3];
		}
		else
			quickmovb((void *)where, buf, bytes_written);

		cpush((void *)where, bytes_written);	/* flush cached data */
		f->pos += bytes_written;
		return bytes_written;
# endif
	}

	while (nbytes > 0) {
	/* get the region of the process matching f->pos */
		m = proc_addr2region(p, where);
		if (!m) {
			DEBUG(("proc_write: Memory not owned by process %d", p->pid));
			if (where == f->pos)
				return EACCES;
			break;
		}

	/* we should tell the memory protection that we need access to
	 * the region. if we are writing to a saveplace region, the
	 * kernel always has access to the memory, so it suffices to get
	 * access to the "real" region.
	 */
		prot_hold = prot_temp(m->loc, m->len, -1);
		assert(prot_hold < 0);

		bytes_written = m->loc + m->len - where;
		if (bytes_written > nbytes)
			bytes_written = nbytes;

		if (!m->save) {
		/* copy the memory to the "real" region */
			quickmovb((void *)where, buf, bytes_written);
			cpush((void *)where, bytes_written);
			buf += bytes_written;
			where += bytes_written;
			nbytes -= bytes_written;
		} else if (!(txtsize = (long)m->save->save)) {
		/* copy the memory from the saveplace */
			quickmovb((void *)(m->save->loc + (where - m->loc)), buf,
				  bytes_written);
			cpush((void *)where, bytes_written);
			buf += bytes_written;
			where += bytes_written;
			nbytes -= bytes_written;
		} else {
		/* Nightmare: this is a saved TPA region with a non-zero
		 * text section, i.e. first 256 bytes (basepage) are in
		 * the save region, then follows the text section in the
		 * "real" region and finally the rest again in the saveplace
		 * region.
		 */
			if (where < m->loc + 256) {
				bytes_written = m->loc + 256 - where;
				if (bytes_written > nbytes)
					bytes_written = nbytes;
				quickmovb((void *)(m->save->loc + (where - m->loc)), buf, bytes_written);
				cpush((void *)where, bytes_written);
				buf += bytes_written;
				where += bytes_written;
				nbytes -= bytes_written;
			}
			
			if (nbytes && where < m->loc + 256 + txtsize) {
				bytes_written = m->loc + 256 + txtsize - where;
				if (bytes_written > nbytes)
					bytes_written = nbytes;
				quickmovb((void *)where, buf, bytes_written);
				cpush((void *)where, bytes_written);
				buf += bytes_written;
				where += bytes_written;
				nbytes -= bytes_written;
			}

			if (nbytes) {
				bytes_written = m->loc + m->len - where;
				if (bytes_written > nbytes)
					bytes_written = nbytes;
				quickmovb((void *)(m->save->loc + (where - m->loc - txtsize)), buf, bytes_written);
				cpush((void *)where, bytes_written);
				buf += bytes_written;
				where += bytes_written;
				nbytes -= bytes_written;
			}
		}

		if (prot_hold != -1)
			prot_temp(m->loc, m->len, prot_hold);
	}

	bytes_written = where - f->pos;

	f->pos += bytes_written;
	return bytes_written;
}

static long _cdecl 
proc_read (FILEPTR *f, char *buf, long nbytes)
{
	struct proc *p = getproc (f->devinfo);
	MEMREGION *m;
	long where, bytes_read, txtsize;
	int prot_hold;

	where = f->pos;

	TRACE(("proc_read from pid %d: %ld bytes from %lx", p->pid, nbytes, where));

	/* Hmmm, maybe we are reading from the process descriptor */
	if ((long)p <= where && where < (long)(p + 1)) {
		bytes_read = (long)(p + 1) - where;
		if (bytes_read > nbytes)
			bytes_read = nbytes;
		quickmovb(buf, (void *)where, bytes_read);
		f->pos += bytes_read;
		return bytes_read;
	}

	while (nbytes > 0) {
	/* get the region of the process matching f->pos */
		m = proc_addr2region(p, where);
		if (!m) {
			DEBUG(("proc_read: Memory not owned by process %d", p->pid));
			if (where == f->pos)
				return EACCES;
			break;
		}

	/* we should tell the memory protection that we need access to
	 * the region. if we are reading from a saveplace region, the
	 * kernel always has access to the memory, so it suffices to get
	 * access to the "real" region.
	 */
		prot_hold = prot_temp(m->loc, m->len, -1);
		assert(prot_hold < 0);

		bytes_read = m->loc + m->len - where;
		if (bytes_read > nbytes)
			bytes_read = nbytes;

		if (!m->save) {
		/* copy the memory from the "real" region */
			quickmovb(buf, (void *)where, bytes_read);
			buf += bytes_read;
			where += bytes_read;
			nbytes -= bytes_read;
		} else if (!(txtsize = (long)m->save->save)) {
		/* copy the memory from the saveplace */
			quickmovb(buf, (void *)(m->save->loc + (where - m->loc)),
				  bytes_read);
			buf += bytes_read;
			where += bytes_read;
			nbytes -= bytes_read;
		} else {
		/* Nightmare: this is a saved TPA region with a non-zero
		 * text section, i.e. first 256 bytes (basepage) are in
		 * the save region, then follows the text section in the
		 * "real" region and finally the rest again in the saveplace
		 * region.
		 */
			if (where < m->loc + 256) {
				bytes_read = m->loc + 256 - where;
				if (bytes_read > nbytes)
					bytes_read = nbytes;
				quickmovb(buf,
					  (void *)(m->save->loc +
					      (where - m->loc)),
					  bytes_read);
				buf += bytes_read;
				where += bytes_read;
				nbytes -= bytes_read;
			}
			
			if (nbytes && where < m->loc + 256 + txtsize) {
				bytes_read = m->loc + 256 + txtsize - where;
				if (bytes_read > nbytes)
					bytes_read = nbytes;
				quickmovb(buf, (void *)where, bytes_read);
				buf += bytes_read;
				where += bytes_read;
				nbytes -= bytes_read;
			}

			if (nbytes) {
				bytes_read = m->loc + m->len - where;
				if (bytes_read > nbytes)
					bytes_read = nbytes;
				quickmovb(buf,
					  (void *)(m->save->loc +
					      (where - m->loc - txtsize)),
					  bytes_read);
				buf += bytes_read;
				where += bytes_read;
				nbytes -= bytes_read;
			}
		}

		if (prot_hold != -1)
			prot_temp(m->loc, m->len, prot_hold);
	}

	bytes_read = where - f->pos;

	f->pos += bytes_read;
	return bytes_read;
}

/*
 * proc_ioctl: currently, the only IOCTL's available are:
 * PPROCADDR: get address of PROC structure's "interesting" bits
 * PCTXTSIZE: get the size of the CONTEXT structure
 * PBASEADDR: get address of process basepage
 * PSETFLAGS: set the memory allocation flags (e.g. to malloc from fastram)
 * PGETFLAGS: get the memory allocation flags
 * PTRACESFLAGS: set the process tracing flags
 * PTRACEGFLAGS: get the process tracing flags
 * PTRACEGO: restart the process (T1=0/T1=0)
 * PTRACEFLOW: restart the process (T1=0/T0=1)
 * PTRACESTEP: restart the process (T1=1/T0=0)
 * PTRACE11: restart the process (T1=1/T0=1)
 * PLOADINFO: get information about the process name and command line
 */

static long _cdecl 
proc_ioctl (FILEPTR *f, int mode, void *buf)
{
	struct proc *p = getproc (f->devinfo);
	
	if (!p)
		return EACCES;
	
	switch (mode)
	{
		case FIONREAD:
		case FIONWRITE:
		{
			/* we're always ready for i/o */
			*((long *) buf) = 1L;
			return E_OK;
		}
		case FIOEXCEPT:
		{
			*((long *) buf) = 0L;
			return E_OK;
		}

		/* Security hole: knowing the address of the proc struct,
		 * which belongs to the calling process, anyone can manually
		 * set rgid/ruid/egid/euid fields and become the root.
		 *
		 * Procedure:
		 *
		 * 	long proc; short fd;
		 *
		 * 	fd = Fopen("u:\\proc\\.XXX", O_RDWR);
		 * 	(void)Fcntl(fd, &proc, PPROCADDR);
		 *	(void)Fclose(fd);
		 *	proc += 14L;	*(short *)proc = 0;
		 *	proc += 2L;	*(short *)proc = 0;
		 *	proc += 2L;	*(short *)proc = 0;
		 *	proc += 2L;	*(short *)proc = 0;
		 *	return Pexec(200, "u:\\bin\\tcsh", "", 0L);
		 *
		 * and voil, we have superuser shell :-)
		 *
		 * A fix? Enable memory protection... (draco)
		 *
		 */

		case PPROCADDR:
		{
			*((long *) buf) = (long) &p->magic;
			return E_OK;
		}
		case PBASEADDR:
		{
			if (p->p_mem)
				*((long *) buf) = (long) p->p_mem->base;
			else
				*((long *) buf) = 0;
			
			return E_OK;
		}
		case PCTXTSIZE:
		{
			*((long *) buf) = sizeof (CONTEXT);
			return E_OK;
		}
		case PFSTAT:
		{
			struct filedesc *fd = p->p_fd;
			FILEPTR *pf;
			int pfd;
			
			if (!fd)
			{
				DEBUG (("ioctl(PFSTAT): no filedesc struct"));
				return EBADF;
			}
			
			pfd = (*(short *) buf);
			if ((pfd < MIN_HANDLE) || (pfd >= fd->nfiles)
				|| ((pf = fd->ofiles[pfd]) == 0))
			{
				return EBADF;
			}
			
			return xfs_getxattr (pf->fc.fs, &pf->fc, (XATTR *) buf);
		}
		case PSETFLAGS:
		{
			int newflags = (ushort)(*(long *) buf);
			
			if ((newflags & F_OS_SPECIAL)
				&& (!(p->p_mem->memflags & F_OS_SPECIAL)))
			{
				/* Restrict F_OS_SPECIAL to root. Some
				 * day this flag will go away, I hope.
				 */

				if ((secure_mode) && (get_curproc()->p_cred->ucr->euid))
					return EPERM;
				/* you're making the process OS_SPECIAL */
				TRACE (("Fcntl OS_SPECIAL pid %d",p->pid));
				p->p_mem->memflags = newflags;
				mem_prot_special (p);
			}
			
			/* note: only the low 16 bits are actually used */
			p->p_mem->memflags = *((long *) buf);
			
			return E_OK;
		}
		case PGETFLAGS:
		{
			*((long *) buf) = p->p_mem->memflags;
			return E_OK;
		}
		case PTRACESFLAGS:
		{
			if (p->ptracer == get_curproc() || p->ptracer == 0)
			{
				p->ptraceflags = *(ushort *) buf;
				if (p->ptraceflags == 0)
				{
					p->ptracer = 0;
					p->ctxt[CURRENT].ptrace = 0;
					p->ctxt[SYSCALL].ptrace = 0;
					
					/* if the process is stopped, restart it */
					if (p->wait_q == STOP_Q)
					{
						p->sigpending &= ~STOPSIGS;
						post_sig (p, SIGCONT);
					}
				}
				else if (p == get_curproc())
				{
					p->ptracer = pid2proc (p->ppid);
				}
				else
				{
					p->ptracer = get_curproc();
				}
			}
			else
			{
				DEBUG (("proc_ioctl: process already being traced"));
				return EACCES;
			}
			
			return E_OK;
		}
		case PTRACEGFLAGS:
		{
			if (p->ptracer == get_curproc())
			{
				*(ushort *) buf = p->ptraceflags;
				return E_OK;
			}
			
			return EACCES;
		}
		case PTRACE11:
		{
			return ENOSYS;
		}
		case PTRACEFLOW:
		{
# ifdef M68000
			if (mcpu < 20)
			{
				DEBUG( ("proc_ioctl: wrong processor"));
				return ENOSYS;
			}
# endif
		}
		/* fall through */
		case PTRACEGO:
		case PTRACESTEP:
		{
			if (!p->ptracer)
			{
				DEBUG (("proc_ioctl(PTRACE): process not being traced"));
				return EACCES;
			}
			else if (p->wait_q != STOP_Q)
			{
				DEBUG (("proc_ioctl(PTRACE): process not stopped"));
				return EACCES;
			}
			else if (p->wait_cond
				&& (1L << ((p->wait_cond >> 8) & 0x1f)) & STOPSIGS)
			{
				DEBUG (("proc_ioctl(PTRACE): process stopped by job control"));
				return EACCES;
			}
			
			if (buf && *(ushort *) buf >= NSIG)
			{
				DEBUG (("proc_ioctl(PTRACE): illegal signal number"));
				return EBADARG;
			}
			
			p->ctxt[SYSCALL].sr &= 0x3fff;	/* clear both trace bits */
			p->ctxt[SYSCALL].sr |= (mode - PTRACEGO) << 14;
			/* Discard the saved frame */
			p->ctxt[SYSCALL].sfmt = 0;
			p->sigpending = 0;
			
			if (buf && *(ushort *) buf != 0)
			{
				TRACE (("PTRACEGO: sending signal %d to pid %d", *(ushort *)buf, p->pid));
				post_sig (p, *(ushort *)buf);
				
				/* another SIGNULL hack... within check_sigs()
				 * we watch for a pending SIGNULL, if we see
				 * this then we allow delivery of a signal to
				 * the process, rather than telling the
				 * parent.
				 */
				p->sigpending |= 1L;
			}
			else
			{
				TRACE(("PTRACEGO: no signal"));
			}
			
			/* wake the process up */
			{
				ushort sr = spl7 ();
				rm_q (p->wait_q, p);
				add_q (READY_Q, p);
				spl (sr);
			}
			
			return E_OK;
		}
		case PLOADINFO:
		{
			/* jr: PLOADINFO returns information about params
			 *     passed to Pexec
			 */
			struct ploadinfo *pl = buf;
			
			if (!p->fname[0])
				return ENOENT;
			
			if (strlen (p->fname) >= pl->fnamelen)
				return EBADARG;
			
			strncpy (pl->cmdlin, p->cmdlin, 128);
			strcpy (pl->fname, p->fname);
			
			return E_OK;
		}
		case PMEMINFO:
		{
			struct memspace *mem = p->p_mem;
			struct pmeminfo *mi = buf;
			int i;
			
			if (!mem || !mem->mem)
			{
				DEBUG (("ioctl(PMEMINFO): no memspace struct"));
				return EBADARG;
			}
			
			for (i = 0; i < mem->num_reg; i++)
			{
				MEMREGION *m;
				
				if (i == mi->mem_blocks)
					return ENOMEM;
				
				m = mem->mem [i];
				if (m)
				{
					mi->mlist[i]->loc = m->loc;
					mi->mlist[i]->len = m->len;
					mi->mlist[i]->flags = get_prot_mode (m);
				}
			}
			
			return E_OK;
		}
		default:
		{
			DEBUG (("procfs: bad Fcntl command"));
			/* return EINVAL; */
		}
	}
	
	return ENOSYS;
}

static long _cdecl 
proc_lseek (FILEPTR *f, long where, int whence)
{
	switch (whence)
	{
		case SEEK_SET:
		case SEEK_END:
			f->pos = where;
			break;
		case SEEK_CUR:
			f->pos += where;
			break;
		default:
			return ENOSYS;
	}
	
	return f->pos;
}

static long _cdecl 
proc_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	struct proc *p = getproc (f->devinfo);
	
	if (!p)
		return EACCES;
	
	switch (flag)
	{
		case 0:
		{
			*(long *) timeptr = p->started.tv_sec;
			break;
		}
		case 1:
		{
			return EACCES;
		}
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl 
proc_close (FILEPTR *f, int pid)
{
	UNUSED (f);
	UNUSED (pid);
	
	return E_OK;
}
