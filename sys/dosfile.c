/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* DOS file handling routines */

# include "dosfile.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/filedesc.h"

# include "biosfs.h"
# include "filesys.h"
# include "k_fds.h"
# include "k_prot.h"
# include "kmemory.h"
# include "memory.h"
# include "pipefs.h"
# include "proc.h"
# include "procfs.h"
# include "time.h"
# include "timeout.h"
# include "tty.h"
# include "util.h"


/* wait condition for selecting processes which got collisions */
short select_coll;


long _cdecl
f_open (const char *name, short mode)
{
	PROC *p = curproc;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	int global = 0;
	long ret;
	
	TRACE (("Fopen(%s, %x)", name, mode));
	
# if O_GLOBAL
	if (mode & O_GLOBAL)
	{
		if (p->p_cred->ucr->euid)
		{
			DEBUG (("Fopen(%s): O_GLOBAL denied for non root"));
			return EPERM;
		}
		
		/* from now the sockets are clean */
		if (!stricmp (name, "u:\\dev\\socket"))
		{
			ALERT ("O_GLOBAL for sockets denied; update your network tools");
			return EINVAL;
		}
		
		ALERT ("Opening a global handle (%s)", name);
		
		p = rootproc;
		global = 1;
	}
# endif
	
	/* make sure the mode is legal */
	mode &= O_USER;
	
	/* note: file mode 3 is reserved for the kernel;
	 * for users, transmogrify it into O_RDWR (mode 2)
	 */
	if ((mode & O_RWMODE) == O_EXEC)
		mode = (mode & ~O_RWMODE) | O_RDWR;
	
	assert (p->p_fd && p->p_cwd);
	
	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;
	
	ret = do_open (&fp, name, mode, 0, NULL);
	if (ret) goto error;
	
	/* activate the fp, default is to close non-standard files on exec */
	FP_DONE (p, fp, fd, FD_CLOEXEC);
	
# if O_GLOBAL
	if (global)
		/* we just opened a global handle */
		fd += 100;
# endif
	
	TRACE (("Fopen: returning %d", fd));
	return fd;

error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }
	
	return ret;
}

long _cdecl
f_create (const char *name, short attrib)
{
	PROC *p = curproc;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	long ret;
	
	TRACE (("Fcreate(%s, %x)", name, attrib));
	
# if O_GLOBAL
	if (attrib & O_GLOBAL)
	{
		DEBUG (("Fcreate(%s): O_GLOBAL denied"));
		return EPERM;
	}
# endif
	
	assert (p->p_fd && p->p_cwd);
	
	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;
	
	if (attrib == FA_LABEL)
	{
		char temp1[PATH_MAX];
		fcookie dir;
		
		/* just in case the caller tries to do something with this handle,
		 * make it point to u:\dev\null
		 */
		ret = do_open (&fp, "u:\\dev\\null", O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
		if (ret) goto error;
		
		ret = path2cookie (name, temp1, &dir);
		if (ret) goto error;
		
		ret = xfs_writelabel (dir.fs, &dir, temp1);
		release_cookie (&dir);
		if (ret) goto error;
	}
	else if (attrib & (FA_LABEL|FA_DIR))
	{
		DEBUG (("Fcreate(%s,%x): illegal attributes", name, attrib));
		ret = EACCES;
		goto error;
	}
	else
	{
		ret = do_open (&fp, name, O_RDWR|O_CREAT|O_TRUNC, attrib, NULL);
		if (ret)
		{
			DEBUG (("Fcreate(%s) failed, error %d", name, ret));
			goto error;
		}
	}
	
	/* activate the fp, default is to close non-standard files on exec */
	FP_DONE (p, fp, fd, FD_CLOEXEC);
	
	TRACE (("Fcreate: returning %d", fd));
	return fd;
	
error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }
	return ret;
}

long _cdecl
f_close (short fd)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	TRACE (("Fclose: %d", fd));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	r = do_close (p, f);
	
	/* XXX do this before do_close?
	 * remove fd */
	FD_REMOVE (curproc, fd);
	
	/* standard handles should be restored to default values
	 * in TOS domain!
	 * 
	 * XXX: why?
	 */
	if (p->domain == DOM_TOS)
	{
		f = NULL;
		
		if (fd == 0 || fd == 1)
			f = p->p_fd->ofiles[-1];
		else if (fd == 2 || fd == 3)
			f = p->p_fd->ofiles[-fd];
		
		if (f)
		{
			FP_DONE (p, f, fd, 0);
			f->links++;
		}
	}
	
	return r;
}

