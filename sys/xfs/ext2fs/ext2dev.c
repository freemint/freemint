/*
 * Filename:     ext2dev.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 *               Copyright 1998, 1999 Axel Kaiser (DKaiser@AM-Gruppe.de)
 * 
 * Portions copyright 1992, 1993, 1994, 1995 Remy Card (card@masi.ibp.fr)
 * and 1991, 1992 Linus Torvalds (torvalds@klaava.helsinki.fi)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include "ext2dev.h"

# include "mint/endian.h"
# include "mint/ioctl.h"

# include "ext2sys.h"
# include "inode.h"
# include "truncate.h"


static long	_cdecl e_open		(FILEPTR *f);
static long	_cdecl e_close		(FILEPTR *f, int pid);
static long	_cdecl e_write		(FILEPTR *f, const char *buf, long len);
static long	_cdecl e_read		(FILEPTR *f, char *buf, long len);
static long	_cdecl e_lseek		(FILEPTR *f, long offset, int flag);
static long	_cdecl e_ioctl		(FILEPTR *f, int mode, void *buf);
static long	_cdecl e_datime		(FILEPTR *f, ushort *timeptr, int flag);
static long	_cdecl e_select		(FILEPTR *f, long proc, int mode);
static void	_cdecl e_unselect	(FILEPTR *f, long proc, int mode);


DEVDRV ext2_dev =
{
	open:		e_open,
	write:		e_write,
	read:		e_read,
	lseek:		e_lseek,
	ioctl:		e_ioctl,
	datime:		e_datime,
	close:		e_close,
	select:		e_select,
	unselect:	e_unselect,
	writeb:		NULL,
	readb:		NULL
};


static long _cdecl
e_open (FILEPTR *f)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG (("Ext2-FS: e_open: enter #%li", c->inode));
	
	if (!EXT2_ISREG (le2cpu16 (c->in.i_mode)))
	{
		if (!(EXT2_ISDIR (le2cpu16 (c->in.i_mode)) && 
		     ((f->flags & O_RWMODE) == O_RDONLY))) {
			DEBUG (("Ext2-FS [%c]: e_open: not a regular file or read-only directory (#%ld)", DriveToLetter(f->fc.dev), c->inode));
			return EACCES;
		}
	}
	
	if (((f->flags & O_RWMODE) == O_WRONLY)
		|| ((f->flags & O_RWMODE) == O_RDWR))
	{
		if (c->s->s_flags & MS_RDONLY)
			return EROFS;
		
		if (IS_IMMUTABLE (c))
			return EACCES;
	}
	
	if (c->open && denyshare (c->open, f))
	{
		DEBUG (("Ext2-FS [%c]: e_open: file sharing denied (#%ld)", DriveToLetter(f->fc.dev), c->inode));
		return EACCES;
	}
	
	if (c->in.i_size_high)
	{
		DEBUG (("Ext2-FS [%c]: e_open: attempt to open a 64bit file(#%ld)", DriveToLetter(f->fc.dev), c->inode));
		return EFBIG;
	}
	
	if ((f->flags & O_TRUNC) && c->in.i_size)
	{
		DEBUG (("e_open: truncate file to 0 bytes"));
		
		ext2_truncate (c, 0);
		
		bio_SYNC_DRV ((&bio), c->s->di);
	}
	
	c->i_size = le2cpu32 (c->in.i_size);
	
	DEBUG (("Ext2-FS: e_open size: %li", c->i_size));
	
	f->pos = 0;
	f->next = c->open;
	c->open = f;
	c->links++;
	
	return E_OK;
}

static long _cdecl
e_close (FILEPTR *f, int pid)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG (("Ext2-FS: e_close: enter #%li", c->inode));
	
	/* if a lock was made, remove any locks of the process */
	if (f->flags & O_LOCK)
	{
		LOCK *lock;
		LOCK **oldlock;
		
		DEBUG (("Ext2-FS: e_close: remove lock (pid = %i)", pid));
		
		oldlock = &c->locks;
		lock = *oldlock;
		
		while (lock)
		{
			if (lock->l.l_pid == pid)
			{
				*oldlock = lock->next;
				/* (void) e_lock (f, 1, lock->l.l_start, lock->l.l_len); */
				wake (IO_Q, (long) lock);
				kfree (lock, sizeof (*lock));
			}
			else
			{
				oldlock = &lock->next;
			}
			lock = *oldlock;
		}
	}
	
	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;
		
		/* remove the FILEPTR from the linked list */
		temp = &c->open;
		while (*temp)
		{
			if (*temp == f)
			{
				*temp = f->next;
				f->next = NULL;
				flag = 0;
				break;
			}
			temp = &(*temp)->next;
		}
		
		if (flag)
		{
			ALERT (("Ext2-FS: remove open FILEPTR fail in: e_close (%li)", c->inode));
		}
		
		if (((f->flags & O_RWMODE) == O_WRONLY) || ((f->flags & O_RWMODE) == O_RDWR))
			ext2_discard_prealloc (c);
		
		rel_cookie (c);
	}
	
	bio_SYNC_DRV ((&bio), super [f->fc.dev]->di);
	
	DEBUG (("Ext2-FS: e_close: leave #%li", c->inode));
	return E_OK;
}

