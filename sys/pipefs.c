/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* simple pipefs.c */

# include "pipefs.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/signal.h"

# include "filesys.h"
# include "memory.h"
# include "kmemory.h"
# include "nullfs.h"
# include "proc.h"
# include "signal.h"
# include "time.h"
# include "tty.h"


static long	_cdecl pipe_root	(int drv, fcookie *fc);
static long	_cdecl pipe_lookup	(fcookie *dir, const char *name, fcookie *fc);
static long	_cdecl pipe_getxattr	(fcookie *file, XATTR *xattr);
static long	_cdecl pipe_stat64	(fcookie *fc, STAT *ptr);
static long	_cdecl pipe_chown	(fcookie *file, int uid, int gid);
static long	_cdecl pipe_chmode	(fcookie *file, unsigned mode);
static long	_cdecl pipe_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl pipe_opendir	(DIR *dirh, int flags);
static long	_cdecl pipe_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl pipe_pathconf	(fcookie *dir, int which);
static long	_cdecl pipe_dfree	(fcookie *dir, long *buf);
static long	_cdecl pipe_creat	(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static DEVDRV *	_cdecl pipe_getdev	(fcookie *fc, long *devsp);
static long	_cdecl pipe_fscntl	(fcookie *dir, const char *name, int cmd, long arg);

static long	_cdecl pipe_open	(FILEPTR *f);
static long	_cdecl pipe_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl pipe_read	(FILEPTR *f, char *buf, long bytes);
static long	_cdecl pty_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl pty_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl pty_writeb	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl pty_readb	(FILEPTR *f, char *buf, long bytes);
static long	_cdecl pipe_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl pipe_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl pipe_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl pipe_close	(FILEPTR *f, int pid);
static long	_cdecl pipe_select	(FILEPTR *f, long p, int mode);
static void	_cdecl pipe_unselect	(FILEPTR *f, long p, int mode);


FILESYS pipe_filesys =
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
	
	pipe_root,
	pipe_lookup, pipe_creat, pipe_getdev, pipe_getxattr,
	null_chattr, pipe_chown, pipe_chmode,
	null_mkdir, null_rmdir, null_remove, pipe_getname, null_rename,
	pipe_opendir, pipe_readdir, null_rewinddir, null_closedir,
	pipe_pathconf, pipe_dfree, null_writelabel, null_readlabel,
	null_symlink, null_readlink, null_hardlink, pipe_fscntl, null_dskchng,
	NULL, NULL,
	NULL,
	
	/* FS_EXT_1 */
	NULL, NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	pipe_stat64,
	
	0, 0, 0, 0, 0,
	NULL, NULL
};

DEVDRV pty_device =
{
	pipe_open,
	pty_write, pty_read, pipe_lseek,
	pipe_ioctl, pipe_datime,
	pipe_close,
	pipe_select, pipe_unselect,
	pty_writeb, pty_readb
};
 
DEVDRV pipe_device =
{
	pipe_open,
	pipe_write, pipe_read, pipe_lseek,
	pipe_ioctl, pipe_datime,
	pipe_close,
	pipe_select, pipe_unselect,
	NULL, NULL
};


/* size of pipes */
#define PIPESIZ	4096		/* MUST be a multiple of 4 */

/* writes smaller than this are atomic */
#define PIPE_BUF 1024		/* should be a multiple of 4 */

/* magic flag: indicates that nobody but the creator has opened this pipe */
/* note: if this many processes open the pipe, we lose :-( */
#define VIRGIN_PIPE	0x7fff

struct pipe
{
	int	readers;	/* number of readers of this pipe */
	int	writers;	/* number of writers of this pipe */
	int	start, len;	/* pipe head index, size */
	long	rsel;		/* process that did select() for reads */
	long	wsel;		/* process that did select() for writes */
	char	buf[PIPESIZ];	/* pipe data */
};

struct fifo* piperoot;
struct timeval pipestamp;

static long _cdecl 
pipe_root (int drv, fcookie *fc)
{
	if (drv == PIPEDRV)
	{
		fc->fs = &pipe_filesys;
		fc->dev = drv;
		fc->index = 0L;
		return E_OK;
	}
	
	fc->fs = 0;
	return EINTERNAL;
}

static long _cdecl 
pipe_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	struct fifo *b;
	
	TRACE (("pipe_lookup(%s)", name));
	
	if (dir->index != 0)
	{
		DEBUG (("pipe_lookup(%s): bad directory", name));
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
	
	/* another special case: ".." could be a mount point
	 */
	if (!strcmp (name, ".."))
	{
		*fc = *dir;
		return EMOUNT;
	}
	
	for (b = piperoot; b; b = b->next)
	{
		if (!strnicmp (b->name, name, NAME_MAX))
		{
			fc->fs = &pipe_filesys;
			fc->index = (long) b;
			fc->dev = dir->dev;
			return E_OK;
		}
	}
	
	DEBUG (("pipe_lookup: name `%s' not found", name));
	return ENOENT;
}