long _cdecl
f_read (short fd, long count, char *buf)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	if ((f->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG (("Fread: read on a write-only handle"));
		return EACCES;
	}
	
	if (is_terminal (f))
		return tty_read (f, buf, count);
	
	TRACELOW (("Fread: %ld bytes from handle %d to %lx", count, fd, buf));
	return xdd_read (f, buf, count);
}

long _cdecl
f_write (short fd, long count, const char *buf)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Fwrite: write on a read-only handle"));
		return EACCES;
	}
	
	if (is_terminal (f))
		return tty_write (f, buf, count);
	
	/* Prevent broken device drivers from wiping the disk.
	 * We return a zero rather than a negative error code
	 * to help programs those don't handle GEMDOS errors
	 * returned by Fwrite()
	 */
	if (count <= 0)
	{
		DEBUG (("Fwrite: invalid count: %d", count));
		return 0;
	}
	
	/* it would be faster to do this in the device driver, but this
	 * way the drivers are easier to write
	 */
	if (f->flags & O_APPEND)
	{
		r = xdd_lseek (f, 0L, SEEK_END);
		/* ignore errors from unseekable files (e.g. pipes) */
		if (r == EACCES)
			r = 0;
	} else
		r = 0;
	
	if (r >= 0)
	{
		TRACELOW (("Fwrite: %ld bytes to handle %d", count, fd));
		r = xdd_write (f, buf, count);
	}
	
	if (r < 0)
		DEBUG (("Fwrite: error %ld", r));
	
	return r;
}

long _cdecl
f_seek (long place, short fd, short how)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	TRACE (("Fseek(%ld, %d) on handle %d", place, how, fd));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	if (is_terminal (f))
		return 0;
	
	return xdd_lseek (f, place, how);
}

long _cdecl
f_dup (short fd)
{
	long r;
	
	r = do_dup (fd, MIN_OPEN);
	
	TRACE (("Fdup(%d) -> %ld", fd, r));
	return r;
}

long _cdecl
f_force (short newfd, short oldfd)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long ret;
	
	TRACE (("Fforce(%d, %d)", newfd, oldfd));
	
	ret = GETFILEPTR (&p, &oldfd, &fp);
	if (ret) return ret;
	
	if (newfd < MIN_HANDLE || newfd >= p->p_fd->nfiles)
	{
		DEBUG (("Fforce: new handle out of range"));
		return EBADF;
	}
	
	do_close (curproc, curproc->p_fd->ofiles[newfd]);
	curproc->p_fd->ofiles[newfd] = fp;
	
	/* set default file descriptor flags */
	if (newfd >= MIN_OPEN)
		curproc->p_fd->ofileflags[newfd] = FD_CLOEXEC;
	else if (newfd >= 0)
		curproc->p_fd->ofileflags[newfd] = 0;
	
	fp->links++;
	
	/* special: for a tty, if this is becoming a control terminal and the
	 * tty doesn't have a pgrp yet, make it have the pgrp of the process
	 * doing the Fforce
	 */
	if (is_terminal (fp) && newfd == -1 && !(fp->flags & O_HEAD))
	{
		struct tty *tty = (struct tty *) fp->devinfo;
		
		if (!tty->pgrp)
		{
			tty->pgrp = curproc->pgrp;
			
			if (!(fp->flags & O_NDELAY) && (tty->state & TS_BLIND))
				(*fp->dev->ioctl)(fp, TIOCWONLINE, 0);
		}
	}
	
	return E_OK;
}

long _cdecl
f_datime (ushort *timeptr, short fd, short wflag)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	TRACE (("%s(%i)", __FUNCTION__, fd));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	/* some programs use Fdatime to test for TTY devices */
	if (is_terminal (f))
		return EACCES;
	
	if (f->fc.fs && f->fc.fs->fsflags & FS_EXT_3)
	{
		ulong t = 0;
		long r;
		
		if (wflag)
			t = unixtime (timeptr [0], timeptr [1]) + timezone;
		
		r = xdd_datime (f, (ushort *) &t, wflag);
		
		if (!r && !wflag)
			*(long *) timeptr = dostime (t - timezone);
		
		return r;
	}
	
	return xdd_datime (f, timeptr, wflag);
}