INLINE void
remove_suid (COOKIE *inode)
{
# if 0
	ushort i_mode = le2cpu16 (inode->in.i_mode);
	ushort mode;
	
	/* set S_IGID if S_IXGRP is set, and always set S_ISUID */
	mode = (i_mode & S_IXGRP)* (S_ISGID / S_IXGRP) | S_ISUID;
	
	/* was any of the uid bits set? */
	mode &= i_mode;
	if (mode && !capable (CAP_FSETID))
	{
		inode->in.i_mode = cpu2le16 (i_mode & ~mode);
		mark_inode_dirty (inode);
	}
# endif
}

static long _cdecl
e_write (FILEPTR *f, const char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	SI *s = super [f->fc.dev];
	
	ulong block;
	ulong offset;
	
	long written;
	long todo;
	long pos;
	
	long err;
	
	
	DEBUG (("Ext2-FS [%c]: e_write: enter (#%li: %li)", DriveToLetter(f->fc.dev), c->inode, bytes));
	
	/* POSIX: mtime/ctime may not change for 0 count */
	if (bytes <= 0)
		return 0;
	
	if ((s->s_flags & MS_RDONLY) || IS_IMMUTABLE (c))
		return 0;
	
	remove_suid (c);
	
	if (IS_APPEND (c))	pos = c->i_size;
	else			pos = f->pos;
	
	block = pos >> EXT2_BLOCK_SIZE_BITS (s);
	offset = pos & EXT2_BLOCK_SIZE_MASK (s);
	
	written = 0;
	todo = bytes;
	
	/* partial block copy
	 */
	if (offset)
	{
		ulong tmp;
		UNIT *u;
		ulong data;
		
		tmp = ext2_getblk (c, block++, &err, 1);
		if (!tmp)
		{
			DEBUG (("Ext2-FS: partial part: ext2_getblk failure (err = %li)", err));
			goto out;
		}
		
		u = bio.read (s->di, tmp, EXT2_BLOCK_SIZE (s));
		if (!u)
		{
			DEBUG (("Ext2-FS: partial part: bio.read failure"));
			goto out;
		}
		
		data = EXT2_BLOCK_SIZE (s) - offset;
		data = MIN (todo, data);
		
		memcpy (u->data + offset, buf, data);
		bio_MARK_MODIFIED (&bio, u);
		
		buf += data;
		todo -= data;
		written += data;
		pos += data;
	}
	
	/* full blocks
	 */
	while (todo >> EXT2_BLOCK_SIZE_BITS (s))
	{
		long blocks = 1;
		long data = EXT2_BLOCK_SIZE (s);
		ulong tmp;
		
		tmp = ext2_getblk (c, block++, &err, 0);
		if (!tmp)
			goto out;
		
		if ((todo - data) >= EXT2_BLOCK_SIZE (s))
		{
			register ulong tmp_new = ext2_getblk (c, block, &err, 0);
			register long tmp_old = tmp;
			
			/* linear write optimization
			 */
			while (tmp_new && ((tmp_new - tmp_old) == 1))
			{
				data += EXT2_BLOCK_SIZE (s);
				block++;
				blocks++;
				
				if ((todo - data) >= EXT2_BLOCK_SIZE (s))
				{
					tmp_old = tmp_new;
					tmp_new = ext2_getblk (c, block, &err, 0);
				}
				else
					break;
			}
		}
		
		err = bio.l_write (s->di, tmp, blocks, EXT2_BLOCK_SIZE (s), buf);
		if (err)
		{
			DEBUG (("Ext2-FS: block part: bio.l_write failure (err = %li)", err));
			goto out;
		}
		
		buf += data;
		todo -= data;
		written += data;
		pos += data;
	}
	
	DEBUG (("todo = %li", todo));
	
	/* anything left?
	 */
	if (todo)
	{
		ulong tmp;
		UNIT *u;
		
		tmp = ext2_getblk (c, block, &err, 1);
		if (!tmp)
		{
			DEBUG (("Ext2-FS: left part: ext2_getblk failure (err = %li)", err));
			goto out;
		}
		
		u = bio.read (s->di, tmp, EXT2_BLOCK_SIZE (s));
		if (!u)
		{
			DEBUG (("Ext2-FS: left part: bio.read failure"));
			goto out;
		}
		
		memcpy (u->data, buf, todo);
		bio_MARK_MODIFIED (&bio, u);
		
		written += todo;
		pos += todo;
	}
	
out:
	if (pos > c->i_size)
	{
		c->i_size = pos;
	}

	if (pos > le2cpu32(c->in.i_size)) 
	{
		c->in.i_size = cpu2le32(pos);
	}

	c->in.i_ctime = c->in.i_mtime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (c);
	
	f->pos = pos;
	
	DEBUG (("Ext2-FS [%c]: e_write: leave (#%li: pos = %li, written = %li)", DriveToLetter(f->fc.dev), c->inode, f->pos, written));
	return written;
}