static long _cdecl 
pipe_getxattr (fcookie *fc, XATTR *xattr)
{
	xattr->index = fc->index;
	xattr->dev = fc->dev;
	xattr->rdev = fc->dev;
	xattr->nlink = 1;
	xattr->blksize = 1024L;
	
	if (fc->index == 0)
	{
		/* root directory */
		
		xattr->uid = xattr->gid = 0;
		
		*(long *) &xattr->mtime = pipestamp.tv_sec;
		*(long *) &xattr->atime = xtime.tv_sec;
		*(long *) &xattr->ctime = rootproc->started.tv_sec;
		
		xattr->mode = S_IFDIR | DEFAULT_DIRMODE;
		xattr->attr = FA_DIR;
		xattr->size = xattr->nblocks = 0;
	}
	else
	{
		struct fifo *this = (struct fifo *) fc->index;
		
		*(long *) &xattr->mtime = this->mtime.tv_sec;
		*(long *) &xattr->atime = xtime.tv_sec;
		*(long *) &xattr->ctime = this->ctime.tv_sec;
		
		xattr->uid = this->uid;
		xattr->gid = this->gid;
		xattr->mode = this->mode;
		xattr->attr = this->dosflags;
		
		/* note: fifo's that haven't been opened yet can be written to
		 */
		if (this->flags & O_HEAD)
		{
			xattr->attr &= ~FA_RDONLY;
		}
		
		if (this->dosflags & FA_SYSTEM)
		{
			/* pseudo-tty */
			xattr->size = PIPESIZ / 4;
			xattr->rdev = PIPE_RDEV | 1;
		}
		else
		{
			xattr->size = PIPESIZ;
			xattr->rdev = PIPE_RDEV | 0;
		}
		
		xattr->nblocks = xattr->size / 1024L;
	}
	
	return E_OK;
}

static long _cdecl
pipe_stat64 (fcookie *fc, STAT *ptr)
{
	bzero (ptr, sizeof (*ptr));
	
	ptr->dev = fc->dev;
	ptr->ino = fc->index;
	ptr->rdev = fc->dev;
	ptr->nlink = 1;
	ptr->blksize = 1024L;
	
	if (fc->index == 0)
	{
		/* root directory */
		
		ptr->mode = S_IFDIR | DEFAULT_DIRMODE;
		
		ptr->atime.time = xtime.tv_sec;
		ptr->mtime.time = pipestamp.tv_sec;
		ptr->ctime.time = rootproc->started.tv_sec;
	}
	else
	{
		struct fifo *this = (struct fifo *) fc->index;
		
		ptr->uid = this->uid;
		ptr->gid = this->gid;
		ptr->mode = this->mode;
		
		ptr->atime.time = xtime.tv_sec;
		ptr->mtime.time = this->mtime.tv_sec;
		ptr->ctime.time = this->ctime.tv_sec;
		
		/* note: fifo's that haven't been opened yet can be written to
		 */
		if (this->flags & O_HEAD)
		{
			ptr->mode &= ~S_IWUGO;
		}
		
		if (this->dosflags & FA_SYSTEM)
		{
			/* pseudo-tty */
			ptr->size = PIPESIZ / 4;
			ptr->rdev = PIPE_RDEV | 1;
		}
		else
		{
			ptr->size = PIPESIZ;
			ptr->rdev = PIPE_RDEV | 0;
		}
		
		ptr->blocks = ptr->size / 1024L;
		
		/* adjust to 512 byte block base size */
		ptr->blocks <<= 2;
	}
	
	return E_OK;
}

static long _cdecl 
pipe_chown (fcookie *fc, int uid, int gid)
{
	struct fifo *this = (struct fifo *) fc->index;
	
	if (!this)
		return EACCES;
	
	if (uid != -1) this->uid = uid;
	if (gid != -1) this->gid = gid;
	
	return E_OK;
}

static long _cdecl 
pipe_chmode (fcookie *fc, unsigned int mode)
{
	struct fifo *this = (struct fifo *) fc->index;
	
	if (!this)
		return EACCES;
	
	this->mode = (this->mode & S_IFMT) | (mode & ~S_IFMT);
	return E_OK;
}

