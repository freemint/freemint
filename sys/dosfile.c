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

/*
 * first, some utility routines
 */

/* do_open(name, rwmode, attr, x, err)
 * 
 * name   - file name
 * rwmode - file access mode
 * attr   - TOS attributes for created files (if applicable)
 * x      - filled in with attributes of opened file (can be NULL)
 * err    - for the error value (can be NUL)
 */
FILEPTR *
do_open (const char *name, int rwmode, int attr, XATTR *x, long *err)
{
	PROC *p = curproc;
	
	struct tty *tty;
	fcookie dir, fc;
	long devsp;
	FILEPTR *f;
	DEVDRV *dev;
	long r;
	XATTR xattr;
	unsigned perm;
	int creating, exec_check;
	char temp1[PATH_MAX];
	short cur_gid, cur_egid;

	TRACE(("do_open(%s)", name));
	assert (p->p_fd && p->p_cwd);
	
	
	/*
	 * first step: get a cookie for the directory
	 */
	r = path2cookie(name, temp1, &dir);
	if (r)
	{
		DEBUG(("do_open(%s): error %ld", name, r));
		if (err) *err = r;
		return NULL;
	}

	/*
	 * second step: try to locate the file itself
	 */
	r = relpath2cookie(&dir, temp1, follow_links, &fc, 0);

#ifdef CREATE_PIPES
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 *	...or if this is Fcreate with nonzero attr on the pipe filesystem
	 */
	if ( (r == 0) && ( (rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL) ||
			(attr && fc.fs == &pipe_filesys &&
			(rwmode & (O_CREAT|O_TRUNC)) == (O_CREAT|O_TRUNC)))) {
#else
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 */
	if ((r == 0) && ( (rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL))) {
#endif
		DEBUG(("do_open(%s): file already exists", name));
		release_cookie (&fc);
		release_cookie (&dir);
		
		if (err) *err = EACCES;
		return NULL;
	}
	
	/* file not found: maybe we should create it
	 * note that if r != 0, the fc cookie is invalid (so we don't need to
	 * release it)
	 */
	if (r == ENOENT && (rwmode & O_CREAT))
	{
		struct pcred *cred = p->p_cred;
		
		/* check first for write permission in the directory */
		r = xfs_getxattr (dir.fs, &dir, &xattr);
		if (r == 0)
		{
			if (denyaccess (&xattr, S_IWOTH))
				r = EACCES;
		}
		
		if (r)
		{
			DEBUG(("do_open(%s): couldn't get "
			      "write permission on directory",name));
			release_cookie (&dir);
			if (err) *err = r;
			return NULL;
		}
		
		/* fake gid if directories setgid bit set */
		cur_gid = cred->rgid;
		cur_egid = cred->ucr->egid;
		
		if (xattr.mode & S_ISGID)
			cred->rgid = cred->ucr->egid = xattr.gid;
		
		r = xfs_creat (dir.fs, &dir, temp1,
			(S_IFREG|DEFAULT_MODE) & (~p->p_cwd->cmask), attr, &fc);
		cred->rgid = cur_gid;
		cred->ucr->egid = cur_egid;
		
		if (r)
		{
			DEBUG(("do_open(%s): error %ld while creating file",
				name, r));
			release_cookie (&dir);
			if (err) *err = r;
			return NULL;
		}
		creating = 1;
	}
	else if (r)
	{
		DEBUG(("do_open(%s): error %ld while searching for file",
			name, r));
		release_cookie (&dir);
		if (err) *err = r;
		return NULL;
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
		DEBUG(("do_open(%s): couldn't get file attributes",name));
		release_cookie (&dir);
		release_cookie (&fc);
		if (err) *err = r;
		return NULL;
	}
	
	/* we don't do directories
	 */
	if ((xattr.mode & S_IFMT) == S_IFDIR)
	{
		DEBUG(("do_open(%s): file is a directory",name));
		release_cookie (&dir);
		release_cookie (&fc);
		if (err) *err = ENOENT;
		return NULL;
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
				if (p->p_cred->ucr->euid == 0)
					exec_check = 1;	/* superuser needs 1 x bit */
			}
			break;
		case O_RDONLY:
			perm = S_IROTH;
			break;
		default:
			perm = 0;
			ALERT("do_open: bad file access mode: %x", rwmode);
	}
	
	/* access checking;  additionally, the superuser needs at least one
	 * execute right to execute a file
	 */
	if ((exec_check && ((xattr.mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0))
		|| (!creating && denyaccess (&xattr, perm)))
	{
		DEBUG(("do_open(%s): access to file denied",name));
		release_cookie (&dir);
		release_cookie (&fc);
		if (err) *err = EACCES;
		return NULL;
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
			if (err) *err = EACCES;
			return NULL;
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
		DEBUG(("do_open(%s): device driver not found",name));
		release_cookie (&dir);
		release_cookie (&fc);
		if (err) *err = devsp;
		return NULL;
	}
	
	if (dev == &fakedev)
	{
		/* fake BIOS devices */
		f = p->p_fd->ofiles[devsp];
		if (!f || f == (FILEPTR *) 1)
		{
			if (err) *err = EBADF;
			return NULL;
		}
		f->links++;
		release_cookie(&dir);
		release_cookie(&fc);
		return f;
	}
	
	f = new_fileptr ();
	if (!f)
	{
		release_cookie (&dir);
		release_cookie (&fc);
		
		if (err) *err = ENOMEM;
		return NULL;
	}
	
	f->links = 1;
	f->flags = rwmode;
	f->pos = 0;
	f->devinfo = devsp;
	f->fc = fc;
	f->dev = dev;
	release_cookie(&dir);
	
	r = xdd_open (f);
	if (r < E_OK)
	{
		DEBUG(("do_open(%s): device open failed with error %ld", name, r));
		
		release_cookie (&fc);
		
		f->links = 0;
		dispose_fileptr (f);
		
		if (err) *err = r;
		DEBUG (("do_open: ok"));
		return NULL;
	}
	
	/* special code for opening a tty */
	if (is_terminal (f))
	{
		tty = (struct tty *) f->devinfo;
		
		/* in the middle of a hangup */
		while (tty->hup_ospeed && !creating)
			sleep (IO_Q, (long) &tty->state);
		
		tty->use_cnt++;
		
		/* first open for this device (not counting set_auxhandle)? */
		if ((!tty->pgrp && tty->use_cnt-tty->aux_cnt <= 1)
			|| tty->use_cnt <= 1)
		{
			short s = tty->state & (TS_BLIND|TS_HOLD|TS_HPCL);
			short u = tty->use_cnt;
			short a = tty->aux_cnt;
			long r = tty->rsel;
			long w = tty->wsel;
			*tty = default_tty;
			
			if (!creating)
				tty->state = s;
			
			if ((tty->use_cnt = u) > 1 || !creating)
			{
				tty->aux_cnt = a;
				tty->rsel = r;
				tty->wsel = w;
			}
			
			if (!(f->flags & O_HEAD))
				tty_ioctl (f, TIOCSTART, 0);
		}
		
# if 0
		/* fn: wait until line is online */
		if (!(f->flags & O_NDELAY) && (tty->state & TS_BLIND))
			(*f->dev->ioctl)(f, TIOCWONLINE, 0);
# endif
	}
	
	return f;
}

/* 2500 ms after hangup: close device, ready for use again */

static void _cdecl
hangup_done (PROC *p, FILEPTR *f)
{
	struct tty *tty = (struct tty *)f->devinfo;
	UNUSED (p);

	tty->hup_ospeed = 0;
	tty->state &= ~TS_HPCL;
	tty_ioctl(f, TIOCSTART, 0);
	wake (IO_Q, (long)&tty->state);
	tty->state &= ~TS_HPCL;
	if (--f->links <= 0) {
		if (--tty->use_cnt-tty->aux_cnt <= 0)
			tty->pgrp = 0;
		if (tty->use_cnt <= 0 && tty->xkey) {
			kfree(tty->xkey);
			tty->xkey = 0;
		}
	}

	/* XXX hack(?): the closing process may no longer exist, use pid 0 */
	if ((*f->dev->close)(f, 0)) {
		DEBUG(("hangup: device close failed"));
	}
	if (f->links <= 0) {
		release_cookie(&f->fc);
		dispose_fileptr(f);
	}
}

/* 500 ms after hangup: restore DTR */

static void _cdecl
hangup_b1 (PROC *p, FILEPTR *f)
{
	struct tty *tty = (struct tty *)f->devinfo;
	TIMEOUT *t = addroottimeout (2000L, (to_func *)hangup_done, 0);

	if (tty->hup_ospeed > 0)
		(*f->dev->ioctl)(f, TIOCOBAUD, &tty->hup_ospeed);
	if (!t) {
		/* should never happen, but... */
		hangup_done(p, f);
		return;
	}
	t->arg = (long)f;
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
do_pclose (PROC *p, FILEPTR *f)
{
	long r = E_OK;

	if (!f) return EBADF;
	if (f == (FILEPTR *)1)
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

	/* TTY manipulation must be done *before* calling the device close routine,
	 * since afterwards the TTY structure may no longer exist
	 */
	if (is_terminal(f) && f->links <= 0)
	{
		struct tty *tty = (struct tty *)f->devinfo;
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
				tty->pgrp = 0;
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
		if (r) DEBUG(("close: device close failed"));
	}
	
	if (f->links <= 0)
	{
		release_cookie (&f->fc);
		dispose_fileptr (f);
	}
	
	return  r;
}

long
do_close(FILEPTR *f)
{
	return do_pclose(curproc, f);
}

long _cdecl
f_open (const char *name, int mode)
{
	long r;
	int i;
	FILEPTR *f;
	PROC *proc;
	int global = 0;

	TRACE(("Fopen(%s, %x)", name, mode));
#if O_GLOBAL
	if (mode & O_GLOBAL) {
	    /* oh, boy! user wants us to open a global handle! */
	    ALERT ("Opening a global handle :-(");
	    proc = rootproc;
	    global = 1;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	for (i = MIN_OPEN; i < proc->p_fd->nfiles; i++) {
		if (!proc->p_fd->ofiles[i])
			goto found_for_open;
	}
	DEBUG(("Fopen(%s): process out of handles",name));
	return EMFILE;		/* no more handles */

found_for_open:
	mode &= O_USER;		/* make sure the mode is legal */

/* note: file mode 3 is reserved for the kernel; for users, transmogrify it
 * into O_RDWR (mode 2)
 */
	if ((mode & O_RWMODE) == O_EXEC) {
		mode = (mode & ~O_RWMODE) | O_RDWR;
	}

	proc->p_fd->ofiles[i] = (FILEPTR *) 1;	/* reserve this handle */
	f = do_open(name, mode, 0, NULL, &r);
	proc->p_fd->ofiles[i] = NULL;

	if (!f) {
		return r;
	}
	proc->p_fd->ofiles[i] = f;
/* default is to close non-standard files on exec */
	proc->p_fd->ofileflags[i] = FD_CLOEXEC;

#if O_GLOBAL
	if (global) {
	    /* we just opened a global handle */
	    i += 100;
	}
#endif

	TRACE(("Fopen: returning %d", i));
	return i;
}

long _cdecl
f_create(const char *name, int attrib)
{
	fcookie dir;
	int i;
	FILEPTR *f;
	long r;
	PROC *proc;
	int offset = 0;
	char temp1[PATH_MAX];

	TRACE(("Fcreate(%s, %x)", name, attrib));
#if O_GLOBAL
	if (attrib & O_GLOBAL) {
		proc = rootproc;
		offset = 100;
		attrib &= ~O_GLOBAL;
	}
	else
#endif
		proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	for (i = MIN_OPEN; i < proc->p_fd->nfiles; i++) {
		if (!proc->p_fd->ofiles[i])
			goto found_for_create;
	}
	DEBUG(("Fcreate(%s): process out of handles",name));
	return EMFILE;		/* no more handles */

found_for_create:
	if (attrib == FA_LABEL) {
		r = path2cookie(name, temp1, &dir);
		if (r) return r;
		r = xfs_writelabel (dir.fs, &dir, temp1);
		release_cookie(&dir);
		if (r) return r;
/*
 * just in case the caller tries to do something with this handle,
 * make it point to u:\dev\null
 */
		f = do_open("u:\\dev\\null", O_RDWR|O_CREAT|O_TRUNC, 0, NULL, NULL);
		assert (f);
		proc->p_fd->ofiles[i] = f;
		proc->p_fd->ofileflags[i] = FD_CLOEXEC;
		return i+offset;
	}
	if (attrib & (FA_LABEL|FA_DIR)) {
		DEBUG(("Fcreate(%s,%x): illegal attributes",name,attrib));
		return EACCES;
	}

	proc->p_fd->ofiles[i] = (FILEPTR *)1;		/* reserve this handle */
	f = do_open(name, O_RDWR|O_CREAT|O_TRUNC, attrib, NULL, &r);
	proc->p_fd->ofiles[i] = NULL;

	if (!f) {
		DEBUG(("Fcreate(%s) failed, error %d", name, r));
		return r;
	}
	proc->p_fd->ofiles[i] = f;
	proc->p_fd->ofileflags[i] = FD_CLOEXEC;
	i += offset;
	TRACE(("Fcreate: returning %d", i));
	return i;
}

long _cdecl
f_close(int fh)
{
	FILEPTR *f;
	long r;
	PROC *proc;

	TRACE(("Fclose: %d", fh));
#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < 0 || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh])) {
		return EBADF;
	}
	r = do_pclose(proc, f);

/* standard handles should be restored to default values */
/* do this for TOS domain only! */
	if (proc->domain == DOM_TOS) {
		if (fh == 0 || fh == 1)
			f = proc->p_fd->ofiles[-1];
		else if (fh == 2 || fh == 3)
			f = proc->p_fd->ofiles[-fh];
		else
			f = 0;
	} else
		f = 0;

	if (f) {
		f->links++;
		proc->p_fd->ofileflags[fh] = 0;
	}
	proc->p_fd->ofiles[fh] = f;
	return r;
}

long _cdecl
f_read(int fh, long count, char *buf)
{
	FILEPTR *f;
	PROC *proc;

#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh])) {
		DEBUG(("Fread: invalid handle: %d", fh));
		return EBADF;
	}
	if ((f->flags & O_RWMODE) == O_WRONLY) {
		DEBUG(("Fread: read on a write-only handle"));
		return EACCES;
	}
	if (is_terminal(f))
		return tty_read(f, buf, count);

	TRACELOW(("Fread: %ld bytes from handle %d to %lx", count, fh, buf));
	return xdd_read (f, buf, count);
}

long _cdecl
f_write(int fh, long count, const char *buf)
{
	FILEPTR *f;
	PROC *proc;
	long r;

#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh])) {
		DEBUG(("Fwrite: bad handle: %d", fh));
		return EBADF;
	}
	if ((f->flags & O_RWMODE) == O_RDONLY) {
		DEBUG(("Fwrite: write on a read-only handle"));
		return EACCES;
	}
	if (is_terminal(f))
		return tty_write(f, buf, count);

	/* Prevent broken device drivers from wiping the disk.
	 * We return a zero rather than a negative error code
	 * to help programs those don't handle GEMDOS errors
	 * returned by F_write()
	 */
	if (count <= 0) {
		DEBUG(("Fwrite: invalid count: %d", count));
		return 0;
	}

	/* it would be faster to do this in the device driver, but this
	 * way the drivers are easier to write
	 */
	if (f->flags & O_APPEND) {
		r = xdd_lseek (f, 0L, SEEK_END);
		/* ignore errors from unseekable files (e.g. pipes) */
		if (r == EACCES)
			r = 0;
	} else
		r = 0;
	if (r >= 0) {
		TRACELOW(("Fwrite: %ld bytes to handle %d", count, fh));
		r = xdd_write (f, buf, count);
	}
	if (r < 0) {
		DEBUG(("Fwrite: error %ld", r));
	}
	return r;
}