static long _cdecl
e_read (FILEPTR *f, char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	SI *s = super [f->fc.dev];
	
	long todo;		/* characters remaining */
	long done;		/* characters processed */

	ulong block = f->pos >> EXT2_BLOCK_SIZE_BITS (s);
	ulong offset = f->pos & EXT2_BLOCK_SIZE_MASK (s);

	DEBUG (("Ext2-FS [%c]: e_read: enter (#%li: pos = %li, bytes = %li [%lu, %lu])", DriveToLetter(f->fc.dev), c->inode, f->pos, bytes, block, offset));

	if (EXT2_ISDIR (le2cpu16 (c->in.i_mode)))
		return EISDIR;
	
	todo = MAX(0, MIN ((long)(c->i_size - f->pos), bytes));
	done = 0;
	
	if (todo == 0)
	{
		DEBUG (("Ext2-FS [%c]: e_read: failure (bytes = %li, todo = 0)", DriveToLetter(f->fc.dev), bytes));
		return 0;
	}
	else if (todo < 0)
	{
		/* hmm, Draco's idea */
		
		todo = 2147483647L; /* LONG_MAX */
		DEBUG (("Ext2-FS [%c]: e_read: negative fix (todo = %li)", DriveToLetter(f->fc.dev), todo));
	}
	
	/* partial block copy
	 */
	if (offset)
	{
		UNIT *u;
		ulong data;
		
		u = ext2_read (c, block++, (long *)&data);
		if (!u && data)
		{
			DEBUG (("Ext2-FS: partial part: ext2_read failure (r = %li)", data));
			goto out;
		}
		
		data = EXT2_BLOCK_SIZE (s) - offset;
		data = MIN (todo, data);
		
		if (u)	memcpy (buf, u->data + offset, data);
		else	bzero (buf, data);
		
		buf += data;
		todo -= data;
		done += data;
		f->pos += data;
	}
	
	/* full blocks
	 */
	while (todo >> EXT2_BLOCK_SIZE_BITS (s))
	{
		long blocks = 1;
		long data = EXT2_BLOCK_SIZE (s);
		ulong tmp;
		
		tmp = ext2_bmap (c, block++);
		if (tmp)
		{
			long r;
			
			if ((todo - data) >= EXT2_BLOCK_SIZE (s))
			{
				register ulong tmp_new = ext2_bmap (c, block);
				register long tmp_old = tmp;
				
				/* linear read optimization
				 */
				while (tmp_new && ((tmp_new - tmp_old) == 1))
				{
					data += EXT2_BLOCK_SIZE (s);
					block++;
					blocks++;
					
					if ((todo - data) >= EXT2_BLOCK_SIZE (s))
					{
						tmp_old = tmp_new;
						tmp_new = ext2_bmap (c, block);
					}
					else
						break;
				}
			}
			
			r = bio.l_read (s->di, tmp, blocks, EXT2_BLOCK_SIZE (s), buf);
			if (r)
			{
				DEBUG (("Ext2-FS: blocks part: bio.l_read failure (r = %li)", r));
				goto out;
			}
		}
		else
			bzero (buf, data);
		
		buf += data;
		todo -= data;
		done += data;
		f->pos += data;
	}
	
	DEBUG (("todo = %li", todo));
	
	/* anything left?
	 */
	if (todo)
	{
		UNIT *u;
		long r;
		
		u = ext2_read (c, block, &r);
		if (!u && r)
		{
			DEBUG (("Ext2-FS: left part: ext2_read failure (r = %li)", r));
			goto out;
		}
		
		if (u)	memcpy (buf, u->data, todo);
		else	bzero (buf, todo);
		
		done += todo;
		f->pos += todo;
	}
	
out:
	if (!((f->flags & O_NOATIME) 
	     || (s->s_flags & MS_NOATIME) 
	     || (s->s_flags & MS_RDONLY) 
	     || IS_IMMUTABLE (c)))
	{
		c->in.i_atime = cpu2le32 (CURRENT_TIME);
		mark_inode_dirty (c);
	}
	
	DEBUG (("Ext2-FS [%c]: e_read: leave (#%li: pos = %li, done = %li)", DriveToLetter(f->fc.dev), c->inode, f->pos, done));
	return done;
}