static long _cdecl 
pipe_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
	char *pipe_name;

	UNUSED(root);

	if (size <= 0)
		return EBADARG;
	if (dir->index == 0)
		*pathname = '\0';
	else
	{
		pipe_name = ((struct fifo *) dir->index)->name;
		if (strlen (pipe_name) < size)
			strcpy (pathname, pipe_name);
		else
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl 
pipe_opendir (DIR *dirh, int flags)
{
	UNUSED (flags);
	
	if (dirh->fc.index != 0)
	{
		DEBUG (("pipe_opendir: bad directory"));
		return ENOTDIR;
	}
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl 
pipe_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	struct fifo *this = piperoot;
	int i = dirh->index++;
	
	while (i && this)
	{
		i--;
		this = this->next;
	}
	
	if (!this)
		return ENMFILES;
	
	fc->fs = &pipe_filesys;
	fc->index = (long) this;
	fc->dev = dirh->fc.dev;
	
	if (dirh->flags == 0)
	{
		namelen -= 4;
		if (namelen <= 0)
			return EBADARG;
		
		*((long *) name) = (long) this;
		name += 4;
	}
	
	if (strlen (this->name) < namelen)
		strcpy (name, this->name);
	else
		return EBADARG;
	
	return E_OK;
}

static long _cdecl 
pipe_pathconf (fcookie *dir, int which)
{
	UNUSED (dir);
	
	switch (which)
	{
		case DP_INQUIRE:
			return DP_XATTRFIELDS;
		case DP_IOPEN:
			return UNLIMITED;	/* no internal limit on open files */
		case DP_MAXLINKS:
			return 1;		/* no hard links */
		case DP_PATHMAX:
			return PATH_MAX;
		case DP_NAMEMAX:
			return NAME_MAX;
		case DP_ATOMIC:
			/* BUG: for pty's, this should actually be PIPE_BUF/4 */
			return PIPE_BUF;
		case DP_TRUNC:
			return DP_AUTOTRUNC;
		case DP_CASE:
			return DP_CASEINSENS;
		case DP_MODEATTR:
			return (0777L << 8) | DP_FT_DIR|DP_FT_FIFO;
		case DP_XATTRFIELDS:
			return DP_INDEX|DP_DEV|DP_NLINK|DP_UID|DP_GID|DP_MTIME;
		default:
			return ENOSYS;
	}
}

static long _cdecl 
pipe_dfree (fcookie *dir, long *buf)
{
	int i;
	struct fifo *b;
	long freemem;
	
	UNUSED (dir);
	
	/* the "sector" size is the number of bytes per pipe
	 * so we get the total number of sectors used by counting pipes
	 */
	
	i = 0;
	for (b = piperoot; b; b = b->next)
	{
		if (b->inp) i++;
		if (b->outp) i++;
	}
	
	freemem = tot_rsize (core, 0) + tot_rsize (alt, 0);
	
	/* note: the "free clusters" isn't quite accurate, since there's
	 * overhead in the fifo structure; but we're not looking for
	 * 100% accuracy here
	 */
	buf[0] = freemem/PIPESIZ;	/* number of free clusters */
	buf[1] = buf[0]+i;		/* total number of clusters */
	buf[2] = PIPESIZ;		/* sector size (bytes) */
	buf[3] = 1;			/* cluster size (sectors) */
	
	return E_OK;
}

/* create a new pipe.
 * this only gets called by the kernel if a lookup already failed,
 * so we know that the new pipe creation is OK
 */

static long _cdecl 
pipe_creat (fcookie *dir, const char *name, unsigned int mode, int attrib, fcookie *fc)
{
	struct pipe *inp, *outp;
	struct tty *tty;
	struct fifo *b;
	
	/* selfread == 1 if we want reads to wait even if no other processes
	 * have currently opened the file, and writes to succeed in the same
	 * event. This is useful for servers who want to wait for requests.
	 * Pipes should always have selfread == 0.
	 */
	int selfread = (attrib & FA_HIDDEN) ? 0 : 1;
	
	/* create the new pipe */
	inp = kmalloc (sizeof (*inp));
	if (!inp)
		return ENOMEM;
	
	if (attrib & FA_RDONLY)
	{
		/* read only FIFOs are unidirectional */
		outp = 0;
	}
	else
	{
		outp = kmalloc (sizeof (*outp));
		if (!outp)
		{
			kfree (inp);
			return ENOMEM;
		}
	}
	
	b = kmalloc (sizeof (*b));
	if (!b)
	{
		if (outp) kfree (outp);
		kfree (inp);
		return ENOMEM;
	}
	
	if (attrib & FA_SYSTEM)
	{
		/* pseudo-tty */
		
		tty = kmalloc (sizeof (*tty));
		if (!tty)
		{
			kfree(b);
			if (outp) kfree(outp);
			kfree(inp);
			return ENOMEM;
		}
		
		tty->use_cnt = 0;
		tty->rsel = tty->wsel = 0;
		   /* do_open does the rest of tty initialization */
	}
	else
		tty = 0;
	
	/* set up the pipes appropriately */
	inp->start = inp->len = 0;
	inp->readers = selfread ? 1 : VIRGIN_PIPE; inp->writers = 1;
	inp->rsel = inp->wsel = 0;
	if (outp)
	{
		outp->start = outp->len = 0;
		outp->readers = 1; outp->writers = selfread ? 1 : VIRGIN_PIPE;
		outp->wsel = outp->rsel = 0;
	}
	strncpy(b->name, name, NAME_MAX);
	b->name[NAME_MAX] = '\0';
	b->mtime = b->ctime = xtime;
	b->dosflags = attrib;
	b->mode = ((attrib & FA_SYSTEM) ? S_IFCHR : S_IFIFO) | (mode & ~S_IFMT);
	b->uid = curproc->euid;
	b->gid = curproc->egid;
	
	/* the O_HEAD flag indicates that the file hasn't actually been opened
	 * yet; the next open gets to be the pty master. pipe_open will
	 * clear the flag when this happens.
	 */
	b->flags = ((attrib & FA_SYSTEM) ? O_TTY : 0) | O_HEAD;
	b->lockpid = b->cursrate = 0;
	b->inp = inp; b->outp = outp; b->tty = tty;
	
	b->next = piperoot;
	b->open = (FILEPTR *)0;
	piperoot = b;
	
	/* we have to return a file cookie as well */
	fc->fs = &pipe_filesys;
	fc->index = (long)b;
	fc->dev = dir->dev;
	
	/* update time/date stamps for u:\pipe */
	pipestamp = xtime;
	
	return E_OK;
}

static DEVDRV * _cdecl 
pipe_getdev (fcookie *fc, long *devsp)
{
	struct fifo *b = (struct fifo *)fc->index;
	
	UNUSED (devsp);
	return (b->flags & O_TTY) ? &pty_device : &pipe_device;
}

static long _cdecl
pipe_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{	
	UNUSED (dir);
	UNUSED (name);
	
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "pipe-xfs");
			return E_OK;
		}
	}
	
	return ENOSYS;
}

