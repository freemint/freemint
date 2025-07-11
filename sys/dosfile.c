/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/* DOS file handling routines */

# include "dosfile.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/iov.h"
# include "mint/net.h"

# include "biosfs.h"
# include "filesys.h"
# include "info.h"
# include "ipc_socketdev.h"
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
sys_f_open (const char *name, short mode)
{
	struct proc *p = get_curproc();
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	int globl = 0;
	long ret;

	TRACE (("Fopen(%s, %x)", name, mode));

# if O_GLOBAL
	if (mode & O_GLOBAL)
	{
		TRACE (("O_GLOBAL Fopen(%s, %x)", name, mode));

		if (stricmp (name, "u:\\dev\\console") == 0 || stricmp (name, "u:\\pipe\\sld") == 0)
		{
			if (p->p_cred->ucr->euid)
			{
				DEBUG (("Fopen(%s): O_GLOBAL denied for non root", name));
				return EPERM;
			}

			p = rootproc;
			globl = 1;
		}
		else
		{
			DEBUG (("Fopen(%s): O_GLOBAL denied", name));
			return EPERM;
		}
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
	if (globl)
		/* we just opened a global handle */
		fd |= 0x8000;
# endif

	TRACE (("Fopen: returning %04x", fd));
	return (long)fd & 0x0000ffffL;

error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }

	return ret;
}

long _cdecl
sys_f_create (const char *name, short attrib)
{
	struct proc *p = get_curproc();
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	long ret;

	TRACE (("Fcreate(%s, %x)", name, attrib));

# if O_GLOBAL
	if (attrib & O_GLOBAL)
	{
		DEBUG (("Fcreate(%s): O_GLOBAL denied", name));
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

		ret = path2cookie (p, name, temp1, &dir);
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
			DEBUG (("Fcreate(%s) failed, error %ld", name, ret));
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
sys_f_close (short fd)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	DIR **where, **_where;
	long r;

	DEBUG (("Fclose: %d", fd));

# ifdef WITH_SINGLE_TASK_SUPPORT
	/* this is for pure-debugger:
	 * some progs call Fclose(-1) when they exit which would
	 * cause pd to lose keyboard
	 */
	if( fd < 0 && (p->modeflags & M_SINGLE_TASK) )
	{
		DEBUG(("Fclose:return 0 for negative fd in singletask-mode."));
		return 0;
	}
# endif
	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	/* close dirent's for the same FD */
	where = &p->p_fd->searches;
	while (*where) {
		_where = &((*where)->next);
		if (fd >= MIN_OPEN && (*where)->fd == fd) {

			r = xfs_closedir ((*where)->fc.fs, (*where));
			release_cookie (&(*where)->fc);

			kfree(*where);

			/* unlink the directory from the chain */
			*where = *_where;
		}
		where = _where;
	}

	r = do_close (p, f);

	/* XXX do this before do_close? */
	FD_REMOVE (p, fd);

# if 0
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
# endif

	return r;
}

long _cdecl
sys_f_read (short fd, long count, void *buf)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG (("Fread: read on a write-only handle"));
		return EACCES;
	}

	if (f->flags & O_DIRECTORY)
	{
		DEBUG (("Fread(%i): read on a directory", fd));
		return EISDIR;
	}

	if (is_terminal (f))
		return tty_read (f, buf, count);

	TRACELOW (("Fread: %ld bytes from handle %d to %p", count, fd, buf));
	return xdd_read (f, buf, count);
}

