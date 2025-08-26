/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "minixdev.h"
# include "minixsys.h"

# include "mint/ioctl.h"

# include "bitmap.h"
# include "inode.h"
# include "zone.h"
# include "misc.h"


static long	_cdecl m_open		(FILEPTR *f);
static long	_cdecl m_write		(FILEPTR *f, const char *buf, long len);
static long	_cdecl m_read		(FILEPTR *f, char *buf, long len);
static long	_cdecl m_seek		(FILEPTR *f, long offset, int flag);
static long	_cdecl m_ioctl		(FILEPTR *f, int mode, void *buf);
static long	_cdecl m_datime		(FILEPTR *f, ushort *timeptr, int flag);
static long	_cdecl m_select		(FILEPTR *f, long proc, int mode);
static void	_cdecl m_unselect	(FILEPTR *f, long proc, int mode);

DEVDRV minix_dev =
{
	m_open, m_write, m_read,
	m_seek, m_ioctl, m_datime,
	m_close, m_select, m_unselect
};

# ifndef IO_Q
# define IO_Q 3
# endif

/* Minix fs device driver */

/* Under minixfs there is no 'per file structure', that is all references to
 * the same file are independent. This complicates file sharing a bit, the 'next'
 * field points to the next fileptr for the minixfs, so that a search checks the
 * list sequentially (the global variable 'firstptr' is the start of the list),
 * references to the same file are grouped together so that the first reference
 * can act as a list pointer to denyshare(), though the last reference's 'next'
 * pointer is temporarily set to NULL to keep denyshare() happy.
 */

static long _cdecl
m_open (FILEPTR *f)
{
	FILEPTR *lst, *elst, *tmplst;
	f_cache *fch;
	d_inode rip;
	
	/* do some sanity checking */
	read_inode (f->fc.index, &rip, f->fc.dev);
	
	if (!IS_REG (rip))
	{
		if (!(IS_DIR (rip) && ((f->flags & O_RWMODE) == O_RDONLY))) {
			DEBUG (("Minix-FS (%c): m_open: not a regular file.", DriveToLetter(f->fc.dev)));
			return EACCES;
		}
	}
	
	/* Set up f_cache structure */
	fch = kmalloc (sizeof (f_cache));
	if (!fch)
	{
		DEBUG (("Minix-FS (%c): m_open: no memory for f_cache structure.", DriveToLetter(f->fc.dev)));
		return ENOMEM;
	}
	
	bzero (fch, sizeof (f_cache));
	f->devinfo = (long) fch;
	
	/* Find first pointer to same file, if any */
	for (lst = firstptr; lst; lst = lst->next)
	{
		if ((f->fc.dev == lst->fc.dev) && (f->fc.index == lst->fc.index))
			break;
	}
	
	if (lst)
	{
		/* Find last pointer to file */
		for (elst = lst; elst->next; elst = elst->next)
		{
			if ((elst->next->fc.dev != lst->fc.dev)
				|| (elst->next->fc.index!=lst->fc.index))
			{
				break;
			}
		}
		
		tmplst = elst->next;
		elst->next = 0;
		
		if (denyshare (lst, f))
		{
			elst->next = tmplst;
			
			kfree (fch);
			return EACCES;
		}
		
		elst->next = f;
		
		/* If truncating invalidate all f_cache zones */
		if (f->flags & O_TRUNC)
		{
			FILEPTR *p;
			for (p = lst; p; p = p->next)
			{
				((f_cache *) p->devinfo)->lzone = 0;
			}
		}
		
		f->next = tmplst;
		fch->lfirst = ((f_cache *) elst->devinfo)->lfirst;
	}
	else
	{
		/* Stick new fptr at top */
		f->next = firstptr;
		firstptr = f;
		fch->lfirst = kmalloc (sizeof (LOCK *)); /* List of locks */
		*fch->lfirst = 0; /* No locks yet */
	}
	
	/* should we truncate the file? */
	if (f->flags & O_TRUNC)
	{
		if (super_ptr [f->fc.dev]->s_flags & MS_RDONLY)
		{
			kfree (fch);
			return EROFS;
		}
		
		trunc_inode (&rip, f->fc.dev, 0L, 1);
		rip.i_size = 0;
		rip.i_mtime = CURRENT_TIME;
		write_inode (f->fc.index, &rip, f->fc.dev);
		
		sync (f->fc.dev);
	}
	
	f->pos = 0;
	return 0;
}