/*
 * PIPE device driver
 */

static long _cdecl 
pipe_open (FILEPTR *f)
{
	struct fifo *p;
	int rwmode = f->flags & O_RWMODE;
	
	p = (struct fifo *) f->fc.index;
	f->flags |= p->flags;
	
	/* if this is the first open for this file, then the O_HEAD flag is
	 * set in p->flags. If not, and someone was trying to create the file,
	 * return an error
	 */
	if (p->flags & O_HEAD)
	{
		if (!(f->flags & O_CREAT))
		{
			DEBUG (("pipe_open: file hasn't been created yet"));
			return EINTERNAL;
		}
		p->flags &= ~O_HEAD;
	}
	else
	{
# if 0
		if (f->flags & O_TRUNC)
		{
			DEBUG (("pipe_open: fifo already exists"));
			return EACCES;
		}
# endif
	}
	
	/* check for file sharing compatibility. note that O_COMPAT gets
	 * mutated into O_DENYNONE, because any old programs that know about
	 * pipes will already handle multitasking correctly
	 */
	if ((f->flags & O_SHMODE) == O_COMPAT)
	{
		f->flags = (f->flags & ~O_SHMODE) | O_DENYNONE;
	}
	
	if (denyshare (p->open, f))
		return EACCES;
	
	f->next = p->open;		/* add this open fileptr to the list */
	p->open = f;
	
	/* add readers/writers to the list
	 */
	if (!(f->flags & O_HEAD))
	{
		if (rwmode == O_RDONLY || rwmode == O_RDWR)
		{
			if (p->inp->readers == VIRGIN_PIPE)
				p->inp->readers = 1;
			else
				p->inp->readers++;
		}
		
		if ((rwmode == O_WRONLY || rwmode == O_RDWR) && p->outp)
		{
			if (p->outp->writers == VIRGIN_PIPE)
				p->outp->writers = 1;
			else
				p->outp->writers++;
		}
	}
	
	/* TTY devices need a tty structure in f->devinfo */
	f->devinfo = (long) p->tty;
	
	p->mtime = xtime;
	
	return E_OK;
}

/* Two little helpers for waking up processes sleeping on our pipe
 */

static void _cdecl
pipe_wake_readers (struct pipe* pipe)
{
	if (pipe->rsel && pipe->len > 0)
		wakeselect ((PROC *) pipe->rsel);
	
	if (pipe->len > 0)
		wake (IO_Q, (long) pipe);
}

static void _cdecl
pipe_wake_writers (struct pipe* pipe)
{
	if (pipe->wsel && pipe->len < PIPESIZ)
		wakeselect ((PROC *) pipe->wsel);
	
	if (pipe->len < PIPESIZ)
		wake (IO_Q, (long) pipe);
}

static long _cdecl 
pipe_write (FILEPTR *f, const char *buf, long nbytes)
{
	int plen, j;
	char *pbuf;
	struct pipe *p;
	struct fifo *this;
	long bytes_written = 0;
	long r;
	
	this = (struct fifo *)f->fc.index;
	p = (f->flags & O_HEAD) ? this->inp : this->outp;
	if (!p)
	{
		DEBUG(("pipe_write: write on wrong end of pipe"));
		return EACCES;
	}
	
	if (nbytes > 0 && nbytes <= PIPE_BUF)
	{
		/* We promised that if the user wants to write less than
		 * PIPE_BUF bytes these write would be atomic.  We have
		 * to wait until at least this number of bytes can be
		 * written to the buffer.
		 */ 
check_atomicity:
		if (is_terminal(f) && !(f->flags & O_HEAD)
			&& (this->tty->state & TS_HOLD))
		{
			if (f->flags & O_NDELAY)
				return 0;
			sleep (IO_Q, (long) &this->tty->state);
			goto check_atomicity;
		}
		
		/* r is the number of bytes we can write */
		r = PIPESIZ - p->len;
		if (r < nbytes)
		{
			/* check for broken pipes */
			if (p->readers == 0 || p->readers == VIRGIN_PIPE)
			{
				check_sigs();
				DEBUG(("pipe_write: broken pipe"));
				raise(SIGPIPE);
				return EPIPE;
			}
			
			/* Now wake up possible readers. */
			pipe_wake_readers (p);
			if (PIPESIZ - p->len < nbytes)
			{
				/* Buffer still full.  Sleep. */
				TRACELOW (("pipe_write: sleep until atomic write possible"));
				sleep(IO_Q, (long)p);
				goto check_atomicity;
			}
			/* else do write now. */
		}
	}
	
	while (nbytes > 0)
	{
		plen = p->len;
		if (plen < PIPESIZ)
		{
			pbuf = &p->buf[(p->start + plen) & (PIPESIZ - 1)];
			/* j is the amount that can be written continuously */
			j = (int)(PIPESIZ - (pbuf - p->buf));
			if (j > nbytes) j = (int)nbytes;
			if (j > PIPESIZ - plen) j = PIPESIZ - plen;
			nbytes -= j; plen += j;
			bytes_written += j;
			quickmovb (pbuf, buf, j);
			buf += j;
			if (nbytes > 0 && plen < PIPESIZ)
			{
			    j = PIPESIZ - plen;
			    if (j > nbytes) j = (int)nbytes;
			    nbytes -= j; plen += j;
			    bytes_written += j;
			    memcpy (p->buf, buf, j);
			    buf += j;
			}
			p->len = plen;
			if (!is_terminal(f) || !(f->flags & O_HEAD)
				|| plen >= this->tty->vmin*4)
			{
				/* is someone select()ing the other end of
				 * the pipe for reading?
				 */
				pipe_wake_readers (p);
			}
		}
		else
		{
			/* pipe full */
			
			if (p->readers == 0 || p->readers == VIRGIN_PIPE)
			{
				/* maybe some other signal is waiting for us? */
				check_sigs();
				DEBUG(("pipe_write: broken pipe"));
				raise(SIGPIPE);
				return EPIPE;
			}
			
			if (f->flags & O_NDELAY)
				break;
			
			/* is someone select()ing the other end of the pipe
			 * for reading?
			 */
			pipe_wake_readers (p);
			if (p->len == plen)
			{
				/* Nobody has read from the pipe.  */
				TRACE (("pipe_write: pipe full: sleep on %lx", p));
				sleep (IO_Q, (long)p);
			}
		}
	}
	
	this->mtime = xtime;
	if (p->len > 0)
		pipe_wake_readers (p);
	
	return bytes_written;
}