long _cdecl
sys_f_write (short fd, long count, const void *buf)
{
	struct proc *p = get_curproc();
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
		DEBUG (("Fwrite: invalid count: %ld", count));
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
sys_f_seek (long place, short fd, short how)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("Fseek(%ld, %d) on handle %d", place, how, fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (is_terminal (f))
		return ESPIPE;

	return xdd_lseek (f, place, how);
}

long _cdecl
sys_f_dup (short fd)
{
	long r;

	r = do_dup (fd, 0, 0);

	TRACE (("Fdup(%d) -> %ld", fd, r));
	return r;
}

long _cdecl
sys_f_force (short newfd, short oldfd)
{
	struct proc *p = get_curproc();
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

	do_close (p, p->p_fd->ofiles[newfd]);

	p->p_fd->ofiles[newfd] = fp;
	p->p_fd->ofileflags[newfd] = 0;

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
			tty->pgrp = p->pgrp;
			DEBUG (("f_force: assigned tty->pgrp = %i", tty->pgrp));

			if (!(fp->flags & O_NDELAY) && (tty->state & TS_BLIND))
				(*fp->dev->ioctl)(fp, TIOCWONLINE, 0);
		}
	}

	return E_OK;
}

long _cdecl
sys_f_datime (ushort *timeptr, short fd, short wflag)
{
	struct proc *p = get_curproc();
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
		unsigned long ut = 0;
		if (wflag)
			ut = unixtime(timeptr[0], timeptr[1]) + timezone;
		r = xdd_datime(f, (ushort *)&ut, wflag);
		if (!r && !wflag) {
			ut = dostime(ut - timezone);
			timeptr[1] = (unsigned short)ut;
			ut >>= 16;
			timeptr[0] = (unsigned short)ut;
		}
		return r;
	}

	return xdd_datime (f, timeptr, wflag);
}

long _cdecl
sys_f_lock (short fd, short mode, long start, long length)
{
	struct proc *p = get_curproc();
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

static long
sys__ffstat_1_12 (struct file *f, XATTR *xattr)
{
	long ret;

# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
		return so_fstat_old (f, xattr);

	if (!f->fc.fs)
	{
		DEBUG (("sys__ffstat_1_12: no xfs!"));
		return ENOSYS;
	}

	ret = xdd_ioctl(f, FSTAT, xattr);
	if (ret == ENOSYS)
		ret = xfs_getxattr (f->fc.fs, &f->fc, xattr);
	if ((ret == E_OK) && (f->fc.fs->fsflags & FS_EXT_3))
	{
		xtime_to_local_dos(xattr,m);
		xtime_to_local_dos(xattr,a);
		xtime_to_local_dos(xattr,c);
	}

	return ret;
}

static long
sys__ffstat_1_16 (struct file *f, struct stat *st)
{
	long ret;

# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
		return so_fstat (f, st);

	if (!f->fc.fs)
	{
		DEBUG (("sys__ffstat_1_16: no xfs"));
		return ENOSYS;
	}

	ret = xdd_ioctl(f, FSTAT64, st);
	if (ret == ENOSYS)
		ret = xfs_stat64 (f->fc.fs, &f->fc, st);
	return ret;
}

long _cdecl
sys_ffstat (short fd, struct stat *st)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	long ret;

	ret = GETFILEPTR (&p, &fd, &f);
	if (ret) return ret;

	return sys__ffstat_1_16 (f, st);
}

/*
 * f_cntl: a combination "ioctl" and "fcntl". Some functions are
 * handled here, if they apply to the file descriptors directly
 * (e.g. F_DUPFD) or if they're easily translated into file system
 * functions (e.g. FSTAT). Others are passed on to the device driver
 * via dev->ioctl.
 */

