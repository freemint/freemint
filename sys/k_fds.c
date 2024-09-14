/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-01-12
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "k_fds.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/credentials.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"

# include "biosfs.h"
# include "dosfile.h"
# include "filesys.h"
# include "k_prot.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "pipefs.h"
# include "proc.h"
# include "procfs.h"
# include "timeout.h"
# include "tty.h"


long
fd_alloc (struct proc *p, short *fd, short min, const char *func)
{
	short i;

	assert (p->p_fd);

	for (i = min; i < p->p_fd->nfiles; i++)
	{
		if (!p->p_fd->ofiles[i])
		{
			/* reserve the handle */
			p->p_fd->ofiles[i] = (FILEPTR *) 1;

			*fd = i;

			TRACE (("%s: fd_alloc -> %i", func, i));
			return 0;
		}
	}

	DEBUG (("%s: process out of handles", func));
# ifdef DEBUG_INFO
	for (i = min; i < p->p_fd->nfiles; i++)
	{
		FILEPTR *f = p->p_fd->ofiles[i];

		if (f && (f != (FILEPTR *) 1))
			DEBUG (("%i -> %p, links %i, flags %x, dev = %p", i, f, f->links, f->flags, f->dev));
		else
			DEBUG (("%i -> %p", i, f));
	}
# endif
	return EMFILE;
}

void
fd_remove (struct proc *p, short fd, const char *func)
{
	assert (p->p_fd);

	p->p_fd->ofiles[fd] = NULL;
}


long
fp_alloc (struct proc *p, FILEPTR **resultfp, const char *func)
{
	FILEPTR *fp;

	fp = kmalloc (sizeof (*fp));
	if (!fp)
	{
		DEBUG (("%s: out of memory for FP_ALLOC", func));
		return ENOMEM;
	}

	mint_bzero (fp, sizeof (*fp));

	fp->links = 1;
	// later
	// fp->cred = p->p_cred;
	// hold_cred (fp->cred);

	*resultfp = fp;

	TRACE (("%s: fp_alloc: kmalloc %p", func, fp));
	return 0;
}

void
fp_done (struct proc *p, FILEPTR *fp, short fd, char fdflags, const char *func)
{
	assert (p->p_fd);
	assert (p->p_fd->ofiles[fd] == (FILEPTR *) 1);

	p->p_fd->ofiles[fd] = fp;
	p->p_fd->ofileflags[fd] = fdflags;
}

void
fp_free (FILEPTR *fp, const char *func)
{
	if (fp->links != 0)
	{
		// FATAL ("dispose_fileptr: fp->links == %d", fp->links);
		ALERT ("%s: fp->links == %i", func, fp->links);
	}

	// later
	// free_cred (fp->cred);

	TRACE (("%s: fp_free: kfree %p", func, fp));
	kfree (fp);
}

long
fp_get (struct proc **p, short *fd, FILEPTR **fp, const char *func)
{
# if O_GLOBAL
	if (*fd & 0x8000)
	{
		*fd &= ~0x8000;
		*p = rootproc;
	}
# endif

	assert ((*p));

	if ((*fd < MIN_HANDLE)
		|| !(*p)->p_fd
	    || (*fd >= (*p)->p_fd->nfiles)
	    || !(*fp = (*p)->p_fd->ofiles[*fd])
	    || (*fp == (FILEPTR *) 1))
	{
		DEBUG (("%s(): invalid fd handle (%i)!", func, *fd));
		return EBADF;
	}

	return 0;
}

long
fp_get1 (struct proc *p, short fd, FILEPTR **fp, const char *func)
{
	assert (p);

	if ((fd < MIN_HANDLE)
			|| !p->p_fd
	    || (fd >= p->p_fd->nfiles)
	    || !(*fp = p->p_fd->ofiles[fd])
	    || (*fp == (FILEPTR *) 1))
	{
		DEBUG (("%s(): invalid fd handle (%i)!", func, fd));
		return EBADF;
	}

	return 0;
}

/* duplicate file pointer fd;
 * returns a new file pointer >= min, if one exists, or EMFILE if not.
 * called by f_dup and f_cntl
 */

long
do_dup (short fd, short min, int cmd)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	short newfd;
	long ret;

	assert (p->p_fd);

	ret = FD_ALLOC (get_curproc(), &newfd, min);
	if (ret) return ret;

	ret = GETFILEPTR (&p, &fd, &fp);
	if (ret)
	{
		FD_REMOVE (get_curproc(), newfd);
		return ret;
	}

	FP_DONE (get_curproc(), fp, newfd, cmd ? FD_CLOEXEC : 0);

	fp->links++;
	return newfd;
}