long _cdecl
m_close (FILEPTR *f, int pid)
{
	FILEPTR **last;
	f_cache *fch = (f_cache *) f->devinfo;

	/* If locked remove any locks for this pid */
	if (f->flags & O_LOCK)
	{
		LOCK *lck, **oldl;
		
		TRACE (("Minix-FS (%c): m_close: removing locks for pid %d", DriveToLetter(f->fc.dev), pid));
		
		oldl = fch->lfirst;
		lck = *oldl;
		while (lck)
		{
			if (lck->l.l_pid == pid) 
			{
				*oldl = lck->next;
				wake (IO_Q, (long) lck);
				kfree (lck);
			}
			else
				oldl = &lck->next;
			
			lck = *oldl;
		}
	}

	if (f->links <= 0)
	{
		/* Last fptr ? */
		if (inode_busy (f->fc.index, f->fc.dev, 0) < 2)
		{
			/* Free LOCK pointer */
			kfree (fch->lfirst);
			if (f->fc.aux & AUX_DEL)
			{
				d_inode rip;
				
				DEBUG (("Minix-FS (%c): m_close: Deleting unlinked file", DriveToLetter(f->fc.dev)));
				
				read_inode (f->fc.index, &rip, f->fc.dev);
				trunc_inode (&rip, f->fc.dev, 0L, 0);
				rip.i_mode = 0;
				write_inode (f->fc.index, &rip, f->fc.dev);
				free_inode (f->fc.dev, f->fc.index);
			}
		}
		
		kfree (fch);
		for (last = &firstptr;; last = &(*last)->next)
		{
			if (*last == f)
			{
				*last = f->next;
				break;
			}
			else if (!*last)
			{
				ALERT (("Minix-FS (%c): m_close: FILEPTR chain corruption!", DriveToLetter(f->fc.dev)));
				break;
			}
		}
	}
	
	sync (f->fc.dev); /* always sync on close */
	return 0;
}


# define READ 0
# define WRITE 1

INLINE long
__next (FILEPTR *f, d_inode *rip, long chunk, short mode)
{
	f_cache *fch = (f_cache *) f->devinfo;
	register long znew = 0;
	
	if (mode == READ && ((chunk >= fch->fzone) && (chunk < fch->lzone)))
		znew = fch->zones[chunk - fch->fzone];
	
	if (!znew)
		znew = find_zone (rip, chunk, f->fc.dev, mode);
	
	return znew;
}

INLINE void
__update_rip (long index, d_inode *rip, ushort dev, long size, short mode)
{
	register ushort flag = 0;
	
	/*
	 * For floppies never update atime this is a bit of a hack, should 
	 * really test write protection and act accordingly. Status is '3'
	 * so that, for example, if a program run on a Minixfs filesystem 
	 * causes a disk change, nasty error messages are avoided. 
	 */
	
	if (size > rip->i_size && mode == WRITE)
	{
		rip->i_size = size;
		flag = 1;
	}
	
	if (dev > 1)
	{
		if (mode == WRITE)
		{
			rip->i_mtime = CURRENT_TIME;
			flag = 1;
		}
		else if (!(super_ptr [dev]->s_flags & MS_RDONLY))
		{
			rip->i_atime = CURRENT_TIME;
			flag = 1;
		}
	}
	
	if (flag)
		write_inode (index, rip, dev);
}