long _cdecl
f_seek (long place, int fh, int how)
{
	FILEPTR *f;
	PROC *proc;

	TRACE(("Fseek(%ld, %d) on handle %d", place, how, fh));
#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh])) {
		DEBUG(("Fseek: bad handle: %d", fh));
		return EBADF;
	}
	if (is_terminal(f)) {
		return 0;
	}
	return xdd_lseek (f, place, how);
}

/* duplicate file pointer fh; returns a new file pointer >= min, if
   one exists, or EMFILE if not. called by f_dup and f_cntl
 */

static long
do_dup (int fh, int min)
{
	FILEPTR *f;
	int i;
	PROC *proc;

	assert (curproc->p_fd && curproc->p_cwd);
	
	for (i = min; i < curproc->p_fd->nfiles; i++) {
		if (!curproc->p_fd->ofiles[i])
			goto found;
	}
	return EMFILE;		/* no more handles */
found:
#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	} else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh]))
		return EBADF;

	curproc->p_fd->ofiles[i] = f;

/* set default file descriptor flags */
	if (i >= 0) {
		if (i >= MIN_OPEN)
			curproc->p_fd->ofileflags[i] = FD_CLOEXEC;
		else
			curproc->p_fd->ofileflags[i] = 0;
	}
	f->links++;
	return i;
}

