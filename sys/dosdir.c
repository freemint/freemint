/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* DOS directory functions */

# include "dosdir.h"
# include "global.h"

# include "libkern/libkern.h"

# include "bios.h"
# include "dosfile.h"
# include "filesys.h"
# include "kmemory.h"
# include "proc.h"
# include "time.h"
# include "unifs.h"


/* change to a new drive: should always return a map of valid drives
 */
long _cdecl
d_setdrv (int d)
{
	long r;
	
	r = drvmap() | dosdrvs;
	
	TRACELOW (("Dsetdrv(%d)", d));
	if (d < 0 || d >= NUM_DRIVES
		|| (r & (1L << d)) == 0
		|| curproc->root_dir)
	{
		DEBUG (("Dsetdrv: invalid drive %d", d));
		return r;
	}
	
	curproc->base->p_defdrv = curproc->curdrv = d;
	return r;
}


long _cdecl
d_getdrv (void)
{
	TRACELOW (("Dgetdrv"));
	return curproc->curdrv;
}

long _cdecl
d_free(long *buf, int d)
{
	fcookie *dir = 0;
	FILESYS *fs;
	fcookie root;
	long r;

	TRACE(("Dfree(%d)", d));

/* drive 0 means current drive, otherwise it's d-1 */
	if (d)
		d = d-1;
	else
		d = curproc->curdrv;

/* If it's not a standard drive or an alias of one, get the pointer to
 * the filesystem structure and use the root directory of the
 * drive.
 */
	if (d < 0 || d >= NUM_DRIVES)
	{
		int i;
		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (aliasdrv[i] == d)
			{
				d = i;
				goto aliased;
			}
		}
		fs = get_filesys (d);
		if (!fs)
		  return ENXIO;
		r = xfs_root (fs, d, &root);
		if (r < E_OK)
		  return r;
		r = xfs_dfree (fs, &root, buf);
		release_cookie (&root);
		return r;
	}

/* check for a media change -- we don't care much either way, but it
 * does keep the results more accurate
 */
	(void)disk_changed(d);

aliased:

/* use current directory, not root, since it's more likely that
 * programs are interested in the latter (this makes U: work much
 * better)
 */
	dir = &curproc->curdir[d];
	if (!dir->fs) {
		DEBUG(("Dfree: bad drive"));
		return ENXIO;
	}

	return xfs_dfree (dir->fs, dir, buf);
}

long _cdecl
d_create (const char *path)
{
	fcookie dir;
	long r;
	char temp1[PATH_MAX];
	short cur_gid, cur_egid;
	XATTR xattr;
	unsigned mode;

	TRACE(("Dcreate(%s)", path));

	r = path2cookie(path, temp1, &dir);
	if (r) {
		DEBUG(("Dcreate(%s): returning %ld", path, r));
		return r;	/* an error occured */
	}
	
	if (temp1[0] == '\0') {
		DEBUG(("Dcreate(%s): creating a NULL dir?", path));
		release_cookie(&dir);
		return EBADARG;
	}
	
	/* check for write permission on the directory */
	r = dir_access(&dir, S_IWOTH, &mode);
	if (r) {
		DEBUG(("Dcreate(%s): write access to directory denied",path));
		release_cookie(&dir);
		return r;
	}
	
	if (mode & S_ISGID) {
		r = xfs_getxattr (dir.fs, &dir, &xattr);
		if (r) {
			DEBUG(("Dcreate(%s): file system returned %ld", path, r));
		} else {
			cur_gid = curproc->rgid;
			cur_egid = curproc->egid;
			curproc->rgid = curproc->egid = xattr.gid;
			r = xfs_mkdir (dir.fs, &dir, temp1,
					     (DEFAULT_DIRMODE & ~curproc->umask)
					     | S_ISGID);
			curproc->rgid = cur_gid;
			curproc->egid = cur_egid;
		}
	} else
		r = xfs_mkdir (dir.fs, &dir, temp1,
				     DEFAULT_DIRMODE & ~curproc->umask);
	release_cookie(&dir);
	return r;
}

long _cdecl
d_delete(const char *path)
{
	fcookie parentdir, targdir;
	long r;
	PROC *p;
	int i;
	XATTR xattr;
	char temp1[PATH_MAX];
	unsigned mode;

	TRACE(("Ddelete(%s)", path));

	r = path2cookie(path, temp1, &parentdir);

	if (r) {
		DEBUG(("Ddelete(%s): error %lx", path, r));
		release_cookie(&parentdir);
		return r;
	}
/* check for write permission on the directory which the target
 * is located
 */
	if ((r = dir_access(&parentdir, S_IWOTH, &mode)) != E_OK) {
		DEBUG(("Ddelete(%s): access to directory denied", path));
		release_cookie(&parentdir);
		return r;
	}

/* now get the info on the file itself */

	r = relpath2cookie(&parentdir, temp1, NULL, &targdir, 0);
	if (r) {
bailout:
		release_cookie(&parentdir);
		DEBUG(("Ddelete: error %ld on %s", r, path));
		return r;
	}
	if ((r = xfs_getxattr (targdir.fs, &targdir, &xattr)) != E_OK) {
		release_cookie(&targdir);
		goto bailout;
	}

/* check effective uid if sticky bit is set in parent */
	if ((mode & S_ISVTX) && curproc->euid
	    && curproc->euid != xattr.uid) {
		release_cookie(&targdir);
		release_cookie(&parentdir);
		DEBUG(("Ddelete: sticky bit set and not owner"));
		return EACCES;
	}

/* if the "directory" is a symbolic link, really unlink it */
	if ( (xattr.mode & S_IFMT) == S_IFLNK ) {
		release_cookie(&targdir);
		r = xfs_remove (parentdir.fs, &parentdir, temp1);
	} else if ( (xattr.mode & S_IFMT) != S_IFDIR ) {
		DEBUG(("Ddelete: %s is not a directory", path));
		r = ENOTDIR;
	} else {

/* don't delete anyone else's root or current directory */
	    for (p = proclist; p; p = p->gl_next) {
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		for (i = 0; i < NUM_DRIVES; i++) {
			if (samefile(&targdir, &p->root[i])) {
				DEBUG(("Ddelete: directory %s is a root directory",
					path));
				release_cookie(&targdir);
				release_cookie(&parentdir);
				return EACCES;
			} else if (i == p->curdrv && p != curproc &&
				   samefile(&targdir, &p->curdir[i])) {
				DEBUG(("Ddelete: directory %s is in use", path));
				release_cookie(&targdir);
				release_cookie(&parentdir);
				return EACCES;
			}
		}
	    }
	    /* Wait with this until everything has been verified */
	    for (p = proclist; p; p = p->gl_next) {
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		for (i = 0; i < NUM_DRIVES; i++) {
			if (samefile(&targdir, &p->curdir[i])) {
				release_cookie(&p->curdir[i]);
				dup_cookie(&p->curdir[i], &p->root[i]);
			}
		}
	    }
	    release_cookie(&targdir);
	    r = xfs_rmdir (parentdir.fs, &parentdir, temp1);
	}
	release_cookie(&parentdir);
	return r;
}