static long _cdecl 
pipe_read (FILEPTR *f, char *buf, long nbytes)
{
	int plen, j;
	struct fifo *this;
	struct pipe *p;
	long bytes_read = 0;
	char *pbuf;

	this = (struct fifo *)f->fc.index;
	p = (f->flags & O_HEAD) ? this->outp : this->inp;
	if (!p)
	{
		DEBUG(("pipe_read: read on the wrong end of a pipe"));
		return EACCES;
	}
	
	while (nbytes > 0)
	{
		plen = p->len;
		if (plen > 0)
		{
			pbuf = &p->buf[p->start];
			/* j is the amount that can be read continuously */
			j = PIPESIZ - p->start;
			if (j > nbytes) j = (int)nbytes;
			if (j > plen) j = plen;
			nbytes -= j; plen -= j;
			bytes_read += j;
			p->start += j;
			memcpy (buf, pbuf, j);
			buf += j;
			if (nbytes > 0 && plen > 0) {
			    j = plen;
			    if (j > nbytes) j = (int)nbytes;
			    nbytes -= j; plen -= j;
			    bytes_read += j;
			    p->start = j;
			    memcpy (buf, p->buf, j);
			    buf += j;
			  }
			p->len = plen;
			if (plen == 0 || p->start == PIPESIZ)
			  p->start = 0;
			pipe_wake_writers (p);
		} else if (p->writers <= 0 || p->writers == VIRGIN_PIPE) {
			TRACE(("pipe_read: no more writers"));
			break;
		} else if ((f->flags & O_NDELAY) ||
				((this->dosflags & FA_CHANGED) && bytes_read > 0) ) {
			break;
		} else {
	/* is someone select()ing the other end of the pipe for writing? */
			pipe_wake_writers (p);
			if (p->len == plen) {
				/* Nobody has read from the pipe. */
				TRACE(("pipe_read: pipe empty: sleep on %lx", p));
				sleep(IO_Q, (long)p);
			}
		}
	}
	
	if (p->len < PIPESIZ)
		pipe_wake_writers (p);

	return bytes_read;
}

static long _cdecl 
pty_write (FILEPTR *f, const char *buf, long nbytes)
{
	long bytes_written = 0;
	
	if (!nbytes)
		return 0;
	
	if (f->flags & O_HEAD)
		return pipe_write (f, buf, nbytes);
	
	if (nbytes != 4)
		ALERT ("pty_write: slave nbytes != 4");
	
	bytes_written = pipe_write (f, buf+3, 1);
	if (bytes_written == 1)
		bytes_written = 4;
	
	return bytes_written;
}

static long _cdecl 
pty_read (FILEPTR *f, char *buf, long nbytes)
{
	long bytes_read = 0;
	
	if (!nbytes)
		return 0;
	
	if (!(f->flags & O_HEAD))
		return pipe_read (f, buf, nbytes);
	
	if (nbytes != 4)
		ALERT ("pty_read: master nbytes != 4");
	
	bytes_read = pipe_read (f, buf+3, 1);
	if (bytes_read == 1)
	{
		bytes_read = 4;
		*buf++ = 0;
		*buf++ = 0;
		*buf++ = 0;
	}
	
	return bytes_read;
}

static long _cdecl 
pty_writeb (FILEPTR *f, const char *buf, long nbytes)
{
	if (!nbytes)
		return 0;
	
	if (f->flags & O_HEAD)
		return ENODEV;
	
	return pipe_write (f, buf, nbytes);
}