long _cdecl
f_dup(int fh)
{
	long r;
	r = do_dup(fh, MIN_OPEN);
	TRACE(("Fdup(%d) -> %ld", fh, r));
	return r;
}

long _cdecl
f_force(int newh, int oldh)
{
	FILEPTR *f;
	PROC *proc;

	TRACE(("Fforce(%d, %d)", newh, oldh));

#if O_GLOBAL
	if (oldh >= 100) {
	    oldh -= 100;
	    proc = rootproc;
	} else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (oldh < MIN_HANDLE || oldh >= proc->p_fd->nfiles ||
	    0 == (f = proc->p_fd->ofiles[oldh])) {
		DEBUG(("Fforce: old handle invalid"));
		return EBADF;
	}

	if (newh < MIN_HANDLE || newh >= proc->p_fd->nfiles) {
		DEBUG(("Fforce: new handle out of range"));
		return EBADF;
	}

	(void)do_close(curproc->p_fd->ofiles[newh]);
	curproc->p_fd->ofiles[newh] = f;
	/* set default file descriptor flags */
	if (newh >= 0)
		curproc->p_fd->ofileflags[newh] = (newh >= MIN_OPEN) ? FD_CLOEXEC : 0;
	f->links++;
/*
 * special: for a tty, if this is becoming a control terminal and the
 * tty doesn't have a pgrp yet, make it have the pgrp of the process
 * doing the Fforce
 */
	if (is_terminal(f) && newh == -1 && !(f->flags & O_HEAD)) {
		struct tty *tty = (struct tty *)f->devinfo;

		if (!tty->pgrp) {
			tty->pgrp = curproc->pgrp;

			if (!(f->flags & O_NDELAY) && (tty->state & TS_BLIND))
				(*f->dev->ioctl)(f, TIOCWONLINE, 0);
		}
	}
	return E_OK;
}