long _cdecl
f_lock (short fd, short mode, long start, long length)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	struct flock lock;
	
	TRACE (("Flock(%i, %i, %li, %li)", fd, mode, start, length));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	lock.l_whence = SEEK_SET;
	lock.l_start = start;
	lock.l_len = length;
	
	if (mode == 0)
		/* create a lock */
		lock.l_type = F_WRLCK;
	else if (mode == 1)
		/* unlock region */
		lock.l_type = F_UNLCK;
	else
		return EINVAL;
	
	return xdd_ioctl (f, F_SETLK, &lock);
}

/*
 * extensions to GEMDOS:
 */

/*
 * Fpipe(int *handles): opens a pipe. if successful, returns 0, and
 * sets handles[0] to a file descriptor for the read end of the pipe
 * and handles[1] to one for the write end.
 */

long _cdecl
f_pipe (short *usrh)
{
	PROC *p = curproc;
	FILEPTR *in, *out;
	short infd, outfd;
	long r;
	
	/* MAGIC: 32 >= strlen "u:\pipe\sys$pipe.000\0" */
	char pipename[32];
	
	TRACE (("Fpipe(%lx)", usrh));
	
	r = FP_ALLOC (p, &out);
	if (r) return r;
	
	/* BUG: more than 999 open pipes hangs the system
	 */
	do {
		static int pipeno = 0;
		
		ksprintf (pipename, sizeof (pipename), "u:\\pipe\\sys$pipe.%03d", pipeno);
		
		pipeno++;
		if (pipeno > 999)
			pipeno = 0;
		
		/* read-only attribute means unidirectional fifo
		 * hidden attribute means check for broken pipes
		 * changed attribute means act like Unix fifos
		 */
		r = do_open (&out, pipename, O_WRONLY|O_CREAT|O_EXCL, FA_RDONLY|FA_HIDDEN|FA_CHANGED, NULL);
	}
	while (r != 0 && r == EACCES);
	
	if (r)
	{
		out->links--;
		FP_FREE (out);
		
		DEBUG (("Fpipe: error %d", r));
		return r;
	}
	
	r = FP_ALLOC (p, &in);
	if (r)
	{
		do_close (p, out);
		return r;
	}
	
	r = do_open (&in, pipename, O_RDONLY, 0, NULL);
	if (r)
	{
		do_close (p, out);
		in->links--;
		FP_FREE (in);
		
		DEBUG (("Fpipe: in side of pipe not opened (error %d)", r));
		return r;
	}
	
	r = FD_ALLOC (p, &infd, MIN_OPEN);
	if (r)
	{
		do_close (p, in);
		do_close (p, out);
		
		return r;
	}
	
	r = FD_ALLOC (p, &outfd, infd+1);
	if (r)
	{
		FD_REMOVE (p, infd);
		
		do_close (p, in);
		do_close (p, out);
		
		return r;
	}
	
	/* activate the fps; default is to leave pipes open across Pexec */
	FP_DONE (p, in, infd, 0);
	FP_DONE (p, out, outfd, 0);
	
	usrh[0] = infd;
	usrh[1] = outfd;
	
	TRACE (("Fpipe: returning E_OK: infd %i outfd %i", infd, outfd));
	return E_OK;
}

/*
 * f_cntl: a combination "ioctl" and "fcntl". Some functions are
 * handled here, if they apply to the file descriptors directly
 * (e.g. F_DUPFD) or if they're easily translated into file system
 * functions (e.g. FSTAT). Others are passed on to the device driver
 * via dev->ioctl.
 */