/* do_open(f, name, rwmode, attr, x)
 *
 * f      - pointer to FILEPTR *
 * name   - file name
 * rwmode - file access mode
 * attr   - TOS attributes for created files (if applicable)
 * x      - filled in with attributes of opened file (can be NULL)
 */
long
do_open (FILEPTR **f, const char *name, int rwmode, int attr, XATTR *x)
{
	struct proc *p = get_curproc();

	fcookie dir, fc;
	long devsp = 0;
	DEVDRV *dev;
	long r;
	XATTR xattr;
	unsigned perm;
	int creating, exec_check;
	char temp1[PATH_MAX];
	short cur_gid, cur_egid;

	TRACE (("do_open(%s)", name));

	/*
	 * first step: get a cookie for the directory
	 */
	r = path2cookie (p, name, temp1, &dir);
	if (r)
	{
		DEBUG (("do_open(%s): error %ld", name, r));
		return r;
	}

	/*
	 * If temp1 is a NULL string, then use the name again.
	 *
	 * This can occur when trying to open the ROOT directory of a
	 * drive. i.e. /, or C:/ or D:/ etc.
	 */
	if (*temp1 == '\0') {
		strncpy(temp1, name, PATH_MAX);
	}

	/*
	 * second step: try to locate the file itself
	 */
	r = relpath2cookie (p, &dir, temp1, follow_links, &fc, 0);

# if 1
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 *	...or if this is Fcreate with nonzero attr on the pipe filesystem
	 */
	if ((r == 0) && ((rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL) ||
			(attr && fc.fs == &pipe_filesys &&
			(rwmode & (O_CREAT|O_TRUNC)) == (O_CREAT|O_TRUNC))))
# else
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 */
	if ((r == 0) && ((rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)))
# endif
	{
		DEBUG (("do_open(%s): file already exists", name));
		release_cookie (&fc);
		release_cookie (&dir);

		return EACCES;
	}

	/* file not found: maybe we should create it
	 * note that if r != 0, the fc cookie is invalid (so we don't need to
	 * release it)
	 */
	if (r == ENOENT && (rwmode & O_CREAT))
	{
		/* check first for write permission in the directory */
		r = xfs_getxattr (dir.fs, &dir, &xattr);
		if (r == 0)
		{
			if (denyaccess (p->p_cred->ucr, &xattr, S_IWOTH))
				r = EACCES;
		}

		if (r)
		{
			DEBUG(("do_open(%s): couldn't get "
			      "write permission on directory", name));
			release_cookie (&dir);
			return r;
		}

		assert (p->p_cred);

		/* fake gid if directories setgid bit set */
		cur_gid = p->p_cred->rgid;
		cur_egid = p->p_cred->ucr->egid;

		if (xattr.mode & S_ISGID)
		{
			p->p_cred->rgid = xattr.gid;

			p->p_cred->ucr = copy_cred (p->p_cred->ucr);
			p->p_cred->ucr->egid = xattr.gid;
		}

		assert (p->p_cwd);

		r = xfs_creat (dir.fs, &dir, temp1,
			(S_IFREG|DEFAULT_MODE) & (~p->p_cwd->cmask), attr, &fc);

		p->p_cred->rgid = cur_gid;
		p->p_cred->ucr->egid = cur_egid;

		if (r)
		{
			DEBUG(("do_open(%s): error %ld while creating file",
				name, r));
			release_cookie (&dir);
			return r;
		}

		creating = 1;
	}
	else if (r)
	{
		DEBUG(("do_open(%s): error %ld while searching for file",
			name, r));
		release_cookie (&dir);
		return r;
	}
	else
	{
		creating = 0;
	}

	/* check now for permission to actually access the file
	 */
	r = xfs_getxattr (fc.fs, &fc, &xattr);
	if (r)
	{
		DEBUG(("do_open(%s): couldn't get file attributes", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return r;
	}

	DEBUG(("do_open(%s): mode 0x%x", name, xattr.mode));

	/* we don't do directories other than read-only
	 */
	if (S_ISDIR(xattr.mode) && ((rwmode & O_RWMODE) != O_RDONLY))
	{
		DEBUG(("do_open(%s): file is a directory", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return ENOENT;
	}

	exec_check = 0;
	switch (rwmode & O_RWMODE)
	{
		case O_WRONLY:
			perm = S_IWOTH;
			break;
		case O_RDWR:
			perm = S_IROTH|S_IWOTH;
			break;
		case O_EXEC:
			if (fc.fs->fsflags & FS_NOXBIT)
				perm = S_IROTH;
			else
			{
				perm = S_IXOTH;

				assert (p->p_cred);

				if (p->p_cred->ucr->euid == 0)
					exec_check = 1;	/* superuser needs 1 x bit */
			}
			break;
		case O_RDONLY:
			perm = S_IROTH;
			break;
		default:
			perm = 0;
			ALERT ("do_open: bad file access mode: %x", rwmode);
	}

	/* access checking;  additionally, the superuser needs at least one
	 * execute right to execute a file
	 */
	if ((exec_check && ((xattr.mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0))
		|| (!creating && denyaccess (p->p_cred->ucr, &xattr, perm)))
	{
		DEBUG(("do_open(%s): access to file denied", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return EACCES;
	}

	/* an extra check for write access -- even the superuser shouldn't
	 * write to files with the FA_RDONLY attribute bit set (unless,
	 * we just created the file, or unless the file is on the proc
	 * file system and hence FA_RDONLY has a different meaning)
	 */
	if (!creating && (xattr.attr & FA_RDONLY) && fc.fs != &proc_filesys)
	{
		if ((rwmode & O_RWMODE) == O_RDWR || (rwmode & O_RWMODE) == O_WRONLY)
		{
			DEBUG(("do_open(%s): can't write a read-only file", name));
			release_cookie (&dir);
			release_cookie (&fc);
			return EACCES;
		}
	}

	/* if writing to a setuid or setgid file, clear those bits
	 */
	if ((perm & S_IWOTH) && (xattr.mode & (S_ISUID|S_ISGID)))
	{
		xattr.mode &= ~(S_ISUID|S_ISGID);
		xfs_chmode (fc.fs, &fc, (xattr.mode & ~S_IFMT));
	}

	/* If the caller asked for the attributes of the opened file, copy them over.
	 */
	if (x) *x = xattr;

	/* So far, so good. Let's get the device driver now, and try to
	 * actually open the file.
	 */
	dev = xfs_getdev (fc.fs, &fc, &devsp);
	if (!dev)
	{
		DEBUG (("do_open(%s): device driver not found (%li)", name, devsp));
		release_cookie (&dir);
		release_cookie (&fc);
		return devsp ? devsp : EINTERNAL;
	}

	assert (f && *f);

	if (dev == &fakedev)
	{
		/* fake BIOS devices */
		FILEPTR *fp;

		assert (p->p_fd);

		fp = p->p_fd->ofiles[devsp];
		if (!fp || fp == (FILEPTR *) 1)
			return EBADF;

		(*f)->links--;
		FP_FREE (*f);

		*f = fp;
		fp->links++;

		release_cookie (&dir);
		release_cookie (&fc);

		return 0;
	}

	(*f)->links = 1;
	(*f)->flags = rwmode;
	(*f)->pos = 0;
	(*f)->devinfo = devsp;
	(*f)->fc = fc;
	(*f)->dev = dev;
	release_cookie (&dir);

	r = xdd_open (*f);
	if (r < E_OK)
	{
		DEBUG(("do_open(%s): device open failed with error %ld", name, r));
		release_cookie (&fc);
		return r;
	}

	/* special code for opening a tty */
	if (is_terminal (*f))
	{
		struct tty *tty;

		tty = (struct tty *) ((*f)->devinfo);

		/* in the middle of a hangup */
		while (tty->hup_ospeed && !creating)
			sleep (IO_Q, (long) &tty->state);

		tty->use_cnt++;

		/* first open for this device (not counting set_auxhandle)? */
		if ((!tty->pgrp && tty->use_cnt - tty->aux_cnt <= 1)
			|| tty->use_cnt <= 1)
		{
			short s = tty->state & (TS_BLIND|TS_HOLD|TS_HPCL);
			short u = tty->use_cnt;
			short a = tty->aux_cnt;
			long rsel = tty->rsel;
			long wsel = tty->wsel;
			*tty = default_tty;

			if (!creating)
				tty->state = s;

			if ((tty->use_cnt = u) > 1 || !creating)
			{
				tty->aux_cnt = a;
				tty->rsel = rsel;
				tty->wsel = wsel;
			}

			if (!((*f)->flags & O_HEAD))
				tty_ioctl (*f, TIOCSTART, 0);
		}

# if 0
		/* XXX fn: wait until line is online */
		if (!((*f)->flags & O_NDELAY) && (tty->state & TS_BLIND))
			(*f->dev->ioctl)((*f), TIOCWONLINE, 0);
# endif
	}

	DEBUG(("do_open(%s) -> 0", name));
	return 0;
}

/* 2500 ms after hangup: close device, ready for use again
 */
static void _cdecl
hangup_done (struct proc *p, FILEPTR *f)
{
	struct tty *tty = (struct tty *) f->devinfo;
	UNUSED (p);

	tty->hup_ospeed = 0;
	tty->state &= ~TS_HPCL;
	tty_ioctl (f, TIOCSTART, 0);
	wake (IO_Q, (long) &tty->state);
	tty->state &= ~TS_HPCL;

	if (--f->links <= 0)
	{
		if (--tty->use_cnt-tty->aux_cnt <= 0)
		{
			tty->pgrp = 0;
			DEBUG(("hangup_done: assigned tty->pgrp = %i", tty->pgrp));
		}

		if (tty->use_cnt <= 0 && tty->xkey)
		{
			kfree (tty->xkey);
			tty->xkey = 0;
		}
	}

	/* XXX hack(?): the closing process may no longer exist, use pid 0 */
	if ((*f->dev->close)(f, 0))
		DEBUG (("hangup: device close failed"));

	if (f->links <= 0)
	{
		release_cookie (&f->fc);
		FP_FREE (f);
	}
}

/* 500 ms after hangup: restore DTR
 */
static void _cdecl
hangup_b1 (struct proc *p, FILEPTR *f)
{
	struct tty *tty = (struct tty *) f->devinfo;
	TIMEOUT *t = addroottimeout (2000L, (to_func *) hangup_done, 0);

	if (tty->hup_ospeed > 0)
		(*f->dev->ioctl)(f, TIOCOBAUD, &tty->hup_ospeed);

	if (!t)
	{
		/* should never happen, but... */
		hangup_done (p, f);
		return;
	}

	t->arg = (long) f;
	tty->hup_ospeed = -1;
}

/*
 * helper function for do_close: this closes the indicated file pointer which
 * is assumed to be associated with process p. The extra parameter is necessary
 * because f_midipipe mucks with file pointers of other processes, so
 * sometimes p != curproc.
 *
 * Note that the function changedrv() in filesys.c can call this routine.
 * in that case, f->dev will be 0 to represent an invalid device, and
 * we cannot call the device close routine.
 */

long
do_close (struct proc *p, FILEPTR *f)
{
	long r = E_OK;

	if (!f) return EBADF;
	if (f == (FILEPTR *) 1)
		return E_OK;

	/* if this file is "select'd" by this process, unselect it
	 * (this is just in case we were killed by a signal)
	 */

	/* BUG? Feature? If media change is detected while we're doing the select,
	 * we'll never unselect (since f->dev is set to NULL by changedrv())
	 */
	if (f->dev)
	{
		(*f->dev->unselect)(f, (long) p, O_RDONLY);
		(*f->dev->unselect)(f, (long) p, O_WRONLY);
		(*f->dev->unselect)(f, (long) p, O_RDWR);
		wake (SELECT_Q, (long) &select_coll);
	}

	f->links--;
	if (f->links < 0)
	{
		ALERT ("do_close on invalid file struct! (links = %i)", f->links);
// XXX		return 0;
	}

	/* TTY manipulation must be done *before* calling the device close routine,
	 * since afterwards the TTY structure may no longer exist
	 */
	if (is_terminal (f) && f->links <= 0)
	{
		struct tty *tty = (struct tty *) f->devinfo;
		TIMEOUT *t;
		long ospeed = -1L, z = 0;

		/* for HPCL ignore ttys open as /dev/aux, else they would never hang up */
		if (tty->use_cnt-tty->aux_cnt <= 1)
		{
			if ((tty->state & TS_HPCL) && !tty->hup_ospeed &&
			    !(f->flags & O_HEAD) &&
			    (*f->dev->ioctl)(f, TIOCOBAUD, &ospeed) >= 0 &&
			    NULL != (t = addroottimeout(500L, (to_func *)hangup_b1, 0))) {
			/* keep device open until hangup complete */
				f->links = 1;
				++tty->use_cnt;
			/* pass f to timeout function */
				t->arg = (long)f;
				(*f->dev->ioctl)(f, TIOCCBRK, 0);
			/* flag: hanging up */
				tty->hup_ospeed = -1;
			/* stop output, flush buffers, drop DTR... */
				tty_ioctl(f, TIOCSTOP, 0);
				tty_ioctl(f, TIOCFLUSH, 0);
				if (ospeed > 0) {
					tty->hup_ospeed = ospeed;
					(*f->dev->ioctl)(f, TIOCOBAUD, &z);
				}
			}
			else
			{
				tty->pgrp = 0;
				DEBUG(("do_close: assigned tty->pgrp = %i", tty->pgrp));
			}
		}

		tty->use_cnt--;
		if (tty->use_cnt <= 0 && tty->xkey)
		{
			kfree (tty->xkey);
			tty->xkey = 0;
		}
	}

	if (f->dev)
	{
		r = xdd_close (f, p->pid);
		if (r) DEBUG (("close: device close failed"));
	}

	if (f->links <= 0)
	{
		release_cookie (&f->fc);
		FP_FREE (f);
	}

	return  r;
}