long _cdecl
f_datime (ushort *timeptr, int fh, int wflag)
{
	FILEPTR *f;
	PROC *proc;
	
	TRACE (("Fdatime(%d)", fh));
# if O_GLOBAL
	if (fh >= 100)
	{
	    fh -= 100;
	    proc = rootproc;
	}
	else
# endif
	    proc = curproc;
	
	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh]))
	{
		DEBUG (("Fdatime: invalid handle"));
		return EBADF;
	}
	
	/* some programs use Fdatime to test for TTY devices */
	if (is_terminal (f))
		return EACCES;
	
	if (f->fc.fs->fsflags & FS_EXT_3)
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
f_lock(int fh, int mode, long start, long length)
{
	FILEPTR *f;
	struct flock lock;
	PROC *proc;

#if O_GLOBAL
	if (fh >= 100) {
	    fh -= 100;
	    proc = rootproc;
	}
	else
#endif
	    proc = curproc;

	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh])) {
		DEBUG(("Flock: invalid handle"));
		return EBADF;
	}
	TRACE(("Flock(%d,%d,%ld,%ld)", fh, mode, start, length));
	lock.l_whence = SEEK_SET;
	lock.l_start = start;
	lock.l_len = length;

	if (mode == 0)		/* create a lock */
		lock.l_type = F_WRLCK;
	else if (mode == 1)	/* unlock region */
		lock.l_type = F_UNLCK;
	else
		return ENOSYS;

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
f_pipe (short int *usrh)
{
	FILEPTR *in, *out;
	static int pipeno = 0;
	int i, j;
	/* MAGIC: 32 >= strlen "u:\pipe\sys$pipe.000\0" */
	char pipename[32];
	long r;
	
	TRACE (("Fpipe"));
	
	/* BUG: more than 999 open pipes hangs the system
	 */
	do {
		ksprintf (pipename, sizeof (pipename), "u:\\pipe\\sys$pipe.%03d", pipeno);
		pipeno++;
		if (pipeno > 999)
			pipeno = 0;
		
		/* read-only attribute means unidirectional fifo
		 * hidden attribute means check for broken pipes
		 * changed attribute means act like Unix fifos
		 */
		out = do_open (pipename, O_WRONLY|O_CREAT|O_EXCL, FA_RDONLY|FA_HIDDEN|FA_CHANGED, NULL, &r);
	}
	while (out == 0 && r == EACCES);
	
	if (!out)
	{
		DEBUG (("Fpipe: error %d", r));
		return r;
	}
	
	in = do_open (pipename, O_RDONLY, 0, NULL, &r);
	if (!in)
	{
		DEBUG (("Fpipe: in side of pipe not opened (error %d)", r));
		(void) do_close (out);
		return r;
	}
	
	for (i = MIN_OPEN; i < curproc->p_fd->nfiles; i++)
	{
		if (curproc->p_fd->ofiles[i] == 0)
			break;
	}
	
	for (j = i+1; j < curproc->p_fd->nfiles; j++)
	{
		if (curproc->p_fd->ofiles[j] == 0)
			break;
	}
	
	if (j >= curproc->p_fd->nfiles)
	{
		(void) do_close (in);
		(void) do_close (out);
		
		DEBUG (("Fpipe: not enough handles left"));
		return EMFILE;
	}
	
	curproc->p_fd->ofiles[i] = in;
	curproc->p_fd->ofiles[j] = out;
	
	/* leave pipes open across Pexec */
	curproc->p_fd->ofileflags[i] = 0;
	curproc->p_fd->ofileflags[j] = 0;
	
	usrh[0] = i;
	usrh[1] = j;
	
	TRACE (("Fpipe: returning E_OK: input %d output %d",i,j));
	return E_OK;
}