static long
__fio (FILEPTR *f, char *buf, long len, short mode)
{
	f_cache *fch = (f_cache *) f->devinfo;
	d_inode rip;
	
	long chunk;
	long todo = len;	/* Characters remaining */
	long done = 0;		/* processed characters */
	
	if (len <= 0)
	{
		if (len < 0 && mode == READ)
		{
			/* hmm, Draco's idea */
			
			len = 2147483647L; /* LONG_MAX */
			DEBUG (("Minix-FS (%c): __fio: (fix) mode == READ -> bytes = %li", DriveToLetter(f->fc.dev), len));
		}
		else
		{
			DEBUG (("Minix-FS (%c): __fio: len = %li (mode %i)", DriveToLetter(f->fc.dev), len, mode));
			return 0;
		}
	}
	
	read_inode (f->fc.index, &rip, f->fc.dev);

	if (IS_DIR (rip))
		return EISDIR;

	if (mode == READ)
	{
		todo = MIN (rip.i_size - f->pos, len);
	}
	else
	{
		if (super_ptr [f->fc.dev]->s_flags & MS_RDONLY)
			return 0;
	}
	
	/* At or past EOF */
	if (todo <= 0)
		return 0;
	
	chunk = f->pos >> L_BS;
	
	/* Every PRE_READ blocks, try to read in PRE_READ zones into cache */
	if (mode == READ && (chunk >= fch->lzone && (len >> L_BS < PRE_READ )))
	{
		long i;
		for (i = 0; i < PRE_READ; i++)
			fch->zones[i] = find_zone (&rip, i + chunk, f->fc.dev, 0);
		
		bio.pre_read (super_ptr[f->fc.dev]->di, (ulong *)fch->zones, PRE_READ, BLOCK_SIZE);
		
		fch->fzone = chunk;
		fch->lzone = chunk + PRE_READ;
	}
	
	/* Are we block aligned ? If not read/copy partial block */
	if (f->pos & (BLOCK_SIZE - 1))
	{
		long znew = __next (f, &rip, chunk++, mode);
		long off = f->pos & (BLOCK_SIZE - 1);
		long data = BLOCK_SIZE - off;
		data = MIN (todo, data);
		
		if (znew) /* Sparse file ? */
		{
			UNIT *u = cget_zone (znew, f->fc.dev);
			if (mode == READ)
			{
				bcopy (u->data + off, buf, data);
			}
			else
			{
				bcopy (buf, u->data + off, data);
				bio_MARK_MODIFIED (&(bio), u);
			}
		}
		else
			if (mode == READ)
				bzero (buf, data);
			else
			{
				/* hmm, what's wrong here ? */
				goto out;
			}
		
		buf += data;
		todo -= data;
		done += data;
		f->pos += data;
	}
	
	/* Any full blocks to read ? */
	while (todo >> L_BS)
	{
		long zstart = __next (f, &rip, chunk++, mode);
		register long zold = zstart;
		register long zones = 1;
		register long data = BLOCK_SIZE;
		
		if (zstart == 0)
		{
			if (mode == WRITE)
			{
				/* partition full */
				goto out;
			}
			
			DEBUG (("Minix-FS (%c): __fio: MODE == READ, zstart == 0, todo = %li, chunk = %li", DriveToLetter(f->fc.dev), todo, chunk));
			
			bzero (buf, data);
			goto cont;
		}
		
		if (todo - data >= BLOCK_SIZE)
		{
			register long znew = __next (f, &rip, chunk, mode);
			
			/* linear read/write optimization */
			while ((znew > 0) && ((znew - zold) == 1))
			{
				data += BLOCK_SIZE;
				zones++;
				chunk++;
				
				if (todo - data >= BLOCK_SIZE)
				{
					zold = znew;
					znew = __next (f, &rip, chunk, mode);
				}
				else
					break;
			}
		}
		
		if (mode == READ)
		{
			if (zones > 1)
				read_zones (zstart, zones, buf, f->fc.dev);
			else
				read_zone (zstart, buf, f->fc.dev);
		}
		else
		{
			if (zones > 1)
				write_zones (zstart, zones, buf, f->fc.dev);
			else
				write_zone (zstart, buf, f->fc.dev);
		}
		
cont:
		buf += data;
		todo -= data;
		done += data;
		f->pos += data;
	}
	
	/* Anything left ? */
	if (todo)
	{
		long znew = __next (f, &rip, chunk, mode);
		
		if (znew)
		{
			UNIT *u = cget_zone (znew, f->fc.dev);
			if (mode == READ)
			{
				bcopy (u->data, buf, todo);
			}
			else
			{
				bcopy (buf, u->data, todo);
				bio_MARK_MODIFIED (&(bio), u);
			}
			done += todo;
			f->pos += todo;
		}
		else
			if (mode == READ)
				bzero (buf, todo);
			else
			{
				/* partition full */
				goto out;
			}
	}
	
out:
	if (!(f->flags & O_NOATIME))
		__update_rip (f->fc.index, &rip, f->fc.dev, f->pos, mode);
	
	return done;	
}

