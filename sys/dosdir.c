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

/* DOS directory functions */

# include "dosdir.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "bios.h"
# include "filesys.h"
# include "k_fds.h"
# include "k_prot.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "proc.h"
# include "time.h"
# include "unifs.h"


/* change to a new drive: should always return a map of valid drives
 */
long _cdecl
sys_d_setdrv (int d)
{
	struct proc *p = get_curproc();
	long r;
	
	r = sys_b_drvmap() | dosdrvs ;
	TRACELOW (("Dsetdrv(%d)", d));
	assert (p->p_fd && p->p_cwd);

	if (d < 0 || d >= NUM_DRIVES
		|| (r & (1L << d)) == 0
		|| p->p_cwd->root_dir)
	{
		DEBUG (("Dsetdrv: invalid drive %d", d));
		return r;
	}

	p->p_mem->base->p_defdrv = p->p_cwd->curdrv = d;
	return r;
}


long _cdecl
sys_d_getdrv (void)
{
	struct proc *p = get_curproc();

	TRACELOW (("Dgetdrv"));
	assert (p->p_fd && p->p_cwd);

	return p->p_cwd->curdrv;
}

long _cdecl
sys_d_free (long *buf, int d)
{
	struct proc *p = get_curproc();
	fcookie *dir = 0;
	FILESYS *fs;
	fcookie root;
	long r;

	TRACE(("Dfree(%d)", d));
	assert (p->p_fd && p->p_cwd);

	/* drive 0 means current drive, otherwise it's d-1 */
	if (d)
		d = d - 1;
	else
		d = p->p_cwd->curdrv;

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
	dir = &p->p_cwd->curdir[d];
	if (!dir->fs)
	{
		DEBUG(("Dfree: bad drive"));
		return ENXIO;
	}

	return xfs_dfree (dir->fs, dir, buf);
}

long _cdecl
sys_d_create (const char *path)
{
	struct proc *p = get_curproc();

	fcookie dir;
	long r;
	char temp1[PATH_MAX];
	XATTR xattr;
	ushort mode;

	TRACE(("Dcreate(%s)", path));
	assert (p->p_fd && p->p_cwd);

	r = path2cookie(p, path, temp1, &dir);
	if (r)
	{
		DEBUG(("Dcreate(%s): returning %ld", path, r));
		return r;
	}

	if (temp1[0] == '\0')
	{
		DEBUG(("Dcreate(%s): creating a NULL dir?", path));
		release_cookie(&dir);
		return EBADARG;
	}

	/* check for write permission on the directory */
	r = dir_access(p->p_cred->ucr, &dir, S_IWOTH, &mode);
	if (r)
	{
		DEBUG(("Dcreate(%s): write access to directory denied",path));
		release_cookie(&dir);
		return r;
	}

	if (mode & S_ISGID)
	{
		r = xfs_getxattr (dir.fs, &dir, &xattr);
		if (r)
		{
			DEBUG(("Dcreate(%s): file system returned %ld", path, r));
		}
		else
		{
			ushort cur_gid = p->p_cred->rgid;
			ushort cur_egid = p->p_cred->ucr->egid;
			p->p_cred->rgid = p->p_cred->ucr->egid = xattr.gid;
			r = xfs_mkdir (dir.fs, &dir, temp1,
					     (DEFAULT_DIRMODE & ~p->p_cwd->cmask)
					     | S_ISGID);
			p->p_cred->rgid = cur_gid;
			p->p_cred->ucr->egid = cur_egid;
		}
	}
	else
		r = xfs_mkdir (dir.fs, &dir, temp1,
				     DEFAULT_DIRMODE & ~p->p_cwd->cmask);
	release_cookie(&dir);
	return r;
}

long _cdecl
sys_d_delete (const char *path)
{
	struct proc *cp = get_curproc();
	struct ucred *cred = cp->p_cred->ucr;

	fcookie parentdir, targdir;
	long r;
	int i;
	XATTR xattr;
	char temp1[PATH_MAX];
	ushort mode;


	TRACE(("Ddelete(%s)", path));

	r = path2cookie (cp, path, temp1, &parentdir);
	if (r)
	{
		DEBUG(("Ddelete(%s): error %lx", path, r));
		release_cookie(&parentdir);
		return r;
	}

	/* check for write permission on the directory which the target
	 * is located
	 */
	r = dir_access (cred, &parentdir, S_IWOTH, &mode);
	if (r)
	{
		DEBUG(("Ddelete(%s): access to directory denied", path));
		release_cookie (&parentdir);
		return r;
	}

	/* now get the info on the file itself */
	r = relpath2cookie (cp, &parentdir, temp1, NULL, &targdir, 0);
	if (r)
	{
bailout:
		release_cookie (&parentdir);
		DEBUG(("Ddelete: error %ld on %s", r, path));
		return r;
	}

	r = xfs_getxattr (targdir.fs, &targdir, &xattr);
	if (r)
	{
		release_cookie (&targdir);
		goto bailout;
	}

	/* check effective uid if sticky bit is set in parent */
	if ((mode & S_ISVTX) && cred->euid
	    && cred->euid != xattr.uid)
	{
		release_cookie (&targdir);
		release_cookie (&parentdir);
		DEBUG(("Ddelete: sticky bit set and not owner"));
		return EACCES;
	}

	/* if the "directory" is a symbolic link, really unlink it */
	if (S_ISLNK(xattr.mode))
	{
		release_cookie (&targdir);
		r = xfs_remove (parentdir.fs, &parentdir, temp1);
	}
	else if (!S_ISDIR(xattr.mode))
	{
		DEBUG(("Ddelete: %s is not a directory", path));
		r = ENOTDIR;
	}
	else if (strlen (temp1) == 1 && temp1[0] == '.')
	{
		DEBUG(("Ddelete: %s is not allowed to be deleted", path));
		r = EINVAL;
	}
	else
	{
		struct proc *p;
		
		/* don't delete anyone else's root or current directory */
		for (p = proclist; p; p = p->gl_next)
		{
			struct cwd *cwd = p->p_cwd;

			if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
				continue;

			assert (cwd);

			for (i = 0; i < NUM_DRIVES; i++)
			{
				if (samefile (&targdir, &cwd->root[i]))
				{
					DEBUG(("Ddelete: directory %s is a root directory", path));
					release_cookie (&targdir);
					release_cookie (&parentdir);
					return EACCES;
				}
				else if (i == cwd->curdrv && p != cp && samefile (&targdir, &cwd->curdir[i]))
				{
					DEBUG(("Ddelete: directory %s is in use", path));
					release_cookie (&targdir);
					release_cookie (&parentdir);
					return EACCES;
				}
			}
		}

		/* Wait with this until everything has been verified */
		for (p = proclist; p; p = p->gl_next)
		{
			struct cwd *cwd = p->p_cwd;

			if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
				continue;

			assert (cwd);

			for (i = 0; i < NUM_DRIVES; i++)
			{
				if (samefile (&targdir, &cwd->curdir[i]))
				{
					release_cookie (&cwd->curdir[i]);
					dup_cookie (&cwd->curdir[i], &cwd->root[i]);
				}
			}
		}

		release_cookie (&targdir);
		r = xfs_rmdir (parentdir.fs, &parentdir, temp1);
	}

	release_cookie (&parentdir);
	return r;
}