/* get_opens: subroutine for Dcntl F_GETOPENS
 */
long
get_opens (fcookie *object, struct listopens *l)
{
	PROC *p;
#define INVALID_PID 32768L
	long last_pid = INVALID_PID;
	int start_pid = l->lo_pid;
	int search_for = l->lo_reason;

	l->lo_reason = 0;

    for (p = proclist; p; p = p->gl_next)
	{
		struct filedesc *fds = p->p_fd;
		struct cwd *cwd = p->p_cwd;
		DIR *dirp;
		int i;

		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;

		assert (fds && cwd);
		
		if (p->pid < start_pid || p->pid > last_pid)
			continue;

		/* check open files */

		if (search_for & LO_FILEOPEN)
		{
			for (i = MIN_HANDLE; i < fds->nfiles; i++)
			{
				FILEPTR *f = fds->ofiles[i];

				if (f != (FILEPTR *) 0L && f != (FILEPTR *) 1L)
				{
					if (samefile (&f->fc, object))
					{
						if (l->lo_pid != p->pid)
						{
							l->lo_pid = p->pid;
							l->lo_reason = 0;
							l->lo_flags = f->flags;
						}
						l->lo_reason |= LO_FILEOPEN;
					}
				}
			}
		}

		/* check current directories */
		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (search_for & LO_CURROOT)
			{
				if (samefile (object, &cwd->root[i]))
				{
					if (l->lo_pid != p->pid)
					{
						l->lo_pid = p->pid;
						l->lo_reason = 0;
					}
					l->lo_reason |= LO_CURROOT;
				}
			}

			if (search_for & LO_CURDIR)
			{
				if (i == cwd->curdrv && samefile (object, &cwd->curdir[i]))
				{
					if (l->lo_pid != p->pid)
					{
						l->lo_pid = p->pid;
						l->lo_reason = 0;
					}
					l->lo_reason |= LO_CURDIR;
				}
			}
		}

		/* check for opened directories */
		if (search_for & LO_DIROPEN)
		{
			for (dirp = p->searches; dirp; dirp = dirp->next)
			{
				if (samefile (object, &dirp->fc))
				{
					if (l->lo_pid != p->pid)
					{
						l->lo_pid = p->pid;
						l->lo_reason = 0;
					}
					l->lo_reason |= LO_DIROPEN;
				}
			}
		}
	}

	return l->lo_reason == 0 ? ENMFILES : E_OK;
}