long _cdecl
d_setpath(const char *path)
{
	XATTR xattr;
	fcookie dir;
	int drv = curproc->curdrv;
	long r;
	
	TRACE (("Dsetpath(%s)", path));
	
	r = path2cookie (path, follow_links, &dir);
	if (r)
	{
		DEBUG (("Dsetpath(%s): returning %ld", path, r));
		return r;
	}
	
	if (path[0] && path[1] == ':')
	{
		char c = *path;
		
		if (c >= 'a' && c <= 'z')
			drv = c-'a';
		else if (c >= 'A' && c <= 'Z'+6)  /* A..Z[\]^_` */
			drv = c-'A';
		else if (c >= '1' && c <= '6')  /* A..Z1..6 */
			drv = c - '1'+26;
	}
	
	r = xfs_getxattr (dir.fs, &dir, &xattr);
	if (r)
	{
		DEBUG (("Dsetpath: file '%s': attributes not found", path));
		release_cookie (&dir);
		return r;
	}
	
	if (!(xattr.attr & FA_DIR))
	{
		DEBUG (("Dsetpath(%s): not a directory", path));
		release_cookie (&dir);
		return ENOTDIR;
	}
	
	if (denyaccess (&xattr, S_IXOTH))
	{
		DEBUG (("Dsetpath(%s): access denied", path));
		release_cookie (&dir);
		return EACCES;
	}
	
	/* watch out for symbolic links; if c:\foo is a link to d:\bar, then
	 * "cd c:\foo" should also change the drive to d:
	 */
	if (drv != UNIDRV && dir.dev != curproc->root[drv].dev)
	{
		int i;
		
		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (curproc->root[i].dev == dir.dev
				&& curproc->root[i].fs == dir.fs)
			{
				if (curproc->curdrv == drv)
					curproc->curdrv = i;
				
				drv = i;
				break;
			}
		}
	}
	
	release_cookie (&curproc->curdir[drv]);
	curproc->curdir[drv] = dir;
	
	return E_OK;
}

/*
 * GEMDOS extension: Dgetcwd(path, drv, size)
 * 
 * like d_getpath, except that the caller provides a limit
 * for the max. number of characters to be put into the buffer.
 * Inspired by POSIX.1, getcwd(), 5.2.2
 * 
 * written by jr
 */

long _cdecl
d_getcwd (char *path, int drv, int size)
{
	FILESYS *fs;
	fcookie *dir, *root;
	long r;
	
	TRACE (("Dgetcwd(%c, %d)", drv + '@', size));
	
	if (drv < 0 || drv > NUM_DRIVES)
		return ENXIO;
	
	drv = (drv == 0) ? curproc->curdrv : drv - 1;
	
	root = &curproc->root[drv];
	if (!root->fs)
	{
		/* maybe not initialized yet? */
		changedrv (drv);
		
		root = &curproc->root[drv];
		if (!root->fs)
			return ENXIO;
	}
	
	fs = root->fs;
	dir = &curproc->curdir[drv];
	
	if (!(fs->fsflags & FS_LONGPATH))
	{
		char buf[PATH_MAX];
		
		r = xfs_getname (fs, root, dir, buf, PATH_MAX);
		if (r) return r;
		
		if (strlen (buf) < size)
		{
			strcpy (path, buf);
		}
		else
			return EBADARG;
	}
	else
		r = xfs_getname (fs, root, dir, path, size);
	
	if (!r && curproc->root_dir)
	{
		if (curproc->curdrv != drv)
		{
			r = ENXIO;
		}
		else
		{
			int len = strlen (curproc->root_dir);
			
			DEBUG (("root_dir detected = %i, (%s), %s", len, path, curproc->root_dir));
			DEBUG (("strncmp = %i", (long) strncmp (curproc->root_dir, path, len)));
			
			if (!strncmp (curproc->root_dir, path, len))
			{
				int i = 0;
				
				while (path [len])
				{
					path [i++] = path [len++];
				}
				
				path [i] = '\0';
			}
		}
	}
	
	return r;
}

long _cdecl
d_getpath(char *path, int drv)
{
	TRACE(("Dgetpath(%c)", drv + '@'));
	return d_getcwd(path, drv, PATH_MAX);
}

long _cdecl
f_setdta(DTABUF *dta)
{
	TRACE(("Fsetdta: %lx", dta));
	curproc->dta = dta;
	curproc->base->p_dta = (char *)dta;
	return E_OK;
}

long _cdecl
f_getdta(void)
{
	long r;

	r = (long)curproc->dta;
	TRACE(("Fgetdta: returning %lx", r));
	return r;
}

/*
 * Fsfirst/next are actually implemented in terms of opendir/readdir/closedir.
 */