long
sys_d_setpath0 (struct proc *p, const char *path)
{
	struct cwd *cwd = p->p_cwd;

	XATTR xattr;
	fcookie dir;
	int drv;
	long r;

	TRACE (("Dsetpath(%s)", path));
	assert (cwd);

	r = path2cookie (p, path, follow_links, &dir);
	if (r)
	{
		DEBUG (("Dsetpath(%s): returning %ld", path, r));
		return r;
	}

	if (path[0] && path[1] == ':')
	{
		char c = *path;

		/* also accept the result of adding 26 to 'A'; some buggy programs use this */
		if (c >= 'A' + 26 && c < 'A' + NUM_DRIVES)  /* A..Z[\]^_` */
			drv = c - 'A';
		else
			drv = DriveFromLetter(c);
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

	if (denyaccess (p->p_cred->ucr, &xattr, S_IXOTH))
	{
		DEBUG (("Dsetpath(%s): access denied", path));
		release_cookie (&dir);
		return EACCES;
	}

	/* watch out for symbolic links; if c:\foo is a link to d:\bar, then
	 * "cd c:\foo" should also change the drive to d:
	 */
	drv = cwd->curdrv;
	if (drv != UNIDRV && dir.dev != cwd->root[drv].dev)
	{
		int i;

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (cwd->root[i].dev == dir.dev
				&& cwd->root[i].fs == dir.fs)
			{
				if (cwd->curdrv == drv)
					cwd->curdrv = i;

				drv = i;
				break;
			}
		}
	}

	release_cookie (&cwd->curdir[drv]);
	cwd->curdir[drv] = dir;

	return E_OK;
}