long _cdecl
f_cntl (short fd, long arg, short cmd)
{
	PROC *p = curproc;
	FILEPTR	*f;
	long r;
	
	TRACE (("Fcntl(%i, cmd=0x%x)", fd, cmd));
	
	if (cmd == F_DUPFD)
  		return do_dup (fd, arg);
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	switch (cmd)
	{
		case F_GETFD:
		{
			TRACE (("Fcntl F_GETFD"));
			return p->p_fd->ofileflags[fd];
		}
		case F_SETFD:
		{
			TRACE (("Fcntl F_SETFD"));
			p->p_fd->ofileflags[fd] = arg;
			return E_OK;
		}
		case F_GETFL:
		{
			TRACE (("Fcntl F_GETFL"));
			return (f->flags & O_USER);
		}
		case F_SETFL:
		{
			TRACE (("Fcntl F_SETFL"));
			
			/* make sure only user bits set */
			arg &= O_USER;
			
			/* make sure the file access and sharing modes are not changed */
			arg &= ~(O_RWMODE | O_SHMODE);
			arg |= f->flags & (O_RWMODE | O_SHMODE);
			
			/* set user bits to arg */
			f->flags &= ~O_USER;
			f->flags |= arg;
			
			return E_OK;
		}
		case FSTAT:
		{
			XATTR *xattr = (XATTR *) arg;
			
			if (!f->fc.fs)
				return EINVAL;
			
			r = xfs_getxattr (f->fc.fs, &f->fc, xattr);
			if ((r == E_OK) && (f->fc.fs->fsflags & FS_EXT_3))
			{
				/* UTC -> localtime -> DOS style */
				*((long *) &(xattr->mtime)) = dostime (*((long *) &(xattr->mtime)) - timezone);
				*((long *) &(xattr->atime)) = dostime (*((long *) &(xattr->atime)) - timezone);
				*((long *) &(xattr->ctime)) = dostime (*((long *) &(xattr->ctime)) - timezone);
			}
			
			TRACE (("Fcntl FSTAT (%i, %lx) on \"%s\" -> %li", fd, xattr, xfs_name (&(f->fc)), r));
			return r;
		}
		case FSTAT64:
		{
			STAT *ptr = (STAT *) arg;
			
			if (!f->fc.fs)
				return EINVAL;
			
			if (f->fc.fs->fsflags & FS_EXT_3)
				r = xfs_stat64 (f->fc.fs, &f->fc, ptr);
			else
				r = EINVAL;
			
			TRACE (("Fcntl FSTAT64 (%i, %lx) on \"%s\" -> %li", fd, ptr, xfs_name (&(f->fc)), r));
			return r;
		}
		case FUTIME:
		{
			if (f->fc.fs && (f->fc.fs->fsflags & FS_EXT_3) && arg)
			{
				MUTIMBUF *buf = (MUTIMBUF *) arg;
				ulong t [2];
				
				t [0] = unixtime (buf->actime, buf->acdate) + timezone;
				t [1] = unixtime (buf->modtime, buf->moddate) + timezone;
				
				return xdd_ioctl (f, cmd, (void *) t);
			}
			
			break;
		}
	}
	
	/* fall through to device ioctl */
	
	TRACE (("Fcntl mode %x: calling ioctl", cmd));
	if (is_terminal (f))
	{
		/* tty in the middle of a hangup? */
		while (((struct tty *) f->devinfo)->hup_ospeed)
			sleep (IO_Q, (long) &((struct tty *) f->devinfo)->state);
		
		if (cmd == FIONREAD
			|| cmd == FIONWRITE
			|| cmd == TIOCSTART
			|| cmd == TIOCSTOP
			|| cmd == TIOCSBRK
			|| cmd == TIOCFLUSH)
		{
			r = tty_ioctl (f, cmd, (void *) arg);
		}
		else
		{
			r = (*f->dev->ioctl)(f, cmd, (void *) arg);
			if (r == ENOSYS)
				r = tty_ioctl (f, cmd, (void *) arg);
		}
	}
	else
		r = xdd_ioctl (f, cmd, (void *) arg);
	
	return r;
}

/*
 * Fselect(timeout, rfd, wfd, xfd)
 * timeout is an (unsigned) 16 bit integer giving the maximum number
 * of milliseconds to wait; rfd, wfd, and xfd are pointers to 32 bit
 * integers containing bitmasks that describe which file descriptors
 * we're interested in. These masks are changed to represent which
 * file descriptors actually have data waiting (rfd), are ready to
 * output (wfd), or have exceptional conditions (xfd). If timeout is 0,
 * fselect blocks until some file descriptor is ready; otherwise, it
 * waits only "timeout" milliseconds. Return value: number of file
 * descriptors that are available for reading/writing; or a negative
 * error number.
 */