long _cdecl
f_sfirst(const char *path, int attrib)
{
	char *s, *slash;
	FILESYS *fs;
	fcookie dir, newdir;
	DTABUF *dta;
	DIR *dirh;
	XATTR xattr;
	long r;
	int i, havelabel;
	char temp1[PATH_MAX];
	unsigned mode;
	
	TRACE(("Fsfirst(%s, %x)", path, attrib));
	
	r = path2cookie(path, temp1, &dir);
	
	if (r) {
		DEBUG(("Fsfirst(%s): path2cookie returned %ld", path, r));
		return r;
	}
	
	/* we need to split the last name (which may be a pattern) off from
	 * the rest of the path, even if FS_KNOPARSE is true
	 */
	slash = 0;
	s = temp1;
	while (*s) {
		if (*s == '\\')
			slash = s;
		s++;
	}
	
	if (slash) {
		*slash++ = 0;	/* slash now points to a name or pattern */
		r = relpath2cookie(&dir, temp1, follow_links, &newdir, 0);
		release_cookie(&dir);
		if (r) {
			DEBUG(("Fsfirst(%s): lookup returned %ld", path, r));
			return r;
		}
		dir = newdir;
	} else {
		slash = temp1;
	}
	
	/* BUG? what if there really is an empty file name?
	 */
	if (!*slash) {
		DEBUG(("Fsfirst: empty pattern"));
		return ENOENT;
	}

	fs = dir.fs;
	dta = curproc->dta;
	
	/* Now, see if we can find a DIR slot for the search. We use the
	 * following heuristics to try to avoid destroying a slot:
	 * (1) if the search doesn't use wildcards, don't bother with a slot
	 * (2) if an existing slot was for the same DTA address, re-use it
	 * (3) if there's a free slot, re-use it. Slots are freed when the
	 *     corresponding search is terminated.
	 */
	
	for (i = 0; i < NUM_SEARCH; i++) {
		if (curproc->srchdta[i] == dta) {
			dirh = &curproc->srchdir[i];
			if (dirh->fc.fs) {
				xfs_closedir (dirh->fc.fs, dirh);
				release_cookie(&dirh->fc);
				dirh->fc.fs = 0;
			}
			curproc->srchdta[i] = 0; /* slot is now free */
		}
	}
	
	/* copy the pattern over into dta_pat into TOS 8.3 form
	 * remember that "slash" now points at the pattern
	 * (it follows the last, if any)
	 */
	copy8_3(dta->dta_pat, slash);
	
	/* if (attrib & FA_LABEL), read the volume label
	 * 
	 * BUG: the label date and time are wrong. ISO/IEC 9293 14.3.3 allows this.
	 * The Desktop set also date and time to 0 when formatting a floppy disk.
	 */
	havelabel = 0;
	if (attrib & FA_LABEL) {
		r = xfs_readlabel (fs, &dir, dta->dta_name, TOS_NAMELEN+1);
		dta->dta_attrib = FA_LABEL;
		dta->dta_time = dta->dta_date = 0;
		dta->dta_size = 0;
		dta->magic = EVALID;
		if (r == E_OK && !pat_match(dta->dta_name, dta->dta_pat))
			r = ENOENT;
		if ((attrib & (FA_DIR|FA_LABEL)) == FA_LABEL)
			return r;
		else if (r == E_OK)
			havelabel = 1;
	}
	
	if (!havelabel && has_wild(slash) == 0) { /* no wild cards in pattern */
		r = relpath2cookie(&dir, slash, follow_links, &newdir, 0);
		if (r == E_OK) {
			r = xfs_getxattr (newdir.fs, &newdir, &xattr);
			release_cookie(&newdir);
		}
		release_cookie(&dir);
		if (r) {
			DEBUG(("Fsfirst(%s): couldn't get file attributes",path));
			return r;
		}
		dta->magic = EVALID;
		dta->dta_attrib = xattr.attr;
		dta->dta_size = xattr.size;
		
		if (fs->fsflags & FS_EXT_3)
		{
			/* UTC -> localtime -> DOS style */
			*((long *) &(dta->dta_time)) = dostime (*((long *) &(xattr.mtime)) - timezone);
		}
		else
		{
			dta->dta_time = xattr.mtime;
			dta->dta_date = xattr.mdate;
		}
		
		strncpy (dta->dta_name, slash, TOS_NAMELEN-1);
		dta->dta_name[TOS_NAMELEN-1] = 0;
		if (curproc->domain == DOM_TOS && !(fs->fsflags & FS_CASESENSITIVE))
			strupr (dta->dta_name);
		
		return E_OK;
	}
	
	/* There is a wild card. Try to find a slot for an opendir/readdir
	 * search. NOTE: we also come here if we were asked to search for
	 * volume labels and found one.
	 */
	for (i = 0; i < NUM_SEARCH; i++) {
		if (curproc->srchdta[i] == 0)
			break;
	}
	if (i == NUM_SEARCH) {
		int oldest = 0; long oldtime = curproc->srchtim[0];

		DEBUG(("Fsfirst(%s): having to re-use a directory slot!",path));
		for (i = 1; i < NUM_SEARCH; i++) {
			if (curproc->srchtim[i] < oldtime) {
				oldest = i;
				oldtime = curproc->srchtim[i];
			}
		}
		/* OK, close this directory for re-use */
		i = oldest;
		dirh = &curproc->srchdir[i];
		if (dirh->fc.fs) {
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
		}
		/* invalidate re-used DTA */
		curproc->srchdta[i]->magic = EVALID;
		curproc->srchdta[i] = 0;
	}
	
	/* check to see if we have read permission on the directory (and make
	 * sure that it really is a directory!)
	 */
	r = dir_access(&dir, S_IROTH, &mode);
	if (r) {
		DEBUG(("Fsfirst(%s): access to directory denied (error code %ld)", path, r));
		release_cookie(&dir);
		return r;
	}
	
	/* set up the directory for a search */
	dirh = &curproc->srchdir[i];
	dirh->fc = dir;
	dirh->index = 0;
	dirh->flags = TOS_SEARCH;
	r = xfs_opendir (dir.fs, dirh, dirh->flags);
	if (r != E_OK) {
		DEBUG(("Fsfirst(%s): couldn't open directory (error %ld)",
			path, r));
		release_cookie(&dir);
		return r;
	}
	
	/* mark the slot as in-use */
	curproc->srchdta[i] = dta;
	
	/* set up the DTA for Fsnext */
	dta->index = i;
	dta->magic = SVALID;
	dta->dta_sattrib = attrib;

	/* OK, now basically just do Fsnext, except that instead of ENMFILES we
	 * return ENOENT.
	 * NOTE: If we already have found a volume label from the search above,
	 * then we skip the f_snext and just return that.
	 */
	if (havelabel)
		return E_OK;
	
	r = f_snext();
	if (r == ENMFILES) r = ENOENT;
	if (r)
		TRACE(("Fsfirst: returning %ld", r));
	
	/* release_cookie isn't necessary, since &dir is now stored in the
	 * DIRH structure and will be released when the search is completed
	 */
	return r;
}

/*
 * Counter for Fsfirst/Fsnext, so that we know which search slots are
 * least recently used. This is updated once per second by the code
 * in timeout.c.
 * BUG: 1/second is pretty low granularity
 */

long searchtime;