static long _cdecl
m_write (FILEPTR *f, const char *buf, long len)
{
	union { const char *c; char *nc; } b;
	long ret;

	b.c = buf;
	ret = __fio (f, b.nc, len, WRITE);
	
# if 0
	/* only in robust mode -> that means never */
	sync (f->fc.dev);
# endif
	return ret;
}

/* Minix read, all manner of horrible things can happen during 
 * a read, if fptr->pos is not block aligned we need to copy a partial
 * block then a load of full blocks then a final possibly partial block
 * any of these can hit EOF and we mustn't copy anything past EOF ...
 * my poor head :-( 
 */

static long _cdecl
m_read (FILEPTR *f, char *buf, long len)
{
	return __fio (f, buf, len, READ);
}

/* seek is a bit easier */

static long _cdecl
m_seek (FILEPTR *f, long where, int whence)
{
	d_inode rip;
	
	TRACE (("Minix-FS: m_lseek (where = %li, whence = %i)", where, whence));
	
	read_inode (f->fc.index, &rip, f->fc.dev);
	
	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
		case SEEK_END:	where += rip.i_size;	break;
		default:	return ENOSYS;
	}
	
	if (where < 0)
	{
		DEBUG (("Minix-FS: m_lseek: leave failure EBADARG (where = %li, size = %li)", where, rip.i_size));
		return EBADARG;
	}
	
	f->pos = where;
	
	return where;
}