static long _cdecl
e_lseek (FILEPTR *f, long where, int whence)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG (("Ext2-FS [%c]: e_lseek: enter (#%li, where = %li, whence = %i)", DriveToLetter(f->fc.dev), c->inode, where, whence));
	
	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
		case SEEK_END:	where += c->i_size;	break;
		default:	return EINVAL;
	}
	
	if (where < 0)
	{
		DEBUG (("Ext2-FS [%c]: e_lseek: EBADARG", DriveToLetter(f->fc.dev)));
		return EBADARG;
	}
	
	f->pos = where;
	
	DEBUG (("Ext2-FS [%c]: e_lseek: leave (#%li, %li)", DriveToLetter(f->fc.dev), c->inode, where));
	return where;
}

static long _cdecl
e_ioctl (FILEPTR *f, int mode, void *arg)
{
	switch (mode)
	{
		case FIONREAD:
		{
			COOKIE *c = (COOKIE *) f->fc.index;
			long read = c->i_size - f->pos;
			
			DEBUG (("Ext2-FS: e_ioctl (FIONREAD)"));
			
			if (read < 0)
				read = 0;
			
			*(long *) arg = read;
			return E_OK;
		}
		case FIONWRITE:
		{
			DEBUG (("Ext2-FS: e_ioctl (FIONWRITE)"));
			
			*(long *) arg = 1;
			return E_OK;
		}
		case FIOEXCEPT:
		{
			DEBUG (("Ext2-FS: e_ioctl (FIOEXCEPT)"));
			
			*(long *) arg = 0;
			return E_OK;
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			COOKIE *c = (COOKIE *) f->fc.index;
			int uid;
			
			DEBUG (("Ext2-FS [%c]: e_ioctl (FUTIME%s) on #%li", DriveToLetter(c->dev), ((mode == FUTIME) ? "" : "_UTC"), c->inode));
			
			/* The owner or super-user can always touch, others only
			 * if timeptr == 0 and open for writing
			 */
			uid = p_geteuid ();
			if (uid && uid != le2cpu16 (c->in.i_uid)
				&& (arg || ((f->flags & O_RWMODE) == O_RDONLY)))
			{
				return EACCES;
			}
			
			if (c->s->s_flags & MS_RDONLY)
				return EROFS;
			
			if (IS_IMMUTABLE (c))
				return EACCES;
			
			c->in.i_ctime = cpu2le32 (CURRENT_TIME);
			
			if (arg)
			{
				if (native_utc || (mode == FUTIME_UTC))
				{
					long *timeptr = arg;
					
					c->in.i_atime = cpu2le32 (timeptr[0]);
					c->in.i_mtime = cpu2le32 (timeptr[1]);
				}
				else
				{
					MUTIMBUF *buf = arg;
					
					c->in.i_atime = cpu2le32 (unixtime (buf->actime, buf->acdate));
					c->in.i_mtime = cpu2le32 (unixtime (buf->modtime, buf->moddate));
				}
			}
			else
			{
				c->in.i_mtime = c->in.i_atime = c->in.i_ctime;
			}
			
			mark_inode_dirty (c);
			
			bio_SYNC_DRV ((&bio), c->s->di);
			return E_OK;
		}
		case FTRUNCATE:
		{
			COOKIE *c = (COOKIE *) f->fc.index;
			long pos;
			
			DEBUG (("Ext2-FS [%c]: e_ioctl (FTRUNCATE) on #%li", DriveToLetter(c->dev), c->inode));
			
			if (c->s->s_flags & MS_RDONLY)
				return EROFS;
			
			if (IS_IMMUTABLE (c))
				return EACCES;
			
			if ((f->flags & O_RWMODE) == O_RDONLY)
				return EACCES;
			
			if (c->i_size < *((long *) arg))
				return EACCES;
			
			ext2_truncate (c, *(unsigned long *)arg);
			
			pos = f->pos;
			(void) e_lseek (f, 0, SEEK_SET);
			(void) e_lseek (f, pos, SEEK_SET);
			
			bio_SYNC_DRV ((&bio), c->s->di);
			return E_OK;
		}
		case FIBMAP:
		{
			COOKIE *c = (COOKIE *) f->fc.index;
			long block;
			
			DEBUG (("Ext2-FS: e_ioctl (FIBMAP)"));
			
			if (!arg)
				return EINVAL;
			
			block = *(long *) arg;
			*(long *) arg = ext2_bmap (c, block);
			
			return E_OK;
		}
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			COOKIE *c = (COOKIE *) f->fc.index;
			struct flock *fl = (struct flock *) arg;
			
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
					long r = e_lseek (f, 0L, SEEK_CUR);
					t.l.l_start += r;
					break;
				}
				case SEEK_END:
				{
					long r = e_lseek (f, 0L, SEEK_CUR);
					t.l.l_start = e_lseek (f, t.l.l_start, SEEK_END);
					(void) e_lseek (f, r, SEEK_SET);
					break;
				}
				default:
				{
					DEBUG (("e_ioctl: invalid value for l_whence"));
					return ENOSYS;
				}
			}
			
			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;
			
			cpid = p_getpid ();
			
			if (mode == F_GETLK)
			{
				lck = denylock (cpid, c->locks, &t);
				if (lck)
				{
					*fl = lck->l;
				}
				else
				{
					fl->l_type = F_UNLCK;
				}
				
				return E_OK;
			}
			
			if (t.l.l_type == F_UNLCK)
			{
				/* try to find the lock */
				LOCK **lckptr = &(c->locks);
				
				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
		                                && ((lck->l.l_start == t.l.l_start
						     && lck->l.l_len == t.l.l_len) ||
						    (lck->l.l_start >= t.l.l_start
						     && t.l.l_len == 0)))
					{
						/* found it -- remove the lock */
						*lckptr = lck->next;
						DEBUG (("e_ioctl: unlocked #%li: %ld + %ld", c->inode, t.l.l_start, t.l.l_len));
						/* (void) e_lock (f, 1, t.l.l_start, t.l.l_len); */
						
						/* wake up anyone waiting on the lock */
						wake (IO_Q, (long) lck);
						kfree (lck, sizeof (*lck));
						
						return E_OK;
					}
					
					lckptr = &(lck->next);
					lck = lck->next;
				}
				
				return ENSLOCK;
			}
			
			DEBUG (("e_ioctl: lock #%li: %ld + %ld", c->inode, t.l.l_start, t.l.l_len));
			
			do {
				long r;
				
				/* see if there's a conflicting lock */
				while ((lck = denylock (cpid, c->locks, &t)) != 0)
				{
					DEBUG (("e_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
					if (mode == F_SETLKW)
					{
						/* sleep a while */
						sleep (IO_Q, (long) lck);
					}
					else
					{
						return ELOCKED;
					}
				}
				
				/* if not, add this lock to the list */
				lck = kmalloc (sizeof (*lck));
				if (!lck)
				{
					ALERT (("Ext2-FS: kmalloc fail in: e_ioctl: #%li", c->inode));
					return ENOMEM;
				}
				
				r = E_OK; /* e_lock (f, 0, t.l.l_start, t.l.l_len); */
				if (r)
				{
					kfree (lck, sizeof (*lck));
					if (mode == F_SETLKW && r == ELOCKED)
					{
						sleep (READY_Q, 0L);
						lck = NULL;
					}
					else
					{
						return r;
					}
				}
			}
			while (!lck);
			
			lck->l = t.l;
			lck->l.l_pid = cpid;
			lck->next = c->locks;
			c->locks = lck;
			
			/* mark the file as being locked */
			f->flags |= O_LOCK;
			
			return E_OK;
		}
		
		/* ext2 specials
		 */
		
		case EXT2_IOC_GETFLAGS:
		{
			COOKIE *inode = (COOKIE *) f->fc.index;
			
			*(ulong *) arg = le2cpu32 (inode->in.i_flags) & EXT2_FL_USER_VISIBLE;
			
			return E_OK;
		}
		case EXT2_IOC_SETFLAGS:
		{
			COOKIE *inode = (COOKIE *) f->fc.index;
			ulong flags;
			
			flags = (*(ulong *) arg) & EXT2_FL_USER_MODIFIABLE;
			
			/* The IMMUTABLE and APPEND_ONLY flags can only be changed by
			 * the super user when the security level is zero.
			 */
			/* if ((flags & (EXT2_APPEND_FL | EXT2_IMMUTABLE_FL)) ^
				(le2cpu32 (inode->in.i_flags) & (EXT2_APPEND_FL | EXT2_IMMUTABLE_FL)))
			{
				if (!capable (CAP_LINUX_IMMUTABLE))
					return EACCES;
			}
			else */
			{
				int euid;
				
				euid = p_geteuid ();
				if (euid && (euid != le2cpu16 (inode->in.i_uid)) /*&& !capable (CAP_FOWNER)*/)
					return EACCES;
			}
			
			if (inode->s->s_flags & MS_RDONLY)
				return EROFS;
			
			if (flags & EXT2_SYNC_FL)
				inode->i_flags |= MS_SYNCHRONOUS;
			else
				inode->i_flags &= ~MS_SYNCHRONOUS;
			
			if (flags & EXT2_APPEND_FL)
				inode->i_flags |= S_APPEND;
			else
				inode->i_flags &= ~S_APPEND;
			
			if (flags & EXT2_IMMUTABLE_FL)
				inode->i_flags |= S_IMMUTABLE;
			else
				inode->i_flags &= ~S_IMMUTABLE;
			
			if (flags & EXT2_NOATIME_FL)
				inode->i_flags |= MS_NOATIME;
			else
				inode->i_flags &= ~MS_NOATIME;
			
			inode->in.i_flags = le2cpu32 (inode->in.i_flags);
			inode->in.i_flags = (inode->in.i_flags & ~EXT2_FL_USER_MODIFIABLE) | flags;
			inode->in.i_flags = cpu2le32 (inode->in.i_flags);
			
			inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
			mark_inode_dirty (inode);
			
			bio_SYNC_DRV ((&bio), inode->s->di);
			return E_OK;
		}
		case EXT2_IOC_GETVERSION:
		{
			COOKIE *inode = (COOKIE *) f->fc.index;
			
			*(long *) arg = le2cpu32 (inode->in.i_version);
			
			return E_OK;
		}
		case EXT2_IOC_SETVERSION:
		{
			COOKIE *inode = (COOKIE *) f->fc.index;
			
			if ((p_geteuid () != le2cpu16 (inode->in.i_uid)) /*&& !capable (CAP_FOWNER)*/)
				return EACCES;
			
			if (inode->s->s_flags & MS_RDONLY)
				return EROFS;
			
			inode->in.i_version = cpu2le32 (*(long *) arg);
			inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
			mark_inode_dirty (inode);
			
			bio_SYNC_DRV ((&bio), inode->s->di);
			return E_OK;
		}
	}
	
	DEBUG (("Ext2-FS: e_ioctl: unknown mode (0x%x) -> ENOSYS", mode));
	return ENOSYS;
}