/*
 * f_cntl: a combination "ioctl" and "fcntl". Some functions are
 * handled here, if they apply to the file descriptors directly
 * (e.g. F_DUPFD) or if they're easily translated into file system
 * functions (e.g. FSTAT). Others are passed on to the device driver
 * via dev->ioctl.
 */

long _cdecl
f_cntl (int fh, long arg, int cmd)
{
	FILEPTR	*f;
	PROC *proc;
	long r;
	
	TRACE (("Fcntl(%d, cmd=0x%x)", fh, cmd));
# if O_GLOBAL
	if (fh >= 100)
	{
	    fh -= 100;
	    proc = rootproc;
	}
	else
# endif
	    proc = curproc;
	
	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Fcntl: bad file handle"));
		return EBADF;
	}
	
	if (cmd == F_DUPFD)
	{
# if O_GLOBAL
		if (proc != curproc)
			fh += 100;
# endif
  		return do_dup (fh, (int) arg);
	}
	
	f = proc->p_fd->ofiles[fh];
	if (!f)
		return EBADF;
	
	switch (cmd)
	{
		case F_GETFD:
		{
			TRACE (("Fcntl F_GETFD"));
			
			if (fh < 0)
				return EBADF;
			
			return proc->p_fd->ofileflags [fh];
		}
		case F_SETFD:
		{
			TRACE (("Fcntl F_SETFD"));
			
			if (fh < 0)
				return EBADF;
			
			proc->p_fd->ofileflags [fh] = arg;
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
			
			r = xfs_getxattr (f->fc.fs, &f->fc, xattr);
			if ((r == E_OK) && (f->fc.fs->fsflags & FS_EXT_3))
			{
				/* UTC -> localtime -> DOS style */
				*((long *) &(xattr->mtime)) = dostime (*((long *) &(xattr->mtime)) - timezone);
				*((long *) &(xattr->atime)) = dostime (*((long *) &(xattr->atime)) - timezone);
				*((long *) &(xattr->ctime)) = dostime (*((long *) &(xattr->ctime)) - timezone);
			}
			
			TRACE (("Fcntl FSTAT (%i, %lx) on \"%s\" -> %li", fh, xattr, xfs_name (&(f->fc)), r));
			return r;
		}
		case FSTAT64:
		{
			STAT *ptr = (STAT *) arg;
			
			if (f->fc.fs->fsflags & FS_EXT_3)
				r = xfs_stat64 (f->fc.fs, &f->fc, ptr);
			else
				r = EINVAL;
			
			TRACE (("Fcntl FSTAT64 (%i, %lx) on \"%s\" -> %li", fh, ptr, xfs_name (&(f->fc)), r));
			return r;
		}
		case F_GETOPENS:
		{
			return get_opens (&f->fc, (struct listopens *) arg);
		}
# if 0
/* fn: totally annyoing check, make only trouble */
		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *fl = (struct flock *) arg;
			
			/* make sure that the file was opened with appropriate permissions */
			if (fl->l_type == F_RDLCK)
			{
				if ((f->flags & O_RWMODE) == O_WRONLY)
					return EACCES;
			}
			else
			{
				if ((f->flags & O_RWMODE) == O_RDONLY)
					return EACCES;
			}
			
			break;
		}