static long _cdecl
m_ioctl (FILEPTR *f, int mode, void *buf)
{
	switch (mode)
	{
		case FIONREAD:
		{
			d_inode rip;
			long nread;
			
			read_inode (f->fc.index, &rip, f->fc.dev);
			nread = rip.i_size - f->pos;
			if (nread < 0)
				nread = 0;
			
			*(long *) buf = nread;
			return E_OK;
		}
		case FIONWRITE:
		{
			*((long *) buf) = 1;
			return E_OK;
		}

		/* File locking code, unlike sharing *lfirst of the f_cache structure 
		 * exists on a 'per-file' basis, this simplifies things somewhat.
		 */
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			f_cache *fch = (f_cache *) f->devinfo;
			struct flock *fl = (struct flock *) buf;
			
			LOCK t;
			LOCK *lck;
			
			int cpid;		/* Current proc pid */
			
			t.l = *fl;
			
			switch (t.l.l_whence)
			{
				case SEEK_SET:
				{
					break;
				}
				case SEEK_CUR:
				{
					t.l.l_start += f->pos;
					break;
				}
				case SEEK_END:
				{
					d_inode rip;
					read_inode (f->fc.index, &rip, f->fc.dev);
					t.l.l_start += rip.i_size;
					break;
				}
				default:
				{
					DEBUG (("Minix-FS (%c): m_ioctl: Invalid value for l_whence", DriveToLetter(f->fc.dev)));
					return ENOSYS;
				}
			}
			
			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;
			
			cpid = p_getpid ();
			
			if (mode == F_GETLK)
			{
				lck = denylock (cpid, *fch->lfirst, &t);
				if (lck)
					*fl = lck->l;
				else
					fl->l_type = F_UNLCK;
				
				return E_OK;
			}
			
			if (t.l.l_type == F_UNLCK)
			{
				/* Try to find the lock */
				LOCK **lckptr = fch->lfirst;
				
				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
		                                && ((lck->l.l_start == t.l.l_start
						     && lck->l.l_len == t.l.l_len) ||
						    (lck->l.l_start >= t.l.l_start
						     && t.l.l_len == 0)))
					{
						*lckptr = lck->next;
						
						/* wake up anyone waiting on the lock */
						wake (IO_Q, (long) lck);
						kfree (lck);
						
						return E_OK;
					}
			 		
					lckptr = &(lck->next);
					lck = lck->next;
				}
				
				return ENSLOCK;
			}
			
			while (lck = denylock (cpid, *fch->lfirst, &t), lck != 0)
			{
				if (mode == F_SETLKW)
					/* sleep a while */
					sleep (IO_Q, (long) lck);
				else
					return ELOCKED;
			}
			
			lck = kmalloc (sizeof (*lck));
			if (!lck) return ENOMEM;
			
			lck->l = t.l;
			lck->l.l_pid = cpid;
			
			/* Insert lck at top of list */
			lck->next = *fch->lfirst;
			*fch->lfirst = lck;
			
			/* Lock op done on FILEPTR */
			f->flags |= O_LOCK;
			return E_OK;
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			d_inode rip;
			int uid = p_geteuid ();
			
			if (super_ptr [f->fc.dev]->s_flags & MS_RDONLY)
				return EROFS;
			
			read_inode (f->fc.index, &rip, f->fc.dev);
			
			/* The owner or super-user can always touch, others only
			 * if timeptr == 0 and open for writing
			 */
			if (uid && uid != rip.i_uid
				&& (buf || ((f->flags & O_RWMODE) == O_RDONLY)))
	  		{
				return EACCES;
	  		}
			
			rip.i_ctime = CURRENT_TIME;
			if (buf)
			{
				if (native_utc || (mode == FUTIME_UTC))
				{
					long *timeptr = (long *) buf;
					
					rip.i_atime = timeptr[0];
					rip.i_mtime = timeptr[1];
				}
				else
				{
					ushort *timeptr = (ushort *) buf;
					
					rip.i_atime = unixtime (timeptr[0], timeptr[1]);
					rip.i_mtime = unixtime (timeptr[2], timeptr[3]);
				}
			}
			else
			{
				rip.i_mtime = rip.i_atime = rip.i_ctime;
			}
			
			write_inode (f->fc.index, &rip, f->fc.dev);
			
			sync (f->fc.dev);
			return E_OK;
		}
		case FTRUNCATE:
		{
			if (super_ptr [f->fc.dev]->s_flags & MS_RDONLY)
				return EROFS;
			
			if ((f->flags & O_RWMODE) == O_RDONLY)
				return EACCES;
			
			itruncate (f->fc.index, f->fc.dev, *((long *) buf));
			
			sync (f->fc.dev);
			return E_OK;
		}
		case FIBMAP:
		{
			d_inode rip;
			long block;
			
			DEBUG (("MinixFS: m_ioctl (FIBMAP)"));
			
			if (!buf)
				return EINVAL;
			
			read_inode (f->fc.index, &rip, f->fc.dev);
			
			block = *(long *) buf;
			*(long *) buf = find_zone (&rip, block, f->fc.dev, 0);
			
			return E_OK;
		}
	}
	
	return ENOSYS;
}

/* Made this a bit like utimes, sets atime, mtime and ctime = current time
 */
static long _cdecl
m_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	d_inode rip;
	
	read_inode (f->fc.index, &rip, f->fc.dev);
	switch (flag)
	{
		case 0:
		{
			if (native_utc)
				*((long *) timeptr) = rip.i_mtime;
			else
				*((long *) timeptr) = dostime (rip.i_mtime);
			
			break;
		}
		case 1:
		{
			if (super_ptr [f->fc.dev]->s_flags & MS_RDONLY)
				return EROFS;
			
			if (native_utc)
				rip.i_mtime = *((long *) timeptr);
			else
				rip.i_mtime = unixtime (timeptr[0], timeptr[1]);
			
			rip.i_atime = rip.i_mtime;
			rip.i_ctime = CURRENT_TIME;
			write_inode (f->fc.index, &rip, f->fc.dev);
			
			sync (f->fc.dev);
			return E_OK;
		}
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl
m_select (FILEPTR *f, long int proc, int mode)
{
	return 1;
}

static void _cdecl
m_unselect (FILEPTR *f, long int proc, int mode)
{
	/* Do nothing */
}