long _cdecl
f_snext (void)
{
	char buf[TOS_NAMELEN+1];
	DTABUF *dta = curproc->dta;
	FILESYS *fs;
	fcookie fc;
	ushort i;
	DIR *dirh;
	long r;
	XATTR xattr;
	
	
	TRACE (("Fsnext"));
	
	if (dta->magic == EVALID)
	{
		DEBUG (("Fsnext(%lx): DTA marked a failing search", dta));
		return ENMFILES;
	}
	
	if (dta->magic != SVALID)
	{
		DEBUG (("Fsnext(%lx): dta incorrectly set up", dta));
		return ENOSYS;
	}
	
	i = dta->index;
	if (i >= NUM_SEARCH)
	{
		DEBUG (("Fsnext(%lx): DTA has invalid index", dta));
		return EBADARG;
	}
	
	dirh = &curproc->srchdir[i];
	curproc->srchtim[i] = searchtime;
	
	fs = dirh->fc.fs;
	if (!fs)
	{
		/* oops -- the directory got closed somehow */
		DEBUG (("Fsnext(%lx): invalid filesystem", dta));
		return EINTERNAL;
	}
	
	/* BUG: f_snext and readdir should check for disk media changes
	 */
	for(;;) {
		r = xfs_readdir (fs, dirh, buf, TOS_NAMELEN+1, &fc);

		if (r == EBADARG) {
			DEBUG(("Fsnext: name too long"));
			continue;	/* TOS programs never see these names */
		}
		if (r != E_OK) {
baderror:
			if (dirh->fc.fs)
				(void) xfs_closedir (fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
			curproc->srchdta[i] = 0;
			dta->magic = EVALID;
			if (r != ENMFILES)
				DEBUG(("Fsnext: returning %ld", r));
			return r;
		}
		
		if (!pat_match(buf, dta->dta_pat))
		{
			release_cookie(&fc);
			continue;	/* different patterns */
		}
		
		/* check for search attributes */
		r = xfs_getxattr (fc.fs, &fc, &xattr);
		if (r) {
			DEBUG(("Fsnext: couldn't get file attributes"));
			release_cookie(&fc);
			goto baderror;
		}
		/* if the file is a symbolic link, try to find what it's linked to */
		if ( (xattr.mode & S_IFMT) == S_IFLNK ) {
			char linkedto[PATH_MAX];
			r = xfs_readlink (fc.fs, &fc, linkedto, PATH_MAX);
			release_cookie(&fc);
			if (r == E_OK) {
			/* the "1" tells relpath2cookie that we read a link */
			    r = relpath2cookie(&dirh->fc, linkedto,
					follow_links, &fc, 1);
			    if (r == E_OK) {
				r = xfs_getxattr (fc.fs, &fc, &xattr);
				release_cookie(&fc);
			    }
			}
			if (r) {
				DEBUG(("Fsnext: couldn't follow link: error %ld",
					r));
			}
		} else {
			release_cookie(&fc);
		}
		
		/* silly TOS rules for matching attributes */
		if (xattr.attr == 0) break;
		if (xattr.attr & (FA_CHANGED|FA_RDONLY)) break;
		if (dta->dta_sattrib & xattr.attr)
			break;
	}
	
	/* here, we have a match
	 */
	
	if (fs->fsflags & FS_EXT_3)
	{
		/* UTC -> localtime -> DOS style */
		*((long *) &(dta->dta_time)) = dostime (*((long *) &(xattr.mtime)) - timezone);
	}
	else
	{
		dta->dta_time = xattr.mtime;
		dta->dta_date = xattr.mdate;
	}
	
	dta->dta_attrib = xattr.attr;
	dta->dta_size = xattr.size;
	strcpy (dta->dta_name, buf);
	
	if (curproc->domain == DOM_TOS && !(fs->fsflags & FS_CASESENSITIVE))
		strupr (dta->dta_name);
	
	return E_OK;
}

long _cdecl
f_attrib(const char *name, int rwflag, int attr)
{
	fcookie fc;
	XATTR xattr;
	long r;

	DEBUG(("Fattrib(%s, %d)", name, attr));

	r = path2cookie(name, follow_links, &fc);

	if (r) {
		DEBUG(("Fattrib(%s): error %ld", name, r));
		return r;
	}

	r = xfs_getxattr (fc.fs, &fc, &xattr);

	if (r) {
		DEBUG(("Fattrib(%s): getxattr returned %ld", name, r));
		release_cookie(&fc);
		return r;
	}

	if (rwflag) {
		if (attr & ~(FA_CHANGED|FA_DIR|FA_SYSTEM|FA_HIDDEN|FA_RDONLY)
		    || (attr & FA_DIR) != (xattr.attr & FA_DIR)) {
			DEBUG(("Fattrib(%s): illegal attributes specified",name));
			r = EACCES;
		} else if (curproc->euid && curproc->euid != xattr.uid) {
			DEBUG(("Fattrib(%s): not the file's owner",name));
			r = EACCES;
		} else if (xattr.attr & FA_LABEL) {
			DEBUG(("Fattrib(%s): file is a volume label", name));
			r = EACCES;
		} else {
			r = xfs_chattr (fc.fs, &fc, attr);
		}
		release_cookie(&fc);
		return r;
	} else {
		release_cookie(&fc);
		return xattr.attr;
	}
}

long _cdecl
f_delete(const char *name)
{
	fcookie dir, fc;
	XATTR	xattr;
	long r;
	char temp1[PATH_MAX];
	unsigned mode;

	TRACE(("Fdelete(%s)", name));

/* get a cookie for the directory the file is in */
	if (( r = path2cookie(name, temp1, &dir)) != E_OK)
	{
		DEBUG(("Fdelete: couldn't get directory cookie: error %ld", r));
		return r;
	}

/* check for write permission on directory */
	r = dir_access(&dir, S_IWOTH, &mode);
	if (r) {
		DEBUG(("Fdelete(%s): write access to directory denied",name));
		release_cookie(&dir);
		return EACCES;
	}

/* now get the file attributes */
	if ((r = xfs_lookup (dir.fs, &dir, temp1, &fc)) != E_OK) {
		DEBUG(("Fdelete: error %ld while looking for %s", r, temp1));
		release_cookie(&dir);
		return r;
	}

	if (( r = xfs_getxattr (fc.fs, &fc, &xattr)) < E_OK)
	{
		release_cookie(&dir);
		release_cookie(&fc);
		DEBUG(("Fdelete: couldn't get file attributes: error %ld", r));
		return r;
	}

/* do not allow directories to be deleted */
	if ((xattr.mode & S_IFMT) == S_IFDIR)
	{
		release_cookie(&dir);
		release_cookie(&fc);
		DEBUG(("Fdelete: %s is a directory", name));
		return EISDIR;
	}

/* check effective uid if directories sticky bit is set */
	if ((mode & S_ISVTX) && curproc->euid
	    && curproc->euid != xattr.uid) {
		release_cookie(&dir);
		release_cookie(&fc);
		DEBUG(("Fdelete: sticky bit set and not owner"));
		return EACCES;
	}

/* TOS domain processes can only delete files if they have write permission
 * for them
 */
	if (curproc->domain == DOM_TOS) {
	/* see if we're allowed to kill it */
		if (denyaccess(&xattr, S_IWOTH)) {
			release_cookie(&dir);
			release_cookie(&fc);
			DEBUG(("Fdelete: file access denied"));
			return EACCES;
		}
	}
	release_cookie(&fc);
	r = xfs_remove (dir.fs, &dir,temp1);

	release_cookie(&dir);
	return r;
}

long _cdecl
f_rename(int junk, const char *old, const char *new)
{
	fcookie olddir, newdir, oldfil;
	XATTR xattr;
	char temp1[PATH_MAX], temp2[PATH_MAX];
	long r;
	unsigned mode;

	UNUSED(junk);  /* ignored, for TOS compatibility */

	TRACE(("Frename(%s, %s)", old, new));

	r = path2cookie(old, temp2, &olddir);
	if (r) {
		DEBUG(("Frename(%s,%s): error parsing old name",old,new));
		return r;
	}
/* check for permissions on the old file
 * GEMDOS doesn't allow rename if the file is FA_RDONLY
 * we enforce this restriction only on regular files; processes,
 * directories, and character special files can be renamed at will
 */
	r = relpath2cookie(&olddir, temp2, (char *)0, &oldfil, 0);
	if (r) {
		DEBUG(("Frename(%s,%s): old file not found",old,new));
		release_cookie(&olddir);
		return r;
	}
	r = xfs_getxattr (oldfil.fs, &oldfil, &xattr);
	release_cookie(&oldfil);
	if (r ||
	    ((xattr.mode & S_IFMT) == S_IFREG &&
	     ((xattr.attr & FA_RDONLY)&&curproc->euid&&(curproc->euid!=xattr.uid)) ))
	       /* Only SuperUser and the owner of the file are allowed to rename
		  readonly files */
	{
		DEBUG(("Frename(%s,%s): access to old file not granted",old,new));
		release_cookie(&olddir);
		return EACCES;
	}
	r = path2cookie(new, temp1, &newdir);
	if (r) {
		DEBUG(("Frename(%s,%s): error parsing new name",old,new));
		release_cookie(&olddir);
		return r;
	}

	if (newdir.fs != olddir.fs) {
		DEBUG(("Frename(%s,%s): different file systems",old,new));
		release_cookie(&olddir);
		release_cookie(&newdir);
		return EXDEV;	/* cross device rename */
	}

/* check for write permission on both directories */
	r = dir_access(&olddir, S_IWOTH, &mode);
	if (!r && (mode & S_ISVTX) && curproc->euid
	    && curproc->euid != xattr.uid)
		r = EACCES;
	if (!r) r = dir_access(&newdir, S_IWOTH, &mode);
	if (r) {
		DEBUG(("Frename(%s,%s): access to a directory denied",old,new));
	} else {
		r = xfs_rename (newdir.fs, &olddir, temp2, &newdir, temp1);
	}
	release_cookie(&olddir);
	release_cookie(&newdir);
	return r;
}

/*
 * GEMDOS extension: Dpathconf(name, which)
 * 
 * returns information about filesystem-imposed limits; "name" is the name
 * of a file or directory about which the limit information is requested;
 * "which" is the limit requested, as follows:
 *	-1	max. value of "which" allowed
 *	0	internal limit on open files, if any
 *	1	max. number of links to a file	{LINK_MAX}
 *	2	max. path name length		{PATH_MAX}
 *	3	max. file name length		{NAME_MAX}
 *	4	no. of bytes in atomic write to FIFO {PIPE_BUF}
 *	5	file name truncation rules
 *	6	file name case translation rules
 *
 * unlimited values are returned as 0x7fffffffL
 *
 * see also Sysconf() in dos.c
 */

long _cdecl
d_pathconf(const char *name, int which)
{
	fcookie dir;
	long r;

	r = path2cookie(name, (char *)0, &dir);
	if (r) {
		DEBUG(("Dpathconf(%s): bad path",name));
		return r;
	}
	r = xfs_pathconf (dir.fs, &dir, which);
	if (which == DP_CASE && r == ENOSYS) {
	/* backward compatibility with old .XFS files */
		r = (dir.fs->fsflags & FS_CASESENSITIVE) ? DP_CASESENS :
				DP_CASEINSENS;
	}
	release_cookie(&dir);
	return r;
}

/*
 * GEMDOS extension: Opendir/Readdir/Rewinddir/Closedir
 * 
 * offer a new, POSIX-like alternative to Fsfirst/Fsnext,
 * and as a bonus allow for arbitrary length file names
 */

long _cdecl
d_opendir(const char *name, int flag)
{
	DIR *dirh;
	fcookie dir;
	long r;
	unsigned mode;

	r = path2cookie(name, follow_links, &dir);
	if (r) {
		DEBUG(("Dopendir(%s): error %ld", name, r));
		return r;
	}
	r = dir_access(&dir, S_IROTH, &mode);
	if (r) {
		DEBUG(("Dopendir(%s): read permission denied", name));
		release_cookie(&dir);
		return r;
	}
	
	dirh = kmalloc (sizeof (*dirh));
	if (!dirh) {
		release_cookie(&dir);
		return ENOMEM;
	}

	dirh->fc = dir;
	dirh->index = 0;
	dirh->flags = flag;
	r = xfs_opendir (dir.fs, dirh, flag);
	if (r) {
		DEBUG(("d_opendir(%s): opendir returned %ld", name, r));
		release_cookie(&dir);
		kfree(dirh);
		return r;
	}

/* we keep a chain of open directories so that if a process
 * terminates without closing them all, we can clean up
 */
	dirh->next = curproc->searches;
	curproc->searches = dirh;

	return (long)dirh;
}

long _cdecl
d_readdir(int len, long handle, char *buf)
{
	DIR *dirh = (DIR *)handle;
	fcookie fc;
	long r;

	if (!dirh->fc.fs)
		return EBADF;
	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (r == E_OK)
		release_cookie(&fc);
	return r;
}

/* jr: just as d_readdir, but also returns XATTR structure (not
 * following links). Note that the return value reflects the
 * result of the Dreaddir operation, the result of the Fxattr
 * operation is stored in long *xret
 */

long _cdecl
d_xreaddir(int len, long handle, char *buf, XATTR *xattr, long *xret)
{
	DIR *dirh = (DIR *)handle;
	fcookie fc;
	long r;
	
	if (!dirh->fc.fs)
		return EBADF;
	
	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (r != E_OK)
		return r;
	
	*xret = xfs_getxattr (fc.fs, &fc, xattr);
	if ((*xret == E_OK) && (fc.fs->fsflags & FS_EXT_3))
	{
		/* UTC -> localtime -> DOS style */
		*((long *) &(xattr->mtime)) = dostime (*((long *) &(xattr->mtime)) - timezone);
		*((long *) &(xattr->atime)) = dostime (*((long *) &(xattr->atime)) - timezone);
		*((long *) &(xattr->ctime)) = dostime (*((long *) &(xattr->ctime)) - timezone);
	}
	
	release_cookie (&fc);
	return r;
}


long _cdecl
d_rewind(long handle)
{
	DIR *dirh = (DIR *)handle;

	if (!dirh->fc.fs)
		return EBADF;
	return xfs_rewinddir (dirh->fc.fs, dirh);
}

/*
 * NOTE: there is also code in terminate() in dosmem.c that
 * does automatic closes of directory searches.
 * If you change d_closedir(), you may also need to change
 * terminate().
 */

long _cdecl
d_closedir(long handle)
{
	long r;
	DIR *dirh = (DIR *)handle;
	DIR **where;

	where = &curproc->searches;
	while (*where && *where != dirh) {
		where = &((*where)->next);
	}
	if (!*where) {
		DEBUG(("Dclosedir: not an open directory"));
		return EBADF;
	}

/* unlink the directory from the chain */
	*where = dirh->next;

	if (dirh->fc.fs) {
		r = xfs_closedir (dirh->fc.fs, dirh);
		release_cookie(&dirh->fc);
	} else {
		r = E_OK;
	}

	if (r) {
		DEBUG(("Dclosedir: error %ld", r));
	}
	kfree(dirh);
	return r;
}

/*
 * GEMDOS extension: Fxattr(flag, file, xattr)
 * 
 * gets extended attributes for a file.
 * flag is 0 if symbolic links are to be followed (like stat),
 * flag is 1 if not (like lstat).
 */

long _cdecl
f_xattr (int flag, const char *name, XATTR *xattr)
{
	fcookie fc;
	long r;
	
	TRACE (("Fxattr(%d, %s)", flag, name));
	
	r = path2cookie (name, flag ? NULL : follow_links, &fc);
	if (r)
	{
		DEBUG (("Fxattr(%s): path2cookie returned %ld", name, r));
		return r;
	}
	
	r = xfs_getxattr (fc.fs, &fc, xattr);
	if (r)
	{
		DEBUG (("Fxattr(%s): returning %ld", name, r));
	}
	else if (fc.fs->fsflags & FS_EXT_3)
	{
		/* UTC -> localtime -> DOS style */
		*((long *) &(xattr->mtime)) = dostime (*((long *) &(xattr->mtime)) - timezone);
		*((long *) &(xattr->atime)) = dostime (*((long *) &(xattr->atime)) - timezone);
		*((long *) &(xattr->ctime)) = dostime (*((long *) &(xattr->ctime)) - timezone);
	}
	
	release_cookie (&fc);
	return r;
}

/*
 * GEMDOS extension: Flink(old, new)
 * 
 * creates a hard link named "new" to the file "old".
 */

long _cdecl
f_link(const char *old, const char *new)
{
	fcookie olddir, newdir;
	char temp1[PATH_MAX], temp2[PATH_MAX];
	long r;
	unsigned mode;

	TRACE(("Flink(%s, %s)", old, new));

	r = path2cookie(old, temp2, &olddir);
	if (r) {
		DEBUG(("Flink(%s,%s): error parsing old name",old,new));
		return r;
	}
	r = path2cookie(new, temp1, &newdir);
	if (r) {
		DEBUG(("Flink(%s,%s): error parsing new name",old,new));
		release_cookie(&olddir);
		return r;
	}

	if (newdir.fs != olddir.fs) {
		DEBUG(("Flink(%s,%s): different file systems",old,new));
		release_cookie(&olddir);
		release_cookie(&newdir);
		return EXDEV;	/* cross device link */
	}
	
	/* check for write permission on the destination directory
	 */
	r = dir_access(&newdir, S_IWOTH, &mode);
	if (r) {
		DEBUG(("Flink(%s,%s): access to directory denied",old,new));
	} else
		r = xfs_hardlink (newdir.fs, &olddir, temp2, &newdir, temp1);
	
	release_cookie (&olddir);
	release_cookie (&newdir);
	
	return r;
}

/*
 * GEMDOS extension: Fsymlink(old, new)
 * 
 * create a symbolic link named "new" that contains the path "old"
 */

long _cdecl
f_symlink(const char *old, const char *new)
{
	fcookie newdir;
	long r;
	char temp1[PATH_MAX];
	unsigned mode;

	TRACE(("Fsymlink(%s, %s)", old, new));

	r = path2cookie(new, temp1, &newdir);
	if (r) {
		DEBUG(("Fsymlink(%s,%s): error parsing %s", old,new,new));
		return r;
	}
	r = dir_access(&newdir, S_IWOTH, &mode);
	if (r) {
		DEBUG(("Fsymlink(%s,%s): access to directory denied",old,new));
	} else
		r = xfs_symlink (newdir.fs, &newdir, temp1, old);
	release_cookie(&newdir);
	return r;
}

/*
 * GEMDOS extension: Freadlink(buflen, buf, linkfile)
 * 
 * read the contents of the symbolic link "linkfile" into the buffer
 * "buf", which has length "buflen".
 */

long _cdecl
f_readlink(int buflen, char *buf, const char *linkfile)
{
	fcookie file;
	long r;
	XATTR xattr;

	TRACE(("Freadlink(%s)", linkfile));

	r = path2cookie(linkfile, (char *)0, &file);
	if (r) {
		DEBUG(("Freadlink: unable to find %s", linkfile));
		return r;
	}
	r = xfs_getxattr (file.fs, &file, &xattr);
	if (r) {
		DEBUG(("Freadlink: unable to get attributes for %s", linkfile));
	} else if ( (xattr.mode & S_IFMT) == S_IFLNK )
		r = xfs_readlink (file.fs, &file, buf, buflen);
	else {
		DEBUG(("Freadlink: %s is not a link", linkfile));
		r = EACCES;
	}
	release_cookie(&file);
	return r;
}

/*
 * GEMDOS extension: Dcntl(cmd, path, arg)
 * 
 * do file system specific functions
 */

long _cdecl
d_cntl (int cmd, const char *name, long arg)
{
	fcookie dir;
	long r;
	char temp1[PATH_MAX];
	
	DEBUG (("Dcntl(cmd=%x, file=%s, arg=%lx)", cmd, name, arg));
	
	r = path2cookie (name, temp1, &dir);
	if (r)
	{
		DEBUG (("Dcntl: couldn't find %s", name));
		return r;
	}
	
	switch (cmd)
	{
		case F_GETOPENS:
		{
			struct listopens *l = (void *) arg;
			fcookie object;
			
			r = relpath2cookie (&dir, temp1, NULL, &object, 0);
			if (r == E_OK)
			{
				r = get_opens (&object, l);
				release_cookie (&object);
			}
			
			break;
		}
		case FUTIME:
		{
			if ((dir.fs->fsflags & FS_EXT_3) && arg)
			{
				MUTIMBUF *buf = (MUTIMBUF *) arg;
				ulong t [2];
				
				t [0] = unixtime (buf->actime, buf->acdate) + timezone;
				t [1] = unixtime (buf->modtime, buf->moddate) + timezone;
				
				r = xfs_fscntl (dir.fs, &dir, temp1, cmd, (long) t);
				break;
			}
		}
		default:
		{
			r = xfs_fscntl (dir.fs, &dir, temp1, cmd, arg);
			break;
		}
	}
	
	release_cookie (&dir);
	return r;
}

/*
 * GEMDOS extension: Fchown(name, uid, gid)
 * 
 * changes the user and group ownerships of a file to "uid" and "gid"
 * respectively
 */

long _cdecl
f_chown(const char *name, int uid, int gid)
{
	fcookie fc;
	XATTR xattr;
	long r;

	TRACE(("Fchown(%s, %d, %d)", name, uid, gid));

	r = path2cookie(name, NULL, &fc);
	if (r) {
		DEBUG(("Fchown(%s): error %ld", name, r));
		return r;
	}

/* MiNT acts like _POSIX_CHOWN_RESTRICTED: a non-privileged process can
 * only change the ownership of a file that is owned by this user, to
 * the effective group id of the process or one of its supplementary groups
 */
	if (curproc->euid) {
		if (curproc->egid != gid && !ngroupmatch(gid))
			r = EACCES;
		else
			r = xfs_getxattr (fc.fs, &fc, &xattr);
		if (r) {
			DEBUG(("Fchown(%s): unable to get file attributes",name));
			release_cookie(&fc);
			return r;
		}
		if (xattr.uid != curproc->euid || xattr.uid != uid) {
			DEBUG(("Fchown(%s): not the file's owner",name));
			release_cookie(&fc);
			return EACCES;
		}
		r = xfs_chown (fc.fs, &fc, uid, gid);

	/* POSIX 5.6.5.2: if name refers to a regular file the set-user-ID and
	 * set-group-ID bits of the file mode shall be cleared upon successful
	 * return from the call to chown, unless the call is made by a process
	 * with the appropriate privileges.
	 * Note that POSIX leaves the behaviour unspecified for all other file
	 * types. At least for directories with BSD-like setgid semantics,
	 * these bits should be left unchanged.
	 */
		if (!r && (xattr.mode & S_IFMT) != S_IFDIR
		    && (xattr.mode & (S_ISUID | S_ISGID))) {
			long s;

			s = xfs_chmode (fc.fs, &fc, xattr.mode & ~(S_ISUID | S_ISGID));
			if (!s)
				DEBUG(("Fchown: chmode returned %ld (ignored)",
				       s));
		}
	}
	else
		r = xfs_chown (fc.fs, &fc, uid, gid);

	release_cookie(&fc);
	return r;
}

/*
 * GEMDOS extension: Fchmod (file, mode)
 * 
 * changes a file's access permissions.
 */

long _cdecl
f_chmod (const char *name, unsigned int mode)
{
	fcookie fc;
	long r;
	XATTR xattr;
	
	TRACE (("Fchmod(%s, %o)", name, mode));
	r = path2cookie (name, follow_links, &fc);
	if (r)
	{
		DEBUG (("Fchmod(%s): error %ld", name, r));
		return r;
	}
	r = xfs_getxattr (fc.fs, &fc, &xattr);
	if (r)
	{
		DEBUG (("Fchmod(%s): couldn't get file attributes",name));
	}
	else if (curproc->euid && curproc->euid != xattr.uid)
	{
		DEBUG (("Fchmod(%s): not the file's owner",name));
		r = EACCES;
	}
	else
	{
		r = xfs_chmode (fc.fs, &fc, mode & ~S_IFMT);
		if (r) DEBUG(("Fchmod: error %ld", r));
	}
	release_cookie (&fc);
	return r;
}

/*
 * GEMDOS extension: Dlock(mode, dev)
 * 
 * locks or unlocks access to a BIOS device
 * "mode" bit 0 is 0 for unlock, 1 for lock; "dev" is a
 * BIOS device (0 for A:, 1 for B:, etc.).
 * 
 * Returns:
 *   E_OK    if the operation was successful
 *   EACCES  if a lock attempt is made on a drive that is being used
 *   ELOCKED if the drive is locked by another process
 *   ENSLOCK if a program attempts to unlock a drive it hasn't locked.
 * 
 * ++jr: if mode bit 1 is set, then instead of returning ELOCKED the
 * pid of the process which has locked the drive is returned (unless
 * it was locked by pid 0, in which case ELOCKED is still returned).
 */

PROC *dlockproc [NUM_DRIVES];

long _cdecl
d_lock (int mode, int _dev)
{
	PROC *p;
	FILEPTR *f;
	int i;
	ushort dev = _dev;
	
	TRACE (("Dlock (%x,%c:)", mode, dev+'A'));
	
	/* check for alias drives */
	if (dev < NUM_DRIVES && aliasdrv[dev])
		dev = aliasdrv[dev] - 1;
	
	/* range check */
	if (dev >= NUM_DRIVES)
		return ENXIO;
	
	if ((mode & 1) == 0)	/* unlock */
	{
		if (dlockproc[dev] == curproc)
		{
			dlockproc[dev] = 0;
			/* changedrv (dev); */
			return E_OK;
		}
		DEBUG (("Dlock: no such lock"));
		return ENSLOCK;
	}
	
	/* code for locking
	 */
	
	/* is the drive already locked? */
	if (dlockproc[dev])
	{
		DEBUG(("Dlock: drive already locked"));
		
		if ((mode & 2) && (dlockproc[dev]->pid != 0))
			return dlockproc[dev]->pid;
		
		return ELOCKED;
	}
	
	/* see if the drive is in use */
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		
		if (p->root_dir && p->root_fc.dev == dev)
			return EACCES;
		
		for (i = MIN_HANDLE; i < MAX_OPEN; i++)
		{
			if (((f = p->handle[i]) != 0)
				&& (f != (FILEPTR *) 1)
				&& (f->fc.dev == dev))
			{
				DEBUG (("Dlock: process %d (%s) has an open handle on the drive", p->pid, p->name));
				
				if ((mode & 2) && (p->pid != 0))
					return p->pid;
				
				return EACCES;
			}
		}
	}
	
	/* if we reach here, the drive is not in use
	 *
	 * we lock it by setting dlockproc and by setting all root and current
	 * directories referring to the device to a null file system
	 */
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		
		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (p->root[i].dev == dev)
			{
				release_cookie (&p->root[i]);
				p->root[i].fs = 0;
			}
			if (p->curdir[i].dev == dev)
			{
				release_cookie (&p->curdir[i]);
				p->curdir[i].fs = 0;
			}
		}
	}
	
	/* always syncing the filesystems before locking */
	{
		FILESYS *fs = drives [dev];
		
		if (fs)
		{
			if (fs->fsflags & FS_EXT_1)
			{
				DEBUG (("Unmounting %c: ...", 'A'+dev));
				(void) xfs_unmount (fs, dev);
			}
			else
			{
				s_ync ();
				
				DEBUG (("Invalidate %c: ...", 'A'+dev));
				(void) xfs_dskchng (fs, dev, 1);
			}
			
			drives [dev] = NULL;
		}
	}
	
	dlockproc[dev] = curproc;
	return E_OK;
}