static long _cdecl
e_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	DEBUG (("Ext2-FS [%c]: e_datime (#%li : %i)", DriveToLetter(f->fc.dev), c->inode, flag));
	
	switch (flag)
	{
		case 0:
		{
			if (native_utc)
				*((long *) timeptr) = le2cpu32 (c->in.i_mtime);
			else
				*((long *) timeptr) = dostime (le2cpu32 (c->in.i_mtime));
			
			break;
		}
		case 1:
		{
			if (c->s->s_flags & MS_RDONLY)
				return EROFS;
			
			if (IS_IMMUTABLE (c))
				return EACCES;
			
			if (native_utc)
				c->in.i_mtime = cpu2le32 (*((long *) timeptr));
			else
				c->in.i_mtime = cpu2le32 (unixtime (timeptr[0], timeptr[1]));
			
			c->in.i_atime = c->in.i_mtime;
			c->in.i_ctime = cpu2le32 (CURRENT_TIME);
			mark_inode_dirty (c);
			
			bio_SYNC_DRV ((&bio), c->s->di);
			break;
		}
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl
e_select (FILEPTR *f, long proc, int mode)
{
	DEBUG (("Ext2-FS [%c]: e_select #%li: -> return 1", DriveToLetter(f->fc.dev), ((COOKIE *) f->fc.index)->inode));
	return 1;
}

static void _cdecl
e_unselect (FILEPTR *f, long proc, int mode)
{
	DEBUG (("Ext2-FS [%c]: e_unselect #%li: -> nop", DriveToLetter(f->fc.dev), ((COOKIE *) f->fc.index)->inode));
}