long _cdecl
sys_f_cntl (short fd, long arg, short cmd)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	long r;

	TRACE (("Fcntl(%i, cmd=0x%x)", fd, cmd));

	if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC) {
  		return do_dup (fd, arg, cmd == F_DUPFD_CLOEXEC ? 1: 0);
	}

	TRACE(("Fcntl getfileptr"));
	r = GETFILEPTR (&p, &fd, &f);
	TRACE(("Fcntl r = %lx", r));
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
			TRACE (("Fcntl FSTAT (%i, %lx) on \"%s\" -> %li", fd, arg, xfs_name (&(f->fc)), r));
			return sys__ffstat_1_12 (f, (XATTR *) arg);
		}
		case FSTAT64:
		{
			TRACE (("Fcntl FSTAT64"));
			TRACE (("Fcntl FSTAT64 (%i, %lx) on \"%s\" -> %li", fd, arg, xfs_name(&(f->fc)), r));
			return sys__ffstat_1_16 (f, (struct stat *) arg);
		}
		case FUTIME:
		{
			TRACE (("Fcntl FUTIME"));
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
unselectme (struct proc *p, long arg)
{
	wakeselect (p);
}

long _cdecl
sys_f_select (ushort timeout, long *rfdp, long *wfdp, long *xfdp)
{
	long rfd, wfd, xfd, col_rfd, col_wfd, col_xfd;
	long mask;
	long count;
	FILEPTR *f;
	struct proc *p;
	TIMEOUT *t = NULL;
	int i, rsel;
	long wait_cond;
	short sr;
#if 0
	long oldgemtimer = 0, gemtimer = 0;
#endif

	if (rfdp) col_rfd = rfd = *rfdp;
	else      col_rfd = rfd = 0;

	if (wfdp) col_wfd = wfd = *wfdp;
	else      col_wfd = wfd = 0;

	if (xfdp) col_xfd = xfd = *xfdp;
	else      col_xfd = xfd = 0;

	/* watch out for aliasing */
	if (rfdp) *rfdp = 0;
	if (wfdp) *wfdp = 0;
	if (xfdp) *xfdp = 0;

	TRACELOW(("Fselect(%u, %lx, %lx, %lx)", timeout, rfd, wfd, xfd));
	p = get_curproc();			/* help the optimizer out */

	assert (p->p_fd && p->p_cwd);

	/* first, validate the masks */
	mask = 1L;
	for (i = 0; i < p->p_fd->nfiles; i++)
	{
		if (((rfd & mask) || (wfd & mask) || (xfd & mask)) && !(p->p_fd->ofiles[i]))
		{
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

	p->wait_cond = (long)wakeselect;		/* flag */

retry_after_collision:
	mask = 1L;
	wait_cond = (long)wakeselect;
	count = 0;

	for (i = 0; i < p->p_fd->nfiles; i++)
	{
		if (col_rfd & mask)
		{
			f = p->p_fd->ofiles[i];
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_RDONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_RDONLY);
			switch (rsel)
			{
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
		if (col_wfd & mask)
		{
			f = p->p_fd->ofiles[i];
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_WRONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_WRONLY);
			switch (rsel)
			{
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
		if (col_xfd & mask)
		{
			f = p->p_fd->ofiles[i];
/* tesche: anybody worried about using O_RDWR for exceptional data? ;) */
			rsel = (int) (*f->dev->select)(f, (long)p, O_RDWR);
/*  tesche: for old device drivers, which don't understand this
 * call, this will never be true and therefore won't disturb us here.
 */
			switch (rsel)
			{
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

	if (count == 0)
	{
		/* no data is ready yet */
		if (timeout && !t)
		{
			t = addtimeout (p, (long)timeout, unselectme);
			timeout = 0;
		}

		/* curproc->wait_cond changes when data arrives or the timeout happens */
		sr = spl7 ();
		while (p->wait_cond == (long)wakeselect)
		{
			p->wait_cond = wait_cond;
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
			if (sleep (SELECT_Q|0x100, wait_cond)) {
				/* signal happened, abort */
				if (t) canceltimeout(t);

				count = EINTR;

				goto cancel;
			}
			sr = spl7 ();
		}
		if (p->wait_cond == (long)&select_coll)
		{
			p->wait_cond = (long)wakeselect;
			spl (sr);
			goto retry_after_collision;
		}
		spl (sr);

	/* we can cancel the time out now (if it hasn't already happened) */
		if (t) canceltimeout(t);

	/* OK, let's see what data arrived (if any) */
		mask = 1L;
		for (i = 0; i < p->p_fd->nfiles; i++)
		{
			if (rfd & mask)
			{
				f = p->p_fd->ofiles[i];
				if (f)
				{
				    if (is_terminal(f))
					rsel = (int) tty_select(f, (long)p, O_RDONLY);
				    else
					rsel = (int) (*f->dev->select)(f, (long)p, O_RDONLY);
				    if (rsel == 1)
				    {
					*rfdp |= mask;
					count++;
				    }
				}
			}
			if (wfd & mask)
			{
				f = p->p_fd->ofiles[i];
				if (f)
				{
				    if (is_terminal(f))
					rsel = (int) tty_select(f, (long)p, O_WRONLY);
				    else
					rsel = (int) (*f->dev->select)(f, (long)p, O_WRONLY);
				    if (rsel == 1)
				    {
					*wfdp |= mask;
					count++;
				    }
				}
			}
			if (xfd & mask)
			{
				f = p->p_fd->ofiles[i];
				if (f)
				{
/*  tesche: since old device drivers won't understand this call,
 * we set up `no exceptional condition' as default.
 */
				    rsel = (int) (*f->dev->select)(f, (long)p, O_RDWR);
				    if (rsel == 1)
				    {
					*xfdp |= mask;
					count++;
				    }
				}
			}
			mask = mask << 1L;
		}
	}
	else if (t)
	{
		/* in case data arrived after a collsion, there
		 * could be a timeout pending even if count > 0
		 */
		canceltimeout(t);
	}

cancel:
	/* at this point, we either have data or a time out */
	/* cancel all the selects */
	mask = 1L;

	for (i = 0; i < p->p_fd->nfiles; i++)
	{
		if (rfd & mask)
		{
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDONLY);
		}
		if (wfd & mask)
		{
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_WRONLY);
		}
		if (xfd & mask)
		{
			f = p->p_fd->ofiles[i];
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDWR);
		}
		mask = mask << 1L;
	}

	/* wake other processes which got a collision */
	if (rfd || wfd || xfd)
		wake(SELECT_Q, (long)&select_coll);

	TRACELOW(("Fselect: returning %ld", count));
	return count;
}


/*
 * GEMDOS extension: Fmidipipe
 * Fmidipipe(pid, in, out) manipulates the MIDI file handles (handles -4 and -5)
 * of process "pid" so that they now point to the files with handles "in" and
 * "out" in the calling process
 */
long _cdecl
sys_f_midipipe (short pid, short in, short out)
{
	FILEPTR *fin, *fout;
	struct proc *p;

	/* first, find the process */

	if (pid == 0)
		p = get_curproc();
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
sys_f_fchown (short fd, short uid, short gid)
{
	struct proc *p = get_curproc();
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

	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Ffchown: write on a read-only handle"));
		return EPERM;
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

		if (xattr.uid != p->p_cred->ucr->euid || xattr.uid != uid)
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
		if (!r && !S_ISDIR(xattr.mode)
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
sys_f_fchmod (short fd, ushort mode)
{
	struct proc *p = get_curproc();
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
sys_f_seek64 (llong place, short fd, short how, llong *newpos)
{
	long r;

# define LONG_MAX	2147483647L
	if (place > LONG_MAX)
		return EBADARG;

	r = sys_f_seek ((long) place, fd, how);
	if (r >= 0)
		*newpos = (llong) r;

	return r;
}

/*
 * GEMDOS extension: Fpoll (fds, nfds, timeout)
 *
 * - new Fselect() call for more than 32 filedescriptors
 * - at the moment only a wrapper around Fselect()
 */

long _cdecl
sys_f_poll (POLLFD *fds, ulong nfds, ulong timeout)
{
	struct proc *p = get_curproc();
	register long i;
	long count = 0;
	long wait_cond;
	FILEPTR *f;
	TIMEOUT *t = NULL;
	int rsel;
	short sr;

	/* validate */
	for (i = 0; i < nfds; i++)
	{
# define LEGAL_INPUT_FLAGS \
	(POLLIN | POLLPRI | POLLOUT | POLLRDNORM | POLLWRNORM | POLLRDBAND | POLLWRBAND)
		if (((fds[i].events | LEGAL_INPUT_FLAGS) != LEGAL_INPUT_FLAGS) ||
		   fds[i].fd >= NDFILE || 
		   !p->p_fd->ofiles[fds[i].fd]) {
			fds[i].revents |= POLLNVAL;
		} else {
			fds[i].revents = fds[i].events;
		}
	}

        assert (p->p_fd && p->p_cwd);

retry_after_collision:
	p->wait_cond = (long)wakeselect;                /* flag */
	wait_cond = (long)wakeselect;
	count = 0;

	for (i = 0; i < nfds; i++)
	{
		if (fds[i].revents & POLLNVAL)
			continue;

		f = p->p_fd->ofiles[fds[i].fd];

		if (fds[i].revents & (POLLIN | POLLRDNORM))
		{
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_RDONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_RDONLY);
			switch (rsel)
			{
			case 0:
				fds[i].revents &= ~(fds[i].events & (POLLIN | POLLRDNORM));
				break;
			case 1:
				fds[i].revents |= (fds[i].events & (POLLIN | POLLRDNORM));
				count++;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
		if (fds[i].revents & (POLLOUT | POLLWRNORM))
		{
			if (is_terminal(f))
				rsel = (int) tty_select(f, (long)p, O_WRONLY);
			else
				rsel = (int) (*f->dev->select)(f, (long)p, O_WRONLY);
			switch (rsel)
			{
			case 0:
				fds[i].revents &= ~(fds[i].events & (POLLOUT | POLLWRNORM));
				break;
			case 1:
				fds[i].revents |= (fds[i].events & (POLLOUT | POLLWRNORM));
				count++;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
		if (fds[i].revents & POLLPRI)
		{
/* tesche: anybody worried about using O_RDWR for exceptional data? ;) */
			rsel = (int) (*f->dev->select)(f, (long)p, O_RDWR);
/*  tesche: for old device drivers, which don't understand this
 * call, this will never be true and therefore won't disturb us here.
 */
			switch (rsel)
			{
			case 0:
				fds[i].revents &= ~(fds[i].events & POLLPRI);
				break;
			case 1:
				fds[i].revents |= (fds[i].events & POLLPRI);
				count++;
				break;
			case 2:
				wait_cond = (long)&select_coll;
				break;
			}
		}
	}

	/* reset timeout for our select call */
	if (timeout == ~0)
		timeout = 0;
	else if (timeout == 0)
		goto cancel;

	if (count == 0)
	{
		/* no data is ready yet */
		if (timeout && !t)
		{
			t = addtimeout (p, (long)timeout, unselectme);
			timeout = 0;
		}

		/* curproc->wait_cond changes when data arrives or the timeout happens */
		sr = spl7 ();
		while (p->wait_cond == (long)wakeselect)
		{
			p->wait_cond = wait_cond;
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
			if (sleep (SELECT_Q|0x100, wait_cond)) {
				/* signal happened, abort */
				if (t) canceltimeout(t);

				count = EINTR;

				goto cancel;
			}
			sr = spl7 ();
		}
		if (p->wait_cond == (long)&select_coll)
		{
			spl (sr);
			goto retry_after_collision;
		}
		spl (sr);

	/* we can cancel the time out now (if it hasn't already happened) */
		if (t) canceltimeout(t);

	/* OK, let's see what data arrived (if any) */
		for (i = 0; i < nfds; i++)
		{
			ushort events = fds[i].events;
	
			if (fds[i].revents & POLLNVAL)
				continue;

			f = p->p_fd->ofiles[fds[i].fd];
			
			if (events & (POLLIN | POLLRDNORM))
			{
				if (f)
				{
				    if (is_terminal(f))
					rsel = (int) tty_select(f, (long)p, O_RDONLY);
				    else
					rsel = (int) (*f->dev->select)(f, (long)p, O_RDONLY);

				    if (rsel == 1) {
					fds[i].revents |= (fds[i].events & (POLLIN | POLLRDNORM));
					count++;
				    }
				}
			}
			if (events & (POLLOUT | POLLWRNORM))
			{
				if (f)
				{
				    if (is_terminal(f))
					rsel = (int) tty_select(f, (long)p, O_WRONLY);
				    else
					rsel = (int) (*f->dev->select)(f, (long)p, O_WRONLY);

				    if (rsel == 1) {
					fds[i].revents |= (fds[i].events & (POLLOUT | POLLWRNORM));
					count++;
				    }
				}
			}
			if (events & POLLPRI)
			{
				if (f)
				{
/*  tesche: since old device drivers won't understand this call,
 * we set up `no exceptional condition' as default.
 */
				    rsel = (int) (*f->dev->select)(f, (long)p, O_RDWR);
				    if (rsel == 1) {
					fds[i].revents |= (fds[i].events & POLLPRI);
					count++;
				    }
				}
			}
		}
	}
	else if (t)
	{
		/* in case data arrived after a collsion, there
		 * could be a timeout pending even if count > 0
		 */
		canceltimeout(t);
	}

cancel:
	/* at this point, we either have data or a time out */
	/* cancel all the selects */

	for (i = 0; i < nfds; i++)
	{
#if 0
		if (count == EINTR) {
			fds[i].revents |= POLLHUP;
			continue;
		}
#endif
		if (fds[i].revents & POLLNVAL) {
			count++; 
			continue;
		}

		f = p->p_fd->ofiles[fds[i].fd];

		if (fds[i].events & (POLLIN | POLLRDNORM))
		{
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDONLY);
		}
		if (fds[i].events & (POLLOUT | POLLWRNORM))
		{
			if (f)
				(*f->dev->unselect)(f, (long)p, O_WRONLY);
		}
		if (fds[i].events & POLLPRI)
		{
			if (f)
				(*f->dev->unselect)(f, (long)p, O_RDWR);
		}
	}

	/* wake other processes which got a collision */
	wake(SELECT_Q, (long)&select_coll);

	return count;
}

long _cdecl
sys_fwritev (short fd, const struct iovec *iov, long niov)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("Fwritev(%i, %p, %li)", fd, iov, niov));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Fwritev: write on a read-only handle"));
		return EACCES;
	}

# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
	{
		struct socket *so = (struct socket *) f->devinfo;

		return (*so->ops->send)(so, iov, niov, f->flags & O_NDELAY, 0, 0, 0);
	}

	{
		char *ptr, *_ptr;
		long size;
		int i;

		size = iov_size (iov, niov);
		if (size < 0)
			return EINVAL;

		/* if (size == 0)
			return 0; */

		ptr = _ptr = kmalloc (size);
		if (!ptr) return ENOMEM;

		for (i = 0; i < niov; ++i)
		{
			memcpy (ptr, iov[i].iov_base, iov[i].iov_len);
			ptr += iov[i].iov_len;
		}

		if (is_terminal (f))
			r = tty_write (f, _ptr, size);
		else
		{
			if (f->flags & O_APPEND)
				xdd_lseek (f, 0L, SEEK_END);

			r = xdd_write (f, _ptr, size);
		}

		kfree (_ptr);
		return r;
	}
}

long _cdecl
sys_freadv (short fd, const struct iovec *iov, long niov)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("Freadv(%i, %p, %li)", fd, iov, niov));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG (("Freadv: read on a write-only handle"));
		return EACCES;
	}

# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
	{
		struct socket *so = (struct socket *) f->devinfo;

		return (*so->ops->recv)(so, iov, niov, f->flags & O_NDELAY, 0, 0, 0);
	}

	{
		char *ptr, *_ptr;
		long size;
		int i;

		size = iov_size (iov, niov);
		if (size < 0)
			return EINVAL;

		/* if (size == 0)
			return 0; */

		ptr = _ptr = kmalloc (size);
		if (!ptr) return ENOMEM;

		if (is_terminal (f))
			r = tty_read (f, ptr, size);
		else
			r = xdd_read (f, ptr, size);

		if (r <= 0)
		{
			kfree (ptr);
			return r;
		}

		for (i = 0, size = r; size > 0; ++i)
		{
			register long copy;

			copy = size > iov[i].iov_len ? iov[i].iov_len : size;
			memcpy (iov[i].iov_base, ptr, copy);

			ptr += copy;
			size -= copy;
		}

		kfree (_ptr);
		return r;
	}
}