# endif
		case FUTIME:
		{
			if ((f->fc.fs->fsflags & FS_EXT_3) && arg)
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
	
	TRACE (("Fcntl mode %x: calling ioctl",cmd));
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
	{
		r = xdd_ioctl (f, cmd, (void *) arg);
	}
	
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
f_select (unsigned timeout, long *rfdp, long *wfdp, long *xfdp)
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
f_midipipe (int pid, int in, int out)
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
	
	do_pclose (p, p->p_fd->midiin);
	do_pclose (p, p->p_fd->midiout);
	
	p->p_fd->midiin = fin;
	p->p_fd->midiout = fout;
	
	return E_OK;
}

/*
 * GEMDOS extension: Ffchown(fh, uid, gid) changes the user and group
 * ownerships of a open file to "uid" and "gid" respectively.
 */

long _cdecl
f_fchown (int fh, int uid, int gid)
{
	FILEPTR *f;
	PROC *proc;
	long r;
	
	TRACE (("Ffchown(%d,%i,%i)", fh, uid, gid));
# if O_GLOBAL
	if (fh >= 100)
	{
	    fh -= 100;
	    proc = rootproc;
	}
	else
# endif
	    proc = curproc;
	
	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh]))
	{
		DEBUG (("Ffchown: invalid handle"));
		return EBADF;
	}
	
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
	if (proc->p_cred->ucr->euid)
	{
		XATTR xattr;
		
		if (proc->p_cred->ucr->egid != gid && !ngroupmatch (proc->p_cred->ucr, gid))
			r = EACCES;
		else
			r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
		
		if (r)
		{
			DEBUG (("Ffchown(%i): unable to get file attributes",fh));
			return r;
		}
		
		if (xattr.uid != curproc->p_cred->ucr->euid || xattr.uid != uid)
		{
			DEBUG (("Ffchown(%i): not the file's owner",fh));
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
f_fchmod (int fh, unsigned mode)
{
	XATTR xattr;
	FILEPTR *f;
	PROC *proc;
	long r;
	
	TRACE (("Ffchmod(%d,%i)", fh, mode));
# if O_GLOBAL
	if (fh >= 100)
	{
	    fh -= 100;
	    proc = rootproc;
	}
	else
# endif
	    proc = curproc;
	
	assert (proc->p_fd && proc->p_cwd);
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles || 0 == (f = proc->p_fd->ofiles[fh]))
	{
		DEBUG (("Ffchmod: invalid handle"));
		return EBADF;
	}
	
	if (!(f->fc.fs))
	{
		DEBUG (("Ffchmod: not a valid filesystem"));
		return ENOSYS;
	}
	
	r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
	if (r)
	{
		DEBUG (("Ffchmod(%i): couldn't get file attributes",fh));
	}
	else if (proc->p_cred->ucr->euid && proc->p_cred->ucr->euid != xattr.uid)
	{
		DEBUG (("Ffchmod(%i): not the file's owner",fh));
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
f_seek64 (llong place, int fh, int how, llong *newpos)
{
	long r;
	
# define LONG_MAX	2147483647L
	if (place > LONG_MAX)
		return EBADARG;
	
	r = f_seek ((long) place, fh, how);
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
	(POLLIN | POLLPRI | POLLOUT | POLLERR | POLLHUP | POLLNVAL)
		
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