static long _cdecl 
pty_readb (FILEPTR *f, char *buf, long nbytes)
{
	struct fifo *this = (struct fifo *) f->fc.index;

	if (!nbytes)
		return 0;
	
	if (!(f->flags & O_HEAD))
	{
		struct tty *tty = this->tty;
		
		/* we don't do pty slave reads yet (they need long -> byte
		 * conversion for every char) but we still want to support
		 * VMIN > 1...  so sleep first and then return ENODEV,
		 * then VMIN chars (well longs :) are ready when
		 * tty_read starts reading them one at a time
		 */
		while (tty->vmin > 1 && !tty->vtime &&
		    !(f->flags & O_NDELAY) &&
		    (tty->sg.sg_flags & (T_RAW|T_CBREAK)) &&
		    this->inp->len < tty->vmin*4 && this->inp->writers > 0 &&
		    this->inp->writers != VIRGIN_PIPE)
			sleep (IO_Q, (long)this->inp);
		
		return ENODEV;
	}
	
	/* pty master reads are always RAW
	 */
	if (nbytes > 1 && nbytes > this->outp->len)
	{
		while (!(f->flags & O_NDELAY) &&
		    !this->outp->len && this->outp->writers > 0 &&
		    this->outp->writers != VIRGIN_PIPE)
			sleep (IO_Q, (long)this->outp);
		
		if (nbytes > this->outp->len)
			nbytes = this->outp->len;
	}
	
	return pipe_read (f, buf, nbytes);
}

static long _cdecl 
pipe_ioctl (FILEPTR *f, int mode, void *buf)
{
	struct fifo *this = (struct fifo *) f->fc.index;
	
	long r;
	
	switch (mode)
	{
		case FIONREAD:
		{
			struct pipe *p;
			
			p = (f->flags & O_HEAD) ? this->outp : this->inp;
			if (p == 0)
				return ENOSYS;
			
			r = p->len;
			if (r == 0)
			{
				if (p->writers <= 0 || p->writers == VIRGIN_PIPE)
				{
					DEBUG (("pipe FIONREAD: no writers"));
					
					/* arguably, we should return 0 for EOF,
					 * but this would break MINIWIN and
					 * perhaps some other MultiTOS programs
					 */
					r = -1;
				}
			}
			else if (is_terminal (f))
			{
				if (!(f->flags & O_HEAD))
					r = r >> 2;		/* r /= 4 */
				else if (this->tty->state & TS_HOLD)
					r = 0;
			}
			
			*((long *) buf) = r;
			break;
		}
		case FIONWRITE:
		{
			struct pipe *p;
			
			p = (f->flags & O_HEAD) ? this->inp : this->outp;
			if (p == 0)
				return ENOSYS;
			
			if (p->readers <= 0)
			{
				/* see compatibility comment under FIONREAD */
				r = -1;
			}
			else
			{
				r = PIPESIZ - p->len;
				if (is_terminal (f))
				{
					if (f->flags & O_HEAD)
						r = r >> 2;	/* r /= 4 */
					else if (this->tty->state & TS_HOLD)
						r = 0;
				}
			}
			
			*((long *) buf) = r;
			break;
		}
		case FIOEXCEPT:
		{
			*((long *) buf) = 0;
			break;
		}
		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *lck = (struct flock *) buf;
			
			while (this->flags & O_LOCK)
			{
				if (this->lockpid != curproc->pid)
				{
					DEBUG(("pipe_ioctl: pipe already locked"));
					if (mode == F_SETLKW && lck->l_type != F_UNLCK)
					{
						/* sleep a while */
						sleep (IO_Q, (long) this);
					}
					else
						return ELOCKED;
				} else
					break;
			}
			
			if (lck->l_type == F_UNLCK)
			{
				if (!(f->flags & O_LOCK))
				{
					DEBUG(("pipe_ioctl: wrong file descriptor for UNLCK"));
					return ENSLOCK;
				}
				
				this->flags &= ~O_LOCK;
				this->lockpid = 0;
				f->flags &= ~O_LOCK;
				
				/* wake up anyone waiting on the lock */
				wake (IO_Q, (long) this);	
			}
			else
			{
				this->flags |= O_LOCK;
				this->lockpid = curproc->pid;
				f->flags |= O_LOCK;
			}
			
			break;
		}
		case F_GETLK:
		{
			struct flock *lck = (struct flock *) buf;
			
			if (this->flags & O_LOCK)
			{
				lck->l_type = F_WRLCK;
				lck->l_start = lck->l_len = 0;
				lck->l_pid = this->lockpid;
			}
			else
				lck->l_type = F_UNLCK;
			
			break;
		}
		/* ptys have no DTR line :)  ignore hang up on close...  */
		case TIOCSSTATEB:
		{
			long mask = ((long *) buf)[1] & ~(TS_HOLD|TS_BLIND|TS_HPCL);
			struct tty *tty = this->tty;
			
			if (!is_terminal (f) || !tty)
				return ENOSYS;
			
			if (!(tty->sg.sg_flags & T_XKEY))
				mask &= ~TS_ESC;
			
			if (*(long *) buf != -1)
				tty->state = (tty->state & ~mask) | (*((long *) buf) & mask);
			
			*(long *) buf = tty->state;
			break;
		}
		case TIOCHPCL:
		{
			break;
		}
		case TIOCGVMIN:
		case TIOCSVMIN:
		{
			ushort *v = buf;
			struct tty *tty = this->tty;
			
			if (!is_terminal (f) || !tty)
				return ENOSYS;
			
			if (mode == TIOCGVMIN)
			{
				v[0] = tty->vmin;
				v[1] = tty->vtime;
			}
			else
			{
				if (v[0] > PIPESIZ/4)
					v[0] = PIPESIZ/4;
				tty->vmin = v[0];
				tty->vtime = v[1];
			}
			
			break;
		}
		case TIOCSTART:
		{
			struct pipe *p;
			
			if (is_terminal (f) && !(f->flags & O_HEAD) &&
# if 0
		    		NULL != (p = this->outp) && p->rsel && p->tail != p->head)
# else
		    		NULL != (p = this->outp) && p->rsel && p->len > 0)
# endif
			wakeselect ((PROC *) p->rsel);
			
			break;
		}
		case TIOCFLUSH:
		{
			long flushtype;
			long *which;
			
			which = (long *)buf;
			if (!which || !(*which & 3))
				flushtype = 3;
			else
				flushtype = *which;
			
			if ((flushtype & 1) && this->inp)
			{
				this->inp->start = this->inp->len = 0;
				wake (IO_Q, (long) this->inp);
			}
			
			if ((flushtype & 2) && this->outp)
			{
				this->outp->start = this->outp->len = 0;
				if (!is_terminal(f) || (f->flags & O_HEAD) ||
				    !(this->tty->state & TS_HOLD))
				{
					wake (IO_Q, (long) this->outp);
					if (this->outp->wsel)
						wakeselect ((PROC *) this->outp->wsel);
				}
			}
			
			break;
		}
		case TIOCOUTQ:
		{
			struct pipe *p;
			
			p = (f->flags & O_HEAD) ? this->inp : this->outp;
			if (!p)
				return EBADF;
			
			if (p->readers <= 0)
			{
				r = -1;
			}
			else
			{
				r = p->len;
				if (is_terminal (f) && (f->flags & O_HEAD))
					r = r >> 2;	/* r /= 4 */
			}
			
			*((long *) buf) = r;
			break;
		}
		case TIOCIBAUD:
		case TIOCOBAUD:
		{
			*(long *) buf = -1L;
			break;
		}
		case TIOCGFLAGS:
		{
			*((ushort *) buf) = 0;
			break;
		}
		case TCURSOFF:
		case TCURSON:
		case TCURSSRATE:
		case TCURSBLINK:
		case TCURSSTEADY:
		{
			/* kludge: this assumes TOSWIN style escape sequences */
			tty_putchar (f, (long)'\033', RAW);
			switch (mode)
			{
				case TCURSOFF:
				{
					tty_putchar (f, (long)'f', RAW);
					break;
				}
				case TCURSON:
				{
					tty_putchar (f, (long)'e', RAW);
					break;
				}
				case TCURSSRATE:
				{
					this->cursrate = *((short *) buf);
					/* fall through */
				}
				case TCURSBLINK:
				{
					tty_putchar (f, (long)'t', RAW);
					tty_putchar (f, (long) this->cursrate+32, RAW);
					break;
				}
				case TCURSSTEADY:
				{
					tty_putchar (f, (long)'t', RAW);
					tty_putchar (f, (long)32, RAW);
					break;
				}
			}
			
			break;
		}
		case TCURSGRATE:
		{
			*((short *)buf) = this->cursrate;
			return this->cursrate;
		}
		default:
		{
			/* if the file is a terminal, Fcntl will automatically
			 * call tty_ioctl for us to handle 'generic' terminal
			 * functions
			 */
			return ENOSYS;
		}
	}
	
	return E_OK;
}