long _cdecl
sys_d_setpath (const char *path)
{
	return sys_d_setpath0(get_curproc(), path);
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
sys_d_getcwd (char *path, int drv, int size)
{
	struct proc *p = get_curproc();
	struct cwd *cwd = p->p_cwd;

	FILESYS *fs;
	fcookie *dir, *root;
	long r;


	TRACE (("Dgetcwd(%c, %d)", drv + '@', size));
	assert (cwd);

	if (drv < 0 || drv > NUM_DRIVES)
		return ENXIO;

	drv = (drv == 0) ? cwd->curdrv : drv - 1;

	root = &cwd->root[drv];
	if (!root->fs)
	{
		/* maybe not initialized yet? */
		changedrv (drv);

		root = &cwd->root[drv];
		if (!root->fs)
			return ENXIO;
	}

	fs = root->fs;
	dir = &cwd->curdir[drv];

	if (!(fs->fsflags & FS_LONGPATH))
	{
		char buf[PATH_MAX];

		r = xfs_getname (fs, root, dir, buf, PATH_MAX);
		if (r) return r;

		if (strlen (buf) < size)
			strcpy (path, buf);
		else
			return EBADARG;
	}
	else
		r = xfs_getname (fs, root, dir, path, size);

	if (!r && cwd->root_dir)
	{
		if (cwd->curdrv != drv)
		{
			r = ENXIO;
		}
		else
		{
			int len = strlen (cwd->root_dir);

			DEBUG (("root_dir detected = %i, (%s), %s", len, path, cwd->root_dir));
			DEBUG (("strncmp = %i", (int)strncmp (cwd->root_dir, path, len)));

			if (!strncmp (cwd->root_dir, path, len))
			{
				int i = 0;

				while (path [len])
					path [i++] = path [len++];

				path [i] = '\0';
			}
		}
	}

	return r;
}

long _cdecl
sys_d_getpath (char *path, int drv)
{
	TRACE(("Dgetpath(%c)", drv + '@'));
	return sys_d_getcwd (path, drv, PATH_MAX);
}

long _cdecl
sys_f_setdta (DTABUF *dta)
{
	struct proc *p = get_curproc();

	TRACE(("Fsetdta: %p", dta));
	p->p_fd->dta = dta;
	p->p_mem->base->p_dta = (char *) dta;

	return E_OK;
}

DTABUF *_cdecl
sys_f_getdta (void)
{
	struct proc *p = get_curproc();
	DTABUF *r;

	r = p->p_fd->dta;

	TRACE(("Fgetdta: returning %p", r));
	return r;
}

/*
 * Fsfirst/next are actually implemented in terms of opendir/readdir/closedir.
 */
long _cdecl
sys_f_sfirst (const char *path, int attrib)
{
	struct proc *p = get_curproc();

	char *s, *slash;
	FILESYS *fs;
	fcookie dir, newdir;
	DTABUF *dta;
	DIR *dirh;
	XATTR xattr;
	long r;
	int i, havelabel;
	char temp1[PATH_MAX];
	ushort mode;


	TRACE(("Fsfirst(%s, %x)", path, attrib));

	r = path2cookie (p, path, temp1, &dir);
	if (r)
	{
		DEBUG(("Fsfirst(%s): path2cookie returned %ld", path, r));
		return r;
	}

	/* we need to split the last name (which may be a pattern) off from
	 * the rest of the path, even if FS_KNOPARSE is true
	 */
	slash = 0;
	s = temp1;
	while (*s)
	{
		if (*s == '\\' || *s == '/')
			slash = s;
		s++;
	}

	if (slash)
	{
		*slash++ = 0;	/* slash now points to a name or pattern */
		r = relpath2cookie (p, &dir, temp1, follow_links, &newdir, 0);
		release_cookie (&dir);
		if (r)
		{
			DEBUG(("Fsfirst(%s): lookup returned %ld", path, r));
			return r;
		}
		dir = newdir;
	}
	else
		slash = temp1;

	/* BUG? what if there really is an empty file name?
	 */
	if (!*slash)
	{
		DEBUG(("Fsfirst: empty pattern"));
		return ENOENT;
	}

	fs = dir.fs;
	dta = p->p_fd->dta;

	/* Now, see if we can find a DIR slot for the search. We use the
	 * following heuristics to try to avoid destroying a slot:
	 * (1) if the search doesn't use wildcards, don't bother with a slot
	 * (2) if an existing slot was for the same DTA address, re-use it
	 * (3) if there's a free slot, re-use it. Slots are freed when the
	 *     corresponding search is terminated.
	 */

	for (i = 0; i < NUM_SEARCH; i++)
	{
		if (p->p_fd->srchdta[i] == dta)
		{
			dirh = &p->p_fd->srchdir[i];
			if (dirh->fc.fs)
			{
				xfs_closedir (dirh->fc.fs, dirh);
				release_cookie(&dirh->fc);
				dirh->fc.fs = 0;
			}
			p->p_fd->srchdta[i] = 0; /* slot is now free */
		}
	}

	/* copy the pattern over into dta_pat into TOS 8.3 form
	 * remember that "slash" now points at the pattern
	 * (it follows the last, if any)
	 */
	copy8_3 (dta->dta_pat, slash);

	/* if (attrib & FA_LABEL), read the volume label
	 *
	 * BUG: the label date and time are wrong. ISO/IEC 9293 14.3.3 allows this.
	 * The Desktop set also date and time to 0 when formatting a floppy disk.
	 */
	havelabel = 0;
	if (attrib & FA_LABEL)
	{
		r = xfs_readlabel (fs, &dir, dta->dta_name, TOS_NAMELEN+1);
		dta->dta_attrib = FA_LABEL;
		dta->dta_time = dta->dta_date = 0;
		dta->dta_size = 0;
		dta->magic = EVALID;
		if (r == E_OK && !pat_match (dta->dta_name, dta->dta_pat))
			r = ENOENT;
		if ((attrib & (FA_DIR|FA_LABEL)) == FA_LABEL)
			return r;
		else if (r == E_OK)
			havelabel = 1;
	}

	if (!havelabel && has_wild (slash) == 0)
	{
		/* no wild cards in pattern */
		r = relpath2cookie (p, &dir, slash, follow_links, &newdir, 0);
		if (r == E_OK)
		{
			r = xfs_getxattr (newdir.fs, &newdir, &xattr);
			release_cookie (&newdir);
		}
		release_cookie (&dir);
		if (r)
		{
			DEBUG(("Fsfirst(%s): couldn't get file attributes",path));
			return r;
		}

		dta->magic = EVALID;
		dta->dta_attrib = xattr.attr;
		dta->dta_size = xattr.size;

		if (fs->fsflags & FS_EXT_3)
		{
			dta_UTC_local_dos(dta,xattr,m);
		}
		else
		{
			dta->dta_time = xattr.mtime;
			dta->dta_date = xattr.mdate;
		}

		strncpy (dta->dta_name, slash, TOS_NAMELEN-1);
		dta->dta_name[TOS_NAMELEN-1] = 0;
		if (p->domain == DOM_TOS && !(fs->fsflags & FS_CASESENSITIVE))
			strupr (dta->dta_name);

		return E_OK;
	}

	/* There is a wild card. Try to find a slot for an opendir/readdir
	 * search. NOTE: we also come here if we were asked to search for
	 * volume labels and found one.
	 */
	for (i = 0; i < NUM_SEARCH; i++)
	{
		if (p->p_fd->srchdta[i] == 0)
			break;
	}

	if (i == NUM_SEARCH)
	{
		int oldest = 0;
		long oldtime = p->p_fd->srchtim[0];

		DEBUG(("Fsfirst(%s): having to re-use a directory slot!", path));
		for (i = 1; i < NUM_SEARCH; i++)
		{
			if (p->p_fd->srchtim[i] < oldtime)
			{
				oldest = i;
				oldtime = p->p_fd->srchtim[i];
			}
		}

		/* OK, close this directory for re-use */
		i = oldest;
		dirh = &p->p_fd->srchdir[i];
		if (dirh->fc.fs)
		{
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
		}

		/* invalidate re-used DTA */
		p->p_fd->srchdta[i]->magic = EVALID;
		p->p_fd->srchdta[i] = 0;
	}

	/* check to see if we have read permission on the directory (and make
	 * sure that it really is a directory!)
	 */
	r = dir_access(p->p_cred->ucr, &dir, S_IROTH, &mode);
	if (r)
	{
		DEBUG(("Fsfirst(%s): access to directory denied (error code %ld)", path, r));
		release_cookie(&dir);
		return r;
	}

	/* set up the directory for a search */
	dirh = &p->p_fd->srchdir[i];
	dirh->fc = dir;
	dirh->index = 0;
	dirh->flags = TOS_SEARCH;

	r = xfs_opendir (dir.fs, dirh, dirh->flags);
	if (r != E_OK)
	{
		DEBUG(("Fsfirst(%s): couldn't open directory (error %ld)", path, r));
		release_cookie(&dir);
		return r;
	}

	/* mark the slot as in-use */
	p->p_fd->srchdta[i] = dta;

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

	r = sys_f_snext();
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
sys_f_snext (void)
{
	struct proc *p = get_curproc();

	char buf[TOS_NAMELEN+1];
	DTABUF *dta = p->p_fd->dta;
	FILESYS *fs;
	fcookie fc;
	ushort i;
	DIR *dirh;
	long r;
	XATTR xattr;


	TRACE (("Fsnext"));

	if (dta->magic == EVALID)
	{
		DEBUG (("Fsnext(%p): DTA marked a failing search", dta));
		return ENMFILES;
	}

	if (dta->magic != SVALID)
	{
		DEBUG (("Fsnext(%p): dta incorrectly set up", dta));
		return ENOSYS;
	}

	i = dta->index;
	if (i >= NUM_SEARCH)
	{
		DEBUG (("Fsnext(%p): DTA has invalid index", dta));
		return EBADARG;
	}

	dirh = &p->p_fd->srchdir[i];
	p->p_fd->srchtim[i] = searchtime;

	fs = dirh->fc.fs;
	if (!fs)
	{
		/* oops -- the directory got closed somehow */
		DEBUG (("Fsnext(%p): invalid filesystem", dta));
		return EINTERNAL;
	}

	/* BUG: f_snext and readdir should check for disk media changes
	 */
	for(;;)
	{
		r = xfs_readdir (fs, dirh, buf, TOS_NAMELEN+1, &fc);

		if (r == EBADARG)
		{
			DEBUG(("Fsnext: name too long"));
			continue;	/* TOS programs never see these names */
		}

		if (r != E_OK)
		{
baderror:
			if (dirh->fc.fs)
				(void) xfs_closedir (fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
			p->p_fd->srchdta[i] = 0;
			dta->magic = EVALID;
			if (r != ENMFILES)
				DEBUG(("Fsnext: returning %ld", r));
			return r;
		}

		if (!pat_match (buf, dta->dta_pat))
		{
			release_cookie (&fc);
			continue;	/* different patterns */
		}

		/* check for search attributes */
		r = xfs_getxattr (fc.fs, &fc, &xattr);
		if (r)
		{
			DEBUG(("Fsnext: couldn't get file attributes"));
			release_cookie (&fc);
			goto baderror;
		}

		/* if the file is a symbolic link, try to find what it's linked to */
		if (S_ISLNK(xattr.mode))
		{
			char linkedto[PATH_MAX];
			r = xfs_readlink (fc.fs, &fc, linkedto, PATH_MAX);
			release_cookie (&fc);
			if (r == E_OK)
			{
				/* the "1" tells relpath2cookie that we read a link */
				r = relpath2cookie (p, &dirh->fc, linkedto,
					follow_links, &fc, 1);
				if (r == E_OK)
				{
					r = xfs_getxattr (fc.fs, &fc, &xattr);
					release_cookie (&fc);
				}
			}
			if (r)
				DEBUG(("Fsnext: couldn't follow link: error %ld", r));
		}
		else
			release_cookie (&fc);

		/* silly TOS rules for matching attributes */
		if (xattr.attr == 0)
			break;

		if (xattr.attr & (FA_CHANGED|FA_RDONLY))
			break;

		if (dta->dta_sattrib & xattr.attr)
			break;
	}

	/* here, we have a match
	 */

	if (fs->fsflags & FS_EXT_3)
	{
		dta_UTC_local_dos(dta,xattr,m);
	}
	else
	{
		dta->dta_time = xattr.mtime;
		dta->dta_date = xattr.mdate;
	}

	dta->dta_attrib = xattr.attr;
	dta->dta_size = xattr.size;
	strcpy (dta->dta_name, buf);

	if (p->domain == DOM_TOS && !(fs->fsflags & FS_CASESENSITIVE))
		strupr (dta->dta_name);

	return E_OK;
}

long _cdecl
sys_f_attrib (const char *name, int rwflag, int attr)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie fc;
	XATTR xattr;
	long r;


	DEBUG(("Fattrib(%s, %d)", name, attr));

	r = path2cookie (p, name, follow_links, &fc);
	if (r)
	{
		DEBUG(("Fattrib(%s): error %ld", name, r));
		return r;
	}

	r = xfs_getxattr (fc.fs, &fc, &xattr);
	if (r)
	{
		DEBUG(("Fattrib(%s): getxattr returned %ld", name, r));
		release_cookie (&fc);
		return r;
	}

	if (rwflag)
	{
		if (attr & ~(FA_CHANGED|FA_DIR|FA_SYSTEM|FA_HIDDEN|FA_RDONLY)
		    || (attr & FA_DIR) != (xattr.attr & FA_DIR))
		{
			DEBUG(("Fattrib(%s): illegal attributes specified",name));
			r = EACCES;
		}
		else if (cred->euid && cred->euid != xattr.uid)
		{
			DEBUG(("Fattrib(%s): not the file's owner",name));
			r = EACCES;
		}
		else if (xattr.attr & FA_LABEL)
		{
			DEBUG(("Fattrib(%s): file is a volume label", name));
			r = EACCES;
		}
		else
			r = xfs_chattr (fc.fs, &fc, attr);

		release_cookie (&fc);
		return r;
	}
	else
	{
		release_cookie (&fc);
		return xattr.attr;
	}
}

long _cdecl
sys_f_delete (const char *name)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie dir, fc;
	XATTR xattr;
	long r;
	char temp1[PATH_MAX];
	ushort mode;


	TRACE(("Fdelete(%s)", name));

	/* get a cookie for the directory the file is in */
	r = path2cookie (p, name, temp1, &dir);
	if (r)
	{
		DEBUG(("Fdelete: couldn't get directory cookie: error %ld", r));
		return r;
	}

	/* check for write permission on directory */
	r = dir_access (cred, &dir, S_IWOTH, &mode);
	if (r)
	{
		DEBUG(("Fdelete(%s): write access to directory denied",name));
		release_cookie (&dir);
		return EACCES;
	}

	/* now get the file attributes */
	r = xfs_lookup (dir.fs, &dir, temp1, &fc);
	if (r)
	{
		DEBUG(("Fdelete: error %ld while looking for %s", r, temp1));
		release_cookie (&dir);
		return r;
	}

	r = xfs_getxattr (fc.fs, &fc, &xattr);
	if (r < E_OK)
	{
		release_cookie (&dir);
		release_cookie (&fc);

		DEBUG(("Fdelete: couldn't get file attributes: error %ld", r));
		return r;
	}

	/* do not allow directories to be deleted */
	if (S_ISDIR(xattr.mode))
	{
		release_cookie (&dir);
		release_cookie (&fc);

		DEBUG(("Fdelete: %s is a directory", name));
		return EISDIR;
	}

	/* check effective uid if directories sticky bit is set */
	if ((mode & S_ISVTX) && cred->euid
		&& cred->euid != xattr.uid)
	{
		release_cookie (&dir);
		release_cookie (&fc);

		DEBUG(("Fdelete: sticky bit set and not owner"));
		return EACCES;
	}

	/* TOS domain processes can only delete files if they have write permission
	 * for them
	 */
	if (p->domain == DOM_TOS)
	{
		/* see if we're allowed to kill it */
		if (denyaccess (cred, &xattr, S_IWOTH))
		{
			release_cookie (&dir);
			release_cookie (&fc);

			DEBUG(("Fdelete: file access denied"));
			return EACCES;
		}
	}

	release_cookie (&fc);
	r = xfs_remove (dir.fs, &dir,temp1);
	release_cookie (&dir);

	return r;
}

long _cdecl
sys_f_rename (short junk, const char *old, const char *new)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie olddir, newdir, oldfil;
	XATTR xattr;
	char temp1[PATH_MAX], temp2[PATH_MAX];
	long r;
	ushort mode;

	/* ignored, for TOS compatibility */
	UNUSED(junk);


	TRACE(("Frename(%s, %s)", old, new));

	r = path2cookie (p, old, temp2, &olddir);
	if (r)
	{
		DEBUG(("Frename(%s,%s): error parsing old name",old,new));
		return r;
	}

	/* check for permissions on the old file
	 * GEMDOS doesn't allow rename if the file is FA_RDONLY
	 * we enforce this restriction only on regular files; processes,
	 * directories, and character special files can be renamed at will
	 */
	r = relpath2cookie (p, &olddir, temp2, (char *)0, &oldfil, 0);
	if (r)
	{
		DEBUG(("Frename(%s,%s): old file not found",old,new));
		release_cookie (&olddir);
		return r;
	}

	r = xfs_getxattr (oldfil.fs, &oldfil, &xattr);
	release_cookie (&oldfil);
	if (r || (S_ISREG(xattr.mode)
			&& ((xattr.attr & FA_RDONLY)
				&& cred->euid
				&& (cred->euid != xattr.uid))))
	{
	       /* Only SuperUser and the owner of the file are allowed to rename
		* readonly files
		*/
		DEBUG(("Frename(%s,%s): access to old file not granted",old,new));
		release_cookie (&olddir);
		return EACCES;
	}

	r = path2cookie(p, new, temp1, &newdir);
	if (r)
	{
		DEBUG(("Frename(%s,%s): error parsing new name",old,new));
		release_cookie (&olddir);
		return r;
	}

	if (newdir.fs != olddir.fs)
	{
		DEBUG(("Frename(%s,%s): different file systems",old,new));
		release_cookie (&olddir);
		release_cookie (&newdir);

		/* cross device rename */
		return EXDEV;
	}

	/* check for write permission on both directories */
	r = dir_access (cred, &olddir, S_IWOTH, &mode);
	if (!r && (mode & S_ISVTX) && cred->euid
	    && cred->euid != xattr.uid)
		r = EACCES;

	if (!r) r = dir_access (cred, &newdir, S_IWOTH, &mode);

	if (r)
		DEBUG(("Frename(%s,%s): access to a directory denied",old,new));
	else
		r = xfs_rename (newdir.fs, &olddir, temp2, &newdir, temp1);

	release_cookie (&olddir);
	release_cookie (&newdir);

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
sys_d_pathconf (const char *name, int which)
{
	struct proc *p = get_curproc();

	fcookie dir;
	long r;

	r = path2cookie (p, name, NULL, &dir);
	if (r)
	{
		DEBUG(("Dpathconf(%s): bad path",name));
		return r;
	}

	r = xfs_pathconf (dir.fs, &dir, which);
	if (which == DP_CASE && r == ENOSYS)
	{
		/* backward compatibility with old .XFS files */
		r = (dir.fs->fsflags & FS_CASESENSITIVE) ? DP_CASESENS :
				DP_CASEINSENS;
	}

	release_cookie (&dir);
	return r;
}

/*
 * GEMDOS extension: Opendir/Readdir/Rewinddir/Closedir
 *
 * offer a new, POSIX-like alternative to Fsfirst/Fsnext,
 * and as a bonus allow for arbitrary length file names
 */
long _cdecl
sys_d_opendir (const char *name, int flag)
{
	struct proc *p = get_curproc();

	DIR *dirh;
	fcookie dir;
	long r;
	ushort mode;

	r = path2cookie (p, name, follow_links, &dir);
	if (r)
	{
		DEBUG(("Dopendir(%s): error %ld", name, r));
		return r;
	}

	r = dir_access (p->p_cred->ucr, &dir, S_IROTH, &mode);
	if (r)
	{
		DEBUG(("Dopendir(%s): read permission denied", name));
		release_cookie (&dir);
		return r;
	}

	dirh = kmalloc (sizeof (*dirh));
	if (!dirh)
	{
		release_cookie (&dir);
		return ENOMEM;
	}

	dirh->fc = dir;
	dirh->index = 0;
	dirh->flags = flag;
	r = xfs_opendir (dir.fs, dirh, flag);
	if (r)
	{
		DEBUG(("d_opendir(%s): opendir returned %ld", name, r));
		release_cookie (&dir);
		kfree (dirh);
		return r;
	}

	/* we keep a chain of open directories so that if a process
	 * terminates without closing them all, we can clean up
	 */
	dirh->next = p->p_fd->searches;
	p->p_fd->searches = dirh;

	dirh->fd = 0; /* less than MIN_OPEN */

	assert(((long) dirh) > 0);
	return (long) dirh;
}

long _cdecl
sys_d_readdir (int len, long handle, char *buf)
{
	struct proc *p = get_curproc();
	DIR *dirh = (DIR *) handle;
	fcookie fc;
	long r;
	DIR **where;

	where = &p->p_fd->searches;
	while (*where && *where != dirh)
		where = &((*where)->next);

	if (!*where)
	{
		DEBUG(("Dreaddir: not an open directory"));
		return EBADF;
	}

	if (!dirh->fc.fs)
		return EBADF;

	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (r == E_OK)
		release_cookie (&fc);

	return r;
}

/* jr: just as d_readdir, but also returns XATTR structure (not
 * following links). Note that the return value reflects the
 * result of the Dreaddir operation, the result of the Fxattr
 * operation is stored in long *xret
 */
long _cdecl
sys_d_xreaddir (int len, long handle, char *buf, XATTR *xattr, long *xret)
{
	struct proc *p = get_curproc();
	DIR *dirh = (DIR *) handle;
	fcookie fc;
	long r;
	DIR **where;

	where = &p->p_fd->searches;
	while (*where && *where != dirh)
		where = &((*where)->next);

	if (!*where)
	{
		DEBUG(("Dxreaddir: not an open directory"));
		return EBADF;
	}

	if (!dirh->fc.fs)
		return EBADF;

	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (r != E_OK)
		return r;

	*xret = xfs_getxattr (fc.fs, &fc, xattr);
	if ((*xret == E_OK) && (fc.fs->fsflags & FS_EXT_3))
	{
		xtime_to_local_dos(xattr, m);
		xtime_to_local_dos(xattr, a);
		xtime_to_local_dos(xattr, c);
	}

	release_cookie (&fc);
	return r;
}


long _cdecl
sys_d_rewind (long handle)
{
	struct proc *p = get_curproc();
	DIR *dirh = (DIR *) handle;
	DIR **where;

	where = &p->p_fd->searches;
	while (*where && *where != dirh)
		where = &((*where)->next);

	if (!*where)
	{
		DEBUG(("Drewinddir: not an open directory"));
		return EBADF;
	}

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
sys_d_closedir (long handle)
{
	struct proc *p = get_curproc();
	DIR *dirh = (DIR *)handle;
	DIR **where, **_where;
	long r;

	where = &p->p_fd->searches;
	while (*where && *where != dirh)
		where = &((*where)->next);

	if (!*where)
	{
		DEBUG(("Dclosedir: not an open directory"));
		return EBADF;
	}

	/* unlink the directory from the chain */
	*where = dirh->next;

	if (!dirh->fc.fs) {
		kfree (dirh);
		return E_OK;
	}

	/* If we've assigned a file descriptor to this cookie from
	 * Fdirfd, then we need to ensure we close it now too.
	 */
	if (dirh->fd >= MIN_OPEN) {
		FILEPTR *f;

		r = GETFILEPTR (&p, &dirh->fd, &f);

		if (!r) {
			/* close dirent's for the same FD */
			where = &p->p_fd->searches;
			while (*where) {
				_where = &((*where)->next);
				if ((*where)->fd == dirh->fd) {

					r = xfs_closedir ((*where)->fc.fs, (*where));
					release_cookie (&(*where)->fc);

					kfree(*where);

					/* unlink the directory from the chain */
					*where = *_where;
				}
				where = _where;
			}

			do_close(p, f);

			DEBUG (("Removing file descriptor %d", dirh->fd));

			FD_REMOVE (p, dirh->fd);
		}
	}

	r = xfs_closedir (dirh->fc.fs, dirh);
	release_cookie (&dirh->fc);

	if (r)
		DEBUG(("Dclosedir: error %ld", r));

	kfree (dirh);
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
sys_f_xattr (int flag, const char *name, XATTR *xattr)
{
	struct proc *p = get_curproc();

	fcookie fc;
	long r;

	TRACE (("Fxattr(%d, %s)", flag, name));

	r = path2cookie (p, name, flag ? NULL : follow_links, &fc);
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
		xtime_to_local_dos(xattr, m);
		xtime_to_local_dos(xattr, a);
		xtime_to_local_dos(xattr, c);
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
sys_f_link (const char *old, const char *new)
{
	struct proc *p = get_curproc();
	
	fcookie olddir, newdir;
	char temp1[PATH_MAX], temp2[PATH_MAX];
	long r;
	ushort mode;

	TRACE(("Flink(%s, %s)", old, new));

	r = path2cookie (p, old, temp2, &olddir);
	if (r)
	{
		DEBUG(("Flink(%s,%s): error parsing old name",old,new));
		return r;
	}

	r = path2cookie (p, new, temp1, &newdir);
	if (r)
	{
		DEBUG(("Flink(%s,%s): error parsing new name",old,new));
		release_cookie(&olddir);
		return r;
	}

	if (newdir.fs != olddir.fs)
	{
		DEBUG(("Flink(%s,%s): different file systems",old,new));
		release_cookie (&olddir);
		release_cookie (&newdir);
		return EXDEV;	/* cross device link */
	}

	/* check for write permission on the destination directory
	 */
	r = dir_access (p->p_cred->ucr, &newdir, S_IWOTH, &mode);
	if (r)
		DEBUG(("Flink(%s,%s): access to directory denied",old,new));
	else
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
sys_f_symlink (const char *old, const char *new)
{
	struct proc *p = get_curproc();
	
	fcookie newdir;
	long r;
	char temp1[PATH_MAX];
	ushort mode;

	TRACE(("Fsymlink(%s, %s)", old, new));

	r = path2cookie(p, new, temp1, &newdir);
	if (r)
	{
		DEBUG(("Fsymlink(%s,%s): error parsing %s", old,new,new));
		return r;
	}

	r = dir_access (p->p_cred->ucr, &newdir, S_IWOTH, &mode);
	if (r)
		DEBUG(("Fsymlink(%s,%s): access to directory denied",old,new));
	else
		r = xfs_symlink (newdir.fs, &newdir, temp1, old);

	release_cookie (&newdir);
	return r;
}

/*
 * GEMDOS extension: Freadlink(buflen, buf, linkfile)
 *
 * read the contents of the symbolic link "linkfile" into the buffer
 * "buf", which has length "buflen".
 */
long _cdecl
sys_f_readlink (int buflen, char *buf, const char *linkfile)
{
	struct proc *p = get_curproc();

	fcookie file;
	long r;
	XATTR xattr;

	TRACE(("Freadlink(%s)", linkfile));

	r = path2cookie (p, linkfile, (char *)0, &file);
	if (r)
	{
		DEBUG(("Freadlink: unable to find %s", linkfile));
		return r;
	}

	r = xfs_getxattr (file.fs, &file, &xattr);
	if (r)
	{
		DEBUG(("Freadlink: unable to get attributes for %s", linkfile));
	}
	else if (S_ISLNK(xattr.mode))
	{
		r = xfs_readlink (file.fs, &file, buf, buflen);
	}
	else
	{
		DEBUG(("Freadlink: %s is not a link", linkfile));
		r = EACCES;
	}

	release_cookie (&file);
	return r;
}

/*
 * GEMDOS extension: Dcntl(cmd, path, arg)
 *
 * do file system specific functions
 */
long _cdecl
sys_d_cntl (int cmd, const char *name, long arg)
{
	struct proc *p = get_curproc();

	fcookie dir;
	long r;
	char temp1[PATH_MAX];

	DEBUG (("Dcntl(cmd=%x, file=%s, arg=%lx)", cmd, name, arg));

	r = path2cookie (p, name, temp1, &dir);
	if (r)
	{
		DEBUG (("Dcntl: couldn't find %s", name));
		return r;
	}

	switch (cmd)
	{
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

			/* else fallback */
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
sys_f_chown (const char *name, int uid, int gid)
{
	return sys_f_chown16( name, uid, gid, 0 );
}

/*
 * GEMDOS extension: Fchown16(name, uid, gid, follow_symlinks)
 *
 * @param follow_symlinks set to 1 to follow or 0 to not to follow
 *                        (the other values are reserved)
 */
long _cdecl
sys_f_chown16 (const char *name, int uid, int gid, int follow_symlinks)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie fc;
	XATTR xattr;
	long r;


	TRACE(("Fchown16(%s, %d, %d, %s)", name, uid, gid, follow_symlinks != 0 ? "follow_links" : "nofollow"));

	r = path2cookie (p, name, follow_symlinks == 1 ? follow_links : NULL, &fc);
	if (r)
	{
		DEBUG(("Fchown(%s): error %ld", name, r));
		return r;
	}

	/* MiNT acts like _POSIX_CHOWN_RESTRICTED: a non-privileged process can
	 * only change the ownership of a file that is owned by this user, to
	 * the effective group id of the process or one of its supplementary groups
	 */
	if (cred->euid)
	{
		if (cred->egid != gid && !groupmember (cred, gid))
			r = EACCES;
		else
			r = xfs_getxattr (fc.fs, &fc, &xattr);

		if (r)
		{
			DEBUG(("Fchown(%s): unable to get file attributes",name));
			release_cookie (&fc);
			return r;
		}

		if (xattr.uid != cred->euid || xattr.uid != uid)
		{
			DEBUG(("Fchown(%s): not the file's owner",name));
			release_cookie (&fc);
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
		if (!r && !S_ISDIR(xattr.mode)
		    && (xattr.mode & (S_ISUID | S_ISGID)))
		{
			long s;

			s = xfs_chmode (fc.fs, &fc, xattr.mode & ~(S_ISUID | S_ISGID));
			if (!s)
				DEBUG(("Fchown: chmode returned %ld (ignored)", s));
		}
	}
	else
		r = xfs_chown (fc.fs, &fc, uid, gid);

	release_cookie (&fc);
	return r;
}

/*
 * GEMDOS extension: Fchmod (file, mode)
 *
 * changes a file's access permissions.
 */
long _cdecl
sys_f_chmod (const char *name, unsigned short mode)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie fc;
	long r;
	XATTR xattr;


	TRACE (("Fchmod(%s, %o)", name, mode));
	r = path2cookie (p, name, follow_links, &fc);
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
	else if (cred->euid && cred->euid != xattr.uid)
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

struct proc *dlockproc [NUM_DRIVES];

long _cdecl
sys_d_lock (int mode, int _dev)
{
	struct proc *cp = get_curproc();
	struct proc *p;

	FILEPTR *f;
	int i;
	ushort dev = _dev;

	TRACE (("Dlock (%x,%c:)", mode, DriveToLetter(dev)));

	/* check for alias drives */
	if (dev < NUM_DRIVES && aliasdrv[dev])
		dev = aliasdrv[dev] - 1;

	/* range check */
	if (dev >= NUM_DRIVES)
		return ENXIO;

	if ((mode & 1) == 0)	/* unlock */
	{
		if (dlockproc[dev] == cp)
		{
			dlockproc[dev] = NULL;
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
		struct filedesc *fd = p->p_fd;
		struct cwd *cwd = p->p_cwd;

		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;

		assert (fd && cwd);

		if (cwd->root_dir && cwd->rootdir.dev == dev)
			return EACCES;

		for (i = MIN_HANDLE; i < fd->nfiles; i++)
		{
			f = fd->ofiles[i];
			if (f && (f != (FILEPTR *) 1) && (f->fc.fs && f->fc.dev == dev))
			{
				DEBUG (("Dlock: process %d (%s) has an open "
					"handle on the drive", p->pid, p->name));

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
		struct cwd *cwd = p->p_cwd;

		if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;

		assert (cwd);

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (cwd->root[i].dev == dev)
			{
				release_cookie (&cwd->root[i]);
				cwd->root[i].fs = 0;
			}
			if (cwd->curdir[i].dev == dev)
			{
				release_cookie (&cwd->curdir[i]);
				cwd->curdir[i].fs = 0;
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
				DEBUG (("Unmounting %c: ...", DriveToLetter(dev)));
				(void) xfs_unmount (fs, dev);
			}
			else
			{
				sys_s_ync ();

				DEBUG (("Invalidate %c: ...", DriveToLetter(dev)));
				(void) xfs_dskchng (fs, dev, 1);
			}

			drives [dev] = NULL;
		}
	}

	dlockproc[dev] = get_curproc();
	return E_OK;
}

/*
 * GEMDOS-extension: Dreadlabel(path, buf, buflen)
 *
 * original written by jr
 */
long _cdecl
sys_d_readlabel (const char *name, char *buf, int buflen)
{
	struct proc *p = get_curproc();

	fcookie dir;
	long r;

	r = path2cookie (p, name, NULL, &dir);
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
sys_d_writelabel (const char *name, const char *label)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;

	fcookie dir;
	long r;

	/* Draco: in secure mode only superuser can write labels
	 */
	if (secure_mode && (cred->euid))
	{
		DEBUG (("Dwritelabel(%s): access denied", name));
		return EACCES;
	}

	r = path2cookie (p, name, NULL, &dir);
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
sys_d_chroot (const char *path)
{
	struct proc *p = get_curproc();
	struct ucred *cred = p->p_cred->ucr;
	struct cwd *cwd = p->p_cwd;

	XATTR xattr;
	fcookie dir;
	long r;


	DEBUG (("Dchroot(%s): enter", path));
	assert (cwd);

	if (cred->euid || !p->domain)
	{
		DEBUG (("Dchroot(%s): access denied", path));
		return EPERM;
	}

	r = path2cookie (p, path, follow_links, &dir);
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

		r = xfs_getname (dir.fs, &cwd->root[dir.dev], &dir, buf, PATH_MAX);
		if (r)
		{
			DEBUG( ("Dchroot(%s): getname fail!", path));
			goto error;
		}

		cwd->root_dir = kmalloc (strlen (buf) + 1);
		if (!cwd->root_dir)
		{
			DEBUG (("Dchroot(%s): kmalloc fail!", buf));
			r = ENOMEM;
			goto error;
		}

		strcpy (cwd->root_dir, buf);
	}

	cwd->rootdir = dir;

	DEBUG (("Dchroot(%s): ok [%lx,%i]", cwd->root_dir, dir.index, dir.dev));
	DEBUG (("Dchroot: [%lx,%p]", cwd->curdir[dir.dev].index, cwd->curdir[dir.dev].fs));
	DEBUG (("Dchroot: [%lx,%p]", cwd->root[dir.dev].index, cwd->root[dir.dev].fs));
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
sys_f_stat64 (int flag, const char *name, STAT *stat)
{
	struct proc *p = get_curproc();

	fcookie fc;
	long r;

	TRACE (("Fstat64(%d, %s)", flag, name));

	r = path2cookie (p, name, flag ? NULL : follow_links, &fc);
	if (r)
	{
		DEBUG (("Fstat64(%s): path2cookie returned %ld", name, r));
		return r;
	}

	r = xfs_stat64 (fc.fs, &fc, stat);
	if (r) DEBUG (("Fstat64(%s): returning %ld", name, r));

	release_cookie (&fc);
	return r;
}

/*
 * GEMDOS extension: Fchdir(fd)
 *
 * sets the current directory from a file descriptor
 */
long _cdecl
sys_f_chdir (short fd)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	XATTR xattr;
	int drv;
	struct cwd *cwd = p->p_cwd;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (!(f->fc.fs))
	{
		DEBUG (("Ffchdir: not a valid filesystem"));
		return ENOSYS;
	}

	r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
	if (r)
	{
		DEBUG (("Ffchdir(%i): couldn't get directory attributes", fd));
		release_cookie (&(f->fc));
		return r;
	}

	if (!(xattr.attr & FA_DIR))
	{
		DEBUG (("Ffchdir(%i): not a directory", fd));
		release_cookie (&(f->fc));
		return ENOTDIR;
	}

	if (denyaccess (p->p_cred->ucr, &xattr, S_IXOTH))
	{
		DEBUG (("Ffchdir(%i): access denied", fd));
		release_cookie (&(f->fc));
		return EACCES;
	}

	/* watch out for symbolic links; if c:\foo is a link to d:\bar, then
	 * "cd c:\foo" should also change the drive to d:
	 */
	drv = cwd->curdrv;
	if (drv != UNIDRV && f->fc.dev != cwd->root[drv].dev)
	{
		int i;

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if (cwd->root[i].dev == f->fc.dev
				&& cwd->root[i].fs == f->fc.fs)
			{
				if (cwd->curdrv == drv)
					cwd->curdrv = i;

				drv = i;
				break;
			}
		}
	}

	release_cookie (&cwd->curdir[drv]);
	dup_cookie(&cwd->curdir[drv], &f->fc);

	return E_OK;
}

/*
 * GEMDOS extension: fdopendir
 *
 * opendir with a file descriptor
 */
long _cdecl
sys_f_opendir (short fd)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	fcookie *dir;
	long r;
	DIR *dirh;
	ushort mode;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (!(f->fc.fs))
	{
		DEBUG (("Ffdopendir: not a valid filesystem"));
		return ENOSYS;
	}

	dir = &(f->fc);
	r = dir_access (p->p_cred->ucr, dir, S_IROTH, &mode);
	if (r)
	{
		DEBUG (("Ffdopendir(%i): read permission denied", fd));
		return r;
	}

	dirh = kmalloc (sizeof (*dirh));
	if (!dirh)
	{
		DEBUG (("Ffdopendir(%i): out of memory", fd));
		return ENOMEM;
	}

	dup_cookie(&dirh->fc, dir);
	dirh->index = 0;
	dirh->flags = 0;
	r = xfs_opendir (dirh->fc.fs, dirh, dirh->flags);
	if (r)
	{
		DEBUG (("Ffdopendir(%i): fdopendir returned %ld", fd, r));
		release_cookie (&dirh->fc);
		kfree (dirh);
		return r;
	}

	/* we keep a chain of open directories so that if a process
	 * terminates without closing them all, we can clean up
	 */
	dirh->next = p->p_fd->searches;
	p->p_fd->searches = dirh;

	dirh->fd = fd;

	assert(((long) dirh) > 0);
	return (long) dirh;
}

/*
 * GEMDOS extension: fdirfd
 *
 * a file descriptor from DIR*
 */
long _cdecl
sys_f_dirfd (long handle)
{
	struct proc *p = get_curproc();
	DIR *dirh = (DIR *)handle;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN;
	DIR **where;
	long r;
	long devsp;
	DEVDRV *dev;

	where = &p->p_fd->searches;
	while (*where && *where != dirh)
		where = &((*where)->next);

	if (!*where)
	{
		DEBUG (("Fdirfd: not an open directory"));
		return EBADF;
	}

	if (!dirh->fc.fs)
	{
		DEBUG (("Fdirfd: not a valid filesystem"));
		return EBADF;
	}

	/* locate previously handed fd */
	if (dirh->fd >= MIN_OPEN) {
		DEBUG(("Same descriptor %d found",dirh->fd));

		return dirh->fd;
	}

	dev = xfs_getdev (dirh->fc.fs, &dirh->fc, &devsp);
	if (!dev)
	{
		DEBUG (("Fdirfd: device driver not found (%li)", devsp));
		return devsp ? devsp : EINTERNAL;
	}

	r = FD_ALLOC (p, &fd, MIN_OPEN);
	if (r) goto error;

	if (dev == &fakedev)
	{
		/* fake BIOS devices */
		assert (p->p_fd);

		fp = p->p_fd->ofiles[devsp];
		if (!fp || fp == (FILEPTR *) 1) {
			FD_REMOVE (p, fd);
			return EBADF;
		}

		fp->links++;
	} else {
		r = FP_ALLOC (p, &fp);
		if (r) goto error;

		fp->links = 1;
		fp->flags = O_RDONLY;
		fp->pos = 0;
		fp->devinfo = devsp;
		fp->dev = dev;
		dup_cookie(&fp->fc, &dirh->fc);
		r = xdd_open (fp);
		if (r < E_OK)
		{
			release_cookie(&fp->fc);
			goto error;
		}
	}

	dirh->fd = fd;	/* associate this dirp with this fd */

	/* activate the fp, default is to close non-standard files on exec */
	FP_DONE (p, fp, fd, FD_CLOEXEC);

	return fd;

error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }

	return r;
}