/*
 * GEMDOS-extension: Dreadlabel(path, buf, buflen)
 * 
 * original written by jr
 */

long _cdecl
d_readlabel (const char *name, char *buf, int buflen)
{
	fcookie dir;
	long r;
	
	r = path2cookie (name, NULL, &dir);
	if (r)
	{
		DEBUG (("Dreadlabel(%s): bad path", name));
		return r;
	}
	
	r = xfs_readlabel (dir.fs, &dir, buf, buflen);
	
	release_cookie (&dir);
	return r;
}

/*
 * GEMDOS-extension: Dwritelabel(path, newlabel)
 * 
 * original written by jr
 */

long _cdecl
d_writelabel (const char *name, const char *label)
{
	fcookie dir;
	long r;
	
	/* Draco: in secure mode only superuser can write labels
	 */
	if (secure_mode && (curproc->euid))
	{
		DEBUG (("Dwritelabel(%s): access denied", name));
		return EACCES;
	}
	
	r = path2cookie (name, NULL, &dir);
	if (r)
	{
		DEBUG (("Dwritelabel(%s): bad path",name));
		return r;
	}
	
	r = xfs_writelabel (dir.fs, &dir, label);
	
	release_cookie (&dir);
	return r;
}

/*
 * GEMDOS-extensions: Dchroot(path)
 * 
 * original written by fn
 */