static long _cdecl 
pipe_lseek (FILEPTR *f, long where, int whence)
{
	UNUSED (f);
	UNUSED (where);
	UNUSED (whence);
	
	return ESPIPE;
}

static long _cdecl 
pipe_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	struct fifo *this = (struct fifo *)f->fc.index;
	
	switch (flag)
	{
		case 0:
		{
			*(long *) timeptr = this->mtime.tv_sec;
			break;
		}
		case 1:
		{
			this->mtime.tv_sec = *(long *) timeptr;
			break;
		}
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl 
pipe_close (FILEPTR *f, int pid)
{
	struct fifo *this = (struct fifo *) f->fc.index;
	struct fifo *old;
	struct pipe *p;
	int rwmode;
	FILEPTR **old_x, *x;
	
	if (f->links <= 0)
	{
		/* wake any processes waiting on this pipe */
		
		wake (IO_Q, (long) this->inp);
		if (this->inp->rsel)
			wakeselect ((PROC *) this->inp->rsel);
		if (this->inp->wsel)
			wakeselect ((PROC *) this->inp->wsel);
		
		if (this->outp)
		{
			wake (IO_Q, (long) this->outp);
			if (this->outp->wsel)
				wakeselect ((PROC *) this->outp->wsel);
			if (this->outp->rsel)
				wakeselect ((PROC *) this->outp->rsel);
		}
		
		/* remove the file pointer from the list of open file
		 * pointers of this pipe
		 */
		old_x = &this->open;
		x = this->open;
		while (x && x != f)
		{
		        old_x = &x->next;
		        x = x->next;
		}
# if 1
		/* Temp. workaround for the famous shutdown assert.
		 * Something is screwed somewhere, but this at least
		 * allows to finish shutdown. The alert reminds that
		 * the bug is still to be fixed. BTW it only occurs for
		 * /pipe/q$ttyv0, which is (I guess) an alias for the
		 * console, so the nature of the bug is probably such
		 * as some code somewhere forgets to do f->links++
		 * opening it.
		 *
		 * Notice that returning from this point doesn't really
		 * hurt, except that the memory for struct fifo et
		 * consortes is not freed. But this is shutdown anyways.
		 * The f->links = 0 allows to kfree() the fileptr
		 * flawlessly. See dispose_fileptr().
		 * (draco) 
		 */ 
		if (!x)
		{
			ALERT ("f->links == %d on /pipe/%s ignored",
				f->links,this->name);
			
			f->links = 0;
			return EINTERNAL;
		}
# else
		assert (x);
# endif
		*old_x = f->next;
		/* f->next = 0; */
		
		rwmode = f->flags & O_RWMODE;
		if (rwmode == O_RDONLY || rwmode == O_RDWR)
		{
			p = (f->flags & O_HEAD) ? this->outp : this->inp;
			
			/* note that this can never be a virgin pipe, since we had a handle
			 * on it!
			 */
			if (p)
				p->readers--;
		}
		
		if (rwmode == O_WRONLY || rwmode == O_RDWR)
		{
			p = (f->flags & O_HEAD) ? this->inp : this->outp;
			if (p)
			{
				p->writers--;
# if 1
				/* if pty master exits tell the slaves */
				if (!p->writers && is_terminal(f) &&
				    (f->flags & O_HEAD) && this->tty->pgrp)
					killgroup(this->tty->pgrp, SIGHUP, 1);
# endif
			}
		}
		
		/* correct for the "selfread" flag (see pipe_creat) */
		if ((f->flags & O_HEAD) && !(this->dosflags & 0x02))
			this->inp->readers--;
		
		/* check for locks */
		if ((f->flags & O_LOCK) && (this->lockpid == pid))
		{
			this->flags &= ~O_LOCK;
			/* wake up anyone waiting on the lock */
			wake (IO_Q, (long) this);
		}
	}
	
	/* see if we're finished with the pipe */
	if (this->inp->readers == VIRGIN_PIPE)
		this->inp->readers = 0;
	if (this->inp->writers == VIRGIN_PIPE)
		this->inp->writers = 0;
	
	if (this->inp->readers <= 0 && this->inp->writers <= 0)
	{
		TRACE (("disposing of closed fifo"));
		
		/* unlink from list of FIFOs */
		if (piperoot == this)
			piperoot = this->next;
		else
		{
			for (old = piperoot; old->next != this; old = old->next)
			{
				if (!old)
				{
					ALERT ("fifo not on list???");
					return EINTERNAL;
				}
			}
			
			old->next = this->next;
		}
		
		kfree (this->inp);
		
		if (this->outp)
			kfree (this->outp);
		if (this->tty)
			kfree (this->tty);
		
		kfree (this);
		
		pipestamp = xtime;
	}
	
	return E_OK;
}

static long _cdecl 
pipe_select (FILEPTR *f, long proc, int mode)
{
	struct fifo *this = (struct fifo *) f->fc.index;
	struct pipe *p;
	
	if (mode == O_RDONLY)
	{
		p = (f->flags & O_HEAD) ? this->outp : this->inp;
		if (!p)
		{
			DEBUG(("read select on wrong end of pipe"));
			return 0;
		}
		
		/* NOTE: if p->writers <= 0 then reads won't block
		 * (they'll fail)
		 */
		if ((p->len > 0 &&
			(!is_terminal(f) || ((f->flags & O_HEAD) ?
				(!(this->tty->state & TS_HOLD)) :
				(p->len >= this->tty->vmin*4)))) ||
		    p->writers <= 0)
		{
			return 1;
		}
		
		if (p->rsel)
			return 2;	/* collision */
		
		p->rsel = proc;
		if (is_terminal (f) && !(f->flags & O_HEAD))
			this->tty->rsel = proc;
		
		return 0;
	}
	else if (mode == O_WRONLY)
	{
		p = (f->flags & O_HEAD) ? this->inp : this->outp;
		if (!p)
		{
			DEBUG(("write select on wrong end of pipe"));
			return 0;
		}
		
		if ((p->len < PIPESIZ &&
			(!is_terminal(f) || (f->flags & O_HEAD) ||
			 !(this->tty->state & TS_HOLD))) ||
		    p->readers <= 0)
		{
			return 1;	/* data may be written */
		}
		
		if (p->wsel)
			return 2;	/* collision */
		
		p->wsel = proc;
		if (is_terminal(f) && !(f->flags & O_HEAD))
			this->tty->wsel = proc;
		
		return 0;
	}
	
	return 0;
}

static void _cdecl 
pipe_unselect (FILEPTR *f, long proc, int mode)
{
	struct fifo *this = (struct fifo *)f->fc.index;
	struct pipe *p;
	
	if (mode == O_RDONLY)
	{
		p = (f->flags & O_HEAD) ? this->outp : this->inp;
		if (!p)
			return;
		
		if (p->rsel == proc)
		{
			p->rsel = 0;
			if (is_terminal (f) && !(f->flags & O_HEAD))
				this->tty->rsel = 0;
		}
	}
	else if (mode == O_WRONLY)
	{
		p = (f->flags & O_HEAD) ? this->inp : this->outp;
		if (!p)
			return;
		
		if (p->wsel == proc)
		{
			p->wsel = 0;
			if (is_terminal (f) && !(f->flags & O_HEAD))
				this->tty->wsel = 0;
		}
	}
}