/* helper function for time outs */
static void _cdecl
unselectme (PROC *p)
{
	wakeselect (p);
}

long _cdecl
f_select (ushort timeout, long *rfdp, long *wfdp, long *xfdp)
{
	long rfd, wfd, xfd, col_rfd, col_wfd, col_xfd;
	long mask, bytes;
	int i, count;
	FILEPTR *f;
	PROC *p;
	TIMEOUT *t;
	int rsel;
	long wait_cond;
	short sr;
#if 0
	long oldgemtimer = 0, gemtimer = 0;
#endif

	if (rfdp) {
		col_rfd = rfd = *rfdp;
	}
	else
		col_rfd = rfd = 0;

	if (wfdp) {
		col_wfd = wfd = *wfdp;
	}
	else
		col_wfd = wfd = 0;
	if (xfdp) {
		col_xfd = xfd = *xfdp;
	} else {
		col_xfd = xfd = 0;
	}

	/* watch out for aliasing */
	if (rfdp) *rfdp = 0;
	if (wfdp) *wfdp = 0;
	if (xfdp) *xfdp = 0;

	t = 0;

	TRACELOW(("Fselect(%u, %lx, %lx, %lx)", timeout, rfd, wfd, xfd));
	p = curproc;			/* help the optimizer out */

	assert (p->p_fd && p->p_cwd);
	
	/* first, validate the masks */
	mask = 1L;
	for (i = 0; i < p->p_fd->nfiles; i++) {
		if ( ((rfd & mask) || (wfd & mask) || (xfd & mask)) && !(p->p_fd->ofiles[i]) ) {
			DEBUG(("Fselect: invalid handle: %d", i));
			return EBADF;
		}
		mask = mask << 1L;
	}

/* now, loop through the file descriptors, setting up the select process */
/* NOTE: wakeselect will set p->wait_cond to 0 if data arrives during the
 * selection
 * Also note: because of the validation above, we may assume that the
 * file handles are valid here. However, this assumption may no longer
 * be true after we've gone to sleep, since a signal handler may have
 * closed one of the handles.
 */

	curproc->wait_cond = (long)wakeselect;		/* flag */

retry_after_collision:
	mask = 1L;
	wait_cond = (long)wakeselect;
	count = 0;
	
	for (i = 0; i < p->p_fd->nfiles; i++) {
		if (col_rfd & mask) {
			f = p->p_fd->ofiles[i];
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_RDONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_RDONLY);
			switch(rsel) {
			case 0:
				col_rfd &= ~mask;
				break;
			case 1:
				count++;
				*rfdp |= mask;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
		if (col_wfd & mask) {
			f = p->p_fd->ofiles[i];
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_WRONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_WRONLY);
			switch(rsel) {
			case 0:
				col_wfd &= ~mask;
				break;
			case 1:
				count++;
				*wfdp |= mask;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
		if (col_xfd & mask) {
			f = p->p_fd->ofiles[i];
/* tesche: anybody worried about using O_RDWR for exceptional data? ;) */
			rsel = (int) (*f->dev->select)(f, (long)p, O_RDWR);
/*  tesche: for old device drivers, which don't understand this
 * call, this will never be true and therefore won't disturb us here.
 */
			switch (rsel) {
			case 0:
				col_xfd &= ~mask;
				break;
			case 1:
				count++;
				*xfdp |= mask;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
		mask = mask << 1L;
	}

#if 0

/* Should we (1999) still keep this code? */

/* GEM kludges part #xxx :(
 *
 * the `last' (1993) GEM AES apparently uses the same etv_timer counter
 * for GEM processes evnt_timer/evnt_multi timeouts and sometimes for
 * some other internal watchdog timer or whatever of always 0x2600 ticks
 * when waking up console/mouse selects, and so sometimes turns your
 * 50ms evnt_timer call into a >3 min one.
 *
 * *sigh* when will atari release source if they don't care about their
 * GEM anymore...  this beast not only needs debugged, it also wants to
 * see a profiler...
 *
 */
	if (!strcmp (curproc->name, "AESSYS")) {
	/* pointer to gems etv_timer handler */
		char *foo = *(char **)(lineA0()-0x42);
		long *bar;
	/* find that counter by looking for the first subql #1,xxxxxx
	 * instruction (0x53b9), save address and old value
	 */
		if (foo && NULL != (foo = memchr (foo, 0x53, 0x40)) &&
		    !(1 & (long)foo) && foo[1] == (char)0xb9 &&
		    foo < (char *)(bar = *(long **)(foo+2))) {
			gemtimer = (long)bar;
			oldgemtimer = *bar;
		}
	}
#endif
	if (count == 0)
	{
		/* no data is ready yet */
		if (timeout && !t)
		{
			t = addtimeout (curproc, (long)timeout, unselectme);
			timeout = 0;
		}
		
		/* curproc->wait_cond changes when data arrives or the timeout happens */
		sr = spl7 ();
		while (curproc->wait_cond == (long)wakeselect)
		{
			curproc->wait_cond = wait_cond;
			spl (sr);
			/*
			 * The 0x100 tells sleep() to return without sleeping
			 * when curproc->wait_cond changes. This way we don't
			 * need spl7 (avoiding endless serial overruns).
			 * Also fixes a deadlock with checkkeys/checkbttys.
			 * They are called from sleep and may wakeselect()
			 * curproc. But sleep used to reset curproc->wait_cond
			 * to wakeselect causing curproc to sleep forever.
			 */
			if (sleep (SELECT_Q|0x100, wait_cond))
				curproc->wait_cond = 0;
			sr = spl7 ();
		}
		if (curproc->wait_cond == (long)&select_coll)
		{
			curproc->wait_cond = (long)wakeselect;
			spl (sr);
			goto retry_after_collision;
		}
		spl (sr);

	/* we can cancel the time out now (if it hasn't already happened) */
		if (t) canceltimeout(t);

	/* OK, let's see what data arrived (if any) */
		mask = 1L;
		for (i = 0; i < p->p_fd->nfiles; i++) {
			if (rfd & mask) {
				f = p->p_fd->ofiles[i];
				if (f) {
				    bytes = 1L;
				    if (is_terminal(f))
					(void)tty_ioctl(f, FIONREAD, &bytes);
				    else
					(void)(*f->dev->ioctl)(f, FIONREAD,&bytes);
				    if (bytes > 0) {
					*rfdp |= mask;
					count++;
				    }
				}
			}
			if (wfd & mask) {
				f = p->p_fd->ofiles[i];
				if (f) {
				    bytes = 1L;
				    if (is_terminal(f))
					(void)tty_ioctl(f, FIONWRITE, &bytes);
				    else
				        (void)(*f->dev->ioctl)(f, FIONWRITE,&bytes);
				    if (bytes > 0) {
					*wfdp |= mask;
					count++;
				    }
				}
			}
			if (xfd & mask) {
				f = p->p_fd->ofiles[i];
				if (f) {
/*  tesche: since old device drivers won't understand this call,
 * we set up `no exceptional condition' as default.
 */
				    bytes = 0L;
				    (void)(*f->dev->ioctl)(f, FIOEXCEPT,&bytes);
				    if (bytes > 0) {
					*xfdp |= mask;
					count++;
				    }
				}
			}
			mask = mask << 1L;
		}
	} else if (t) {
		/* in case data arrived after a collsion, there
		 * could be a timeout pending even if count > 0
		 */
		canceltimeout(t);
	}

	/* at this point, we either have data or a time out */
	/* cancel all the selects */
	mask = 1L;

	for (i = 0; i < p->p_fd->nfiles; i++) {
		if (rfd & mask) {
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDONLY);
		}
		if (wfd & mask) {
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_WRONLY);
		}
		if (xfd & mask) {
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDWR);
		}
		mask = mask << 1L;
	}

#if 0
/* GEM kludgery continued...
 *
 * if the counter was already in use and is somewhere at 3 minutes now
 * then just restore the old value
 */
	if (oldgemtimer) {
		long *bar = (long *)gemtimer;
		short sr = spl7();
		if ((gemtimer = *bar) <= 0x2600 && gemtimer > 0x2000 &&
		    gemtimer > oldgemtimer) {
			*bar = oldgemtimer;
			spl(sr);
			DEBUG(("select: restoring gem timer (%lx: %lx -> %lx)", bar, gemtimer, oldgemtimer));
		}
		spl(sr);
	}
#endif
	/* wake other processes which got a collision */
	if (rfd || wfd || xfd)
		wake(SELECT_Q, (long)&select_coll);

	TRACELOW(("Fselect: returning %d", count));
	return count;
}


/*
 * GEMDOS extension: Fmidipipe
 * Fmidipipe(pid, in, out) manipulates the MIDI file handles (handles -4 and -5)
 * of process "pid" so that they now point to the files with handles "in" and
 * "out" in the calling process
 */
long _cdecl
f_midipipe (short pid, short in, short out)
{
	FILEPTR *fin, *fout;
	PROC *p;
	
	/* first, find the process */
	
	if (pid == 0)
		p = curproc;
	else
	{
		p = pid2proc(pid);
		if (!p)
			return ENOENT;
	}
	
	assert (p->p_fd && p->p_cwd);
	
	/* next, validate the input and output file handles */
	if (in < MIN_HANDLE || in >= p->p_fd->nfiles || (0==(fin = p->p_fd->ofiles[in])))
		return EBADF;
	
	if ((fin->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG(("Fmidipipe: input side is write only"));
		return EACCES;
	}
	
	if (out < MIN_HANDLE || out >= p->p_fd->nfiles || (0==(fout = p->p_fd->ofiles[out])))
		return EBADF;
	
	if ((fout->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG(("Fmidipipe: output side is read only"));
		return EACCES;
	}
	
	/* OK, duplicate the handles and put them in the new process */
	fin->links++;
	fout->links++;
	
	do_close (p, p->p_fd->midiin);
	do_close (p, p->p_fd->midiout);
	
	p->p_fd->midiin = fin;
	p->p_fd->midiout = fout;
	
	return E_OK;
}

/*
 * GEMDOS extension: Ffchown(fh, uid, gid) changes the user and group
 * ownerships of a open file to "uid" and "gid" respectively.
 */

long _cdecl
f_fchown (short fd, short uid, short gid)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	
	TRACE (("Ffchown(%d,%i,%i)", fd, uid, gid));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	if (!(f->fc.fs))
	{
		DEBUG (("Ffchown: not a valid filesystem"));
		return ENOSYS;
	}
	
	/* MiNT acts like _POSIX_CHOWN_RESTRICTED: a non-privileged process
	 * can only change the ownership of a file that is owned by this
	 * user, to the effective group id of the process or one of its
	 * supplementary groups
	 */
	if (p->p_cred->ucr->euid)
	{
		XATTR xattr;
		
		if (p->p_cred->ucr->egid != gid && !groupmember (p->p_cred->ucr, gid))
			r = EACCES;
		else
			r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
		
		if (r)
		{
			DEBUG (("Ffchown(%i): unable to get file attributes", fd));
			return r;
		}
		
		if (xattr.uid != curproc->p_cred->ucr->euid || xattr.uid != uid)
		{
			DEBUG (("Ffchown(%i): not the file's owner", fd));
			return EACCES;
		}
		
		r = xfs_chown (f->fc.fs, &(f->fc), uid, gid);
		
		/* POSIX 5.6.5.2: if name refers to a regular file the
		 * set-user-ID and set-group-ID bits of the file mode shall
		 * be cleared upon successful return from the call to chown,
		 * unless the call is made by a process with the appropriate
		 * privileges. Note that POSIX leaves the behaviour
		 * unspecified for all other file types. At least for
		 * directories with BSD-like setgid semantics, these bits
		 * should be left unchanged.
		 */
		if (!r && (xattr.mode & S_IFMT) != S_IFDIR
			&& (xattr.mode & (S_ISUID | S_ISGID)))
		{
			long s;
			
			s = xfs_chmode (f->fc.fs, &(f->fc), xattr.mode & ~(S_ISUID | S_ISGID));
			if (!s)
				DEBUG (("Ffchown: chmode returned %ld (ignored)", s));
		}
	}
	else
		r = xfs_chown (f->fc.fs, &(f->fc), uid, gid);
	
	return r;
}

/*
 * GEMDOS extension: Fchmod (fh, mode) changes a file's access
 * permissions on a open file.
 */

long _cdecl
f_fchmod (short fd, ushort mode)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;
	XATTR xattr;
	
	TRACE (("Ffchmod(%i, %i)", fd, mode));
	
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;
	
	if (!(f->fc.fs))
	{
		DEBUG (("Ffchmod: not a valid filesystem"));
		return ENOSYS;
	}
	
	r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
	if (r)
	{
		DEBUG (("Ffchmod(%i): couldn't get file attributes", fd));
	}
	else if (p->p_cred->ucr->euid && p->p_cred->ucr->euid != xattr.uid)
	{
		DEBUG (("Ffchmod(%i): not the file's owner", fd));
		r = EACCES;
	}
	else
	{
		r = xfs_chmode (f->fc.fs, &(f->fc), mode & ~S_IFMT);
		if (r)
			DEBUG (("Ffchmod: error %ld", r));
	}
	
	return r;
}

/*
 * GEMDOS extension: Fseek64 (place, fh, how, newpos)
 * 
 * - 64bit clean seek system call
 * - newpos is written only if return value is 0 (no failure)
 * - at the moment only a wrapper around Fseek() as there is no
 *   64bit xfs/xdd support
 */

long _cdecl
f_seek64 (llong place, short fd, short how, llong *newpos)
{
	long r;
	
# define LONG_MAX	2147483647L
	if (place > LONG_MAX)
		return EBADARG;
	
	r = f_seek ((long) place, fd, how);
	if (r >= 0)
		*newpos = (llong) r;
	
	return r;
}

/*
 * GEMDOS extension: Fpoll (fds, nfds, timeout)
 * 
 * - new Fselect() call for more than 32 filedeskriptors
 * - at the moment only a wrapper around Fselect()
 */

long _cdecl
f_poll (POLLFD *fds, ulong nfds, ulong timeout)
{
	ulong rfds = 0;
	ulong wfds = 0;
	ulong xfds = 0;
	long retval;
	register long i;
	
	for (i = 0; i < nfds; i++)
	{
		if (fds[i].fd > 31)
			return EINVAL;
		
# define LEGAL_FLAGS \
	(POLLIN | POLLPRI | POLLOUT | POLLERR | POLLHUP | POLLNVAL | POLLRDNORM | POLLWRNORM)
		
		if ((fds[i].events | LEGAL_FLAGS) != LEGAL_FLAGS)
			return EINVAL;
		
		if (fds[i].events & POLLIN)
			rfds |= (1L << (fds[i].fd));
		if (fds[i].events & POLLPRI)
			xfds |= (1L << (fds[i].fd));
		if (fds[i].events & POLLOUT)
			wfds |= (1L << (fds[i].fd));
	}
	
	if (timeout == ~0)
		retval = f_select (0L, &rfds, &wfds, &xfds);
	else if (timeout == 0)
		retval = f_select (1L, &rfds, &wfds, &xfds);
# define USHRT_MAX	65535
	else if (timeout < USHRT_MAX)
		retval = f_select (timeout, &rfds, &wfds, &xfds);
	else
	{
		ulong saved_rfds, saved_wfds, saved_xfds;
		ushort this_timeout;
		int last_round = 0;
		
		saved_rfds = rfds;
		saved_wfds = wfds;
		saved_xfds = xfds;
		
		do {
			if ((ulong) timeout > USHRT_MAX)
				this_timeout = USHRT_MAX;
			else
			{
				this_timeout = timeout;
				last_round = 1;
			}
			
			retval = f_select (this_timeout, &rfds, &wfds, &xfds);
			if (retval != 0)
				break;
			
			timeout -= this_timeout;
			
			rfds = saved_rfds;
			wfds = saved_wfds;
			xfds = saved_xfds;
		}
		while (!last_round);
	}
	
	if (retval < 0)
		return retval;
	
	for (i = 0; i < nfds; i++)
	{
		if (rfds & (1L << (fds[i].fd)))
			fds[i].revents = POLLIN;
		else
			fds[i].revents = 0;
		
		if (xfds & (1L << (fds[i].fd)))
			fds[i].revents |= POLLPRI;
		
		if (wfds & (1L << (fds[i].fd)))
			fds[i].revents |= POLLOUT;
	}
	
	return retval;
}