long _cdecl
d_chroot (const char *path)
{
	XATTR xattr;
	fcookie dir;
	long r;
	
	DEBUG (("Dchroot(%s): enter", path));
	
	if (curproc->euid)
	{
		DEBUG (("Dchroot(%s): access denied", path));
		return EPERM;
	}
	
	r = path2cookie (path, follow_links, &dir);
	if (r)
	{
		DEBUG (("Dchroot(%s): bad path -> %li", path, r));
		return r;
	}
	
	r = xfs_getxattr (dir.fs, &dir, &xattr);
	if (r)
	{
		DEBUG (("Dchroot(%s): attributes not found -> %li", path, r));
		goto error;
	}
	
	if (!(xattr.attr & FA_DIR))
	{
		DEBUG (("Dchroot(%s): not a directory", path));
		r = ENOTDIR;
		goto error;
	}
	
	{
		char buf[PATH_MAX];
		
		r = xfs_getname (dir.fs, &curproc->root[dir.dev], &dir, buf, PATH_MAX);
		if (r)
		{
			DEBUG( ("Dchroot(%s): getname fail!", path));
			goto error;
		}
		
		curproc->root_dir = kmalloc (strlen (buf) + 1);
		if (!curproc->root_dir)
		{
			DEBUG (("Dchroot(%s): kmalloc fail!", buf));
			r = ENOMEM;
			goto error;
		}
		
		strcpy (curproc->root_dir, buf);
	}
	
	curproc->root_fc = dir;
	
	/*
	curproc->curdrv = dir.dev;
	
	release_cookie(&curproc->curdir[curproc->curdrv]);
	dup_cookie(&curproc->curdir[curproc->curdrv],&curproc->root_fc);
	*/
	
	DEBUG (("Dchroot(%s): ok [%lx,%i]", curproc->root_dir, dir.index, dir.dev));
	DEBUG (("Dchroot: [%lx,%lx]", curproc->curdir[dir.dev].index, curproc->curdir[dir.dev].fs));
	DEBUG (("Dchroot: [%lx,%lx]", curproc->root[dir.dev].index, curproc->root[dir.dev].fs));
	return E_OK;
	
error:
	release_cookie (&dir);
	return r;
}

/*
 * GEMDOS extension: Fstat(flag, file, stat)
 * 
 * gets extended attributes in native UNIX format for a file.
 * flag is 0 if symbolic links are to be followed (like stat),
 * flag is 1 if not (like lstat).
 */

long _cdecl
f_stat64 (int flag, const char *name, STAT *stat)
{
	fcookie fc;
	long r;
	
	TRACE (("Fstat64(%d, %s)", flag, name));
	
	r = path2cookie (name, flag ? NULL : follow_links, &fc);
	if (r)
	{
		DEBUG (("Fstat64(%s): path2cookie returned %ld", name, r));
		return r;
	}
	
	if (fc.fs->fsflags & FS_EXT_3)
	{
		r = xfs_stat64 (fc.fs, &fc, stat);
		if (r)
			DEBUG (("Fstat64(%s): returning %ld", name, r));
	}
	else
		r = EINVAL;
	
	release_cookie (&fc);
	return r;
}
