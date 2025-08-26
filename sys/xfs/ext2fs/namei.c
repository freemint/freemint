/*
 * Filename:     namei.c
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

# include "namei.h"

# include <mint/endian.h>

# include "inode.h"
# include "super.h"


# if 1
# define CACHE_DIR_HASHBITS	(9)
# else
# define CACHE_DIR_HASHBITS	(12)
# endif
# define CACHE_DIR_SIZE		(1UL << CACHE_DIR_HASHBITS)
# define CACHE_DIR_HASHMASK	(CACHE_DIR_SIZE - 1)

static _DIR cache_dir [CACHE_DIR_SIZE];
static _DIR * dtable [CACHE_DIR_SIZE];

/* ?
 *	prevhash = (prevhash << 4) | (prevhash >> 28);
 *	return prevhash ^ TOUPPER (c);
 */

INLINE ulong
d_hash_hash (register const char *s)
{
	register ulong hash = 0;

	while (*s)
	{
		hash = ((hash << 5) - hash) + TOUPPER ((int)*s & 0xff);
		s++;
	}

	hash ^= (hash >> CACHE_DIR_HASHBITS) ^ (hash >> (CACHE_DIR_HASHBITS << 1));

	return hash & CACHE_DIR_HASHMASK;
}

INLINE long
d_hash_setup (_DIR *dentry, const char *name, long namelen)
{
	register ulong hash = 0;
	register char *s;

	dentry->namelen = namelen;
	dentry->name = s = kmalloc (namelen + 1);

	if (!s)
		return ENOMEM;

	while (namelen--)
	{
		*s = *name++;
		hash = ((hash << 5) - hash) + TOUPPER (*s);
		s++;
	}

	*s = '\0';

	hash ^= (hash >> CACHE_DIR_HASHBITS) ^ (hash >> (CACHE_DIR_HASHBITS << 1));

	dentry->hash = (hash & CACHE_DIR_HASHMASK);

	DEBUG (("Ext2-FS: %s, %i, %i", dentry->name, dentry->namelen, dentry->hash));
	return E_OK;
}

INLINE _DIR *
d_hash_lookup (register const char *s, register ulong parent, register ushort dev)
{
	register _DIR *dentry;

	for (dentry = dtable [d_hash_hash (s)]; dentry != NULL; dentry = dentry->next)
	{
		if ((dentry->dev == dev)
			&& (dentry->parent == parent)
			&& (strcmp (dentry->name, s) == 0))
		{
			DEBUG (("d_hash_lookup: match (s = %s, dentry->name = %s)!", s, dentry->name));
			return dentry;
		}
	}

	DEBUG (("d_hash_lookup: fail (%li, %s fail)!", d_hash_hash (s), s));
	return NULL;
}

INLINE void
d_hash_install (register _DIR *dentry)
{
	register const ulong hashval = dentry->hash;

	DEBUG (("c_hash_install: %i, %s", dentry->hash, dentry->name));

	dentry->next = dtable [hashval];
	dtable [hashval] = dentry;
}

INLINE void
d_hash_remove  (register _DIR *dentry)
{
	register const ulong hashval = dentry->hash;
	register _DIR **temp = & dtable [hashval];
	register long flag = 1;

	while (*temp)
	{
		if (*temp == dentry)
		{
			*temp = dentry->next;
			flag = 0;
			break;
		}
		temp = &(*temp)->next;
	}

	if (flag)
	{
		ALERT (("Ext2-FS: remove from hashtable fail in: d_hash_remove (addr = %p, %s)", dentry, dentry->name));
	}
}

_DIR *
d_lookup (COOKIE *dir, const char *name)
{
	return d_hash_lookup (name, dir->inode, dir->dev);
}

_DIR *
d_get_dir (COOKIE *dir, ulong inode, const char *name, long namelen)
{
	register long i;

	for (i = 0; i < CACHE_DIR_SIZE; i++)
	{
		static long count = 0;
		register _DIR *dentry;

		count++;
		if (count == CACHE_DIR_SIZE)
			count = 0;

		dentry = &(cache_dir [count]);

		d_del_dir (dentry);

		if (d_hash_setup (dentry, name, namelen))
			return NULL;

		d_hash_install (dentry);

		dentry->parent = dir->inode;
		dentry->inode = inode;
		dentry->dev = dir->dev;

		return dentry;
	}

	ALERT (("Ext2-FS: d_get_dir: no free _DIR found for %s", name));
	return NULL;
}

void
d_del_dir (_DIR *dentry)
{
	if (dentry->name)
	{
		d_hash_remove (dentry);

		kfree (dentry->name, dentry->namelen + 1);
		dentry->name = NULL;
	}
}

void
d_inv_dir (ushort dev)
{
	register long i;

	for (i = 0; i < CACHE_DIR_SIZE; i++)
	{
		_DIR *dentry;

		dentry = &(cache_dir [i]);
		if (dentry->dev == dev)
		{
			if (dentry->name)
				d_del_dir (dentry);
		}
	}
}

# ifdef EXT2FS_DEBUG
void
d_verify_clean (ushort dev, ulong parent)
{
	register long i;

	for (i = 0; i < CACHE_DIR_SIZE; i++)
	{
		_DIR *dentry = &(cache_dir [i]);

		if ((dentry->dev == dev)
			&& (dentry->parent == parent))
		{
			if (dentry->name)
			{
				ALERT (("Ext2-FS [%c]: d_verify_clean: entry found for #%li (#%li: %s)!", DriveToLetter(dev), parent, dentry->inode, dentry->name));
				d_del_dir (dentry);
			}
		}
	}
}

_DIR *
d_lookup_cookie (COOKIE *c, long *i)
{
	ASSERT (((*i) >= 0));

	while ((*i) < CACHE_DIR_SIZE)
	{
		_DIR *dentry = &(cache_dir [(*i)]);

		(*i)++;

		if ((dentry->dev == c->dev)
			&& (dentry->inode == c->inode))
		{
			if (dentry->name)
				return dentry;
		}
	}

	return NULL;
}

void
dump_dir_cache (char *buf, long bufsize)
{
	char tmp [128 /*SPRINTF_MAX*/];
	long len;
	long i;

	bufsize--;
	if (bufsize <= 0)
		return;

	ksprintf (tmp, "\n\n--- directory cache ---\n");
	len = MIN (strlen (tmp), bufsize);
	strncpy (buf, tmp, len);
	buf += len;
	bufsize -= len;

	for (i = 0; i < CACHE_DIR_SIZE; i++)
	{
		_DIR *d = &(cache_dir [i]);

		ksprintf (tmp, "%3li: [%c] - #%06li", i, DriveToLetter(d->dev), d->inode);
		len = MIN (strlen (tmp), bufsize);
		strncpy (buf, tmp, len);
		buf += len;
		bufsize -= len;

		ksprintf (tmp, " - #%06li - %4i - %s\n", d->parent, d->hash, d->name);
		len = MIN (strlen (tmp), bufsize);
		strncpy (buf, tmp, len);
		buf += len;
		bufsize -= len;

		if (bufsize <= 0)
			break;
	}

	*buf = '\0';
}
# endif

long
ext2_check_dir_entry (const char *caller, COOKIE *dir, ext2_d2 *de, UNIT *u, ulong offset)
{
	const char *error_msg = NULL;
	register ushort rec_len = le2cpu16 (de->rec_len);

	if (rec_len < EXT2_DIR_REC_LEN (1))
		error_msg = "rec_len is smaller than minimal";
	else if (rec_len % 4 != 0)
		error_msg = "rec_len % 4 != 0";
	else if (rec_len < EXT2_DIR_REC_LEN (de->name_len))
		error_msg = "rec_len is too small for name_len";
	else if (dir && (ulong)de - (ulong)u->data + rec_len > EXT2_BLOCK_SIZE(dir->s)) // ((char *)de - (char *)u->data) + rec_len > EXT2_BLOCK_SIZE(dir->s))
		error_msg = "directory entry across blocks";
	else if (dir && le2cpu32 (de->inode) > le2cpu32 (dir->s->sbi.s_sb->s_inodes_count))
		error_msg = "inode out of bounds";

	if (error_msg)
	{
		ALERT (("Ext2-FS: %s: bad entry in directory #%lu: %s - "
			"offset=%lu, inode=%lu, rec_len=%d, name_len=%d",
			caller, dir->inode, error_msg, offset,
			le2cpu32 (de->inode), rec_len, de->name_len));

		return 0;
	}

	return 1;
}

/* returns 1 for success, 0 for failure.
 *
 * `length <= EXT2_NAME_LEN' is guaranteed by caller.
 * `de != NULL' is guaranteed by caller.
 */

INLINE long
ext2_match (long length, const char *name, ext2_d2 *de)
{
	if (length != de->name_len)
		return 0;

	if (de->inode == 0)
		return 0;

	return !memcmp (name, de->name, length);
}


_DIR *
ext2_search_entry (COOKIE *dir, const char *name, long namelen)
{
	SI *s = super [dir->dev];
	ulong size = le2cpu32 (dir->in.i_size);

	long block;
	long offset;

	_DIR *dentry;

	DEBUG (("Ext2-FS [%c]: ext2_search_entry (%li, %s)", DriveToLetter(dir->dev), namelen, name));

	if (namelen == 0)
		return NULL;
	/* 1. search in directory cache
	 */
	dentry = d_lookup (dir, name);
	if (dentry)
		return dentry;

	/* 2. check lastlookup fail cache
	 */
	if (dir->lastlookup)
	{
		if ((namelen == dir->lastlookupsize)
			&& (strcmp (name, dir->lastlookup) == 0))
		{
			DEBUG (("ext2_search_entry: leave not found (lastlookup) (name = %s)", name));
			return NULL;
		}
	}

	/* 3. search on disk
	 */
	for (block = 0, offset = 0; offset < size; block++)
	{
		UNIT *u;
		ext2_d2 *de;
		char *upper;

		u = ext2_read (dir, block, NULL);
		if (!u)
		{
			DEBUG (("Ext2-FS: ext2_search_entry: ext2_read fail (%li)", block));
			goto failure;
		}

		de = (ext2_d2 *) u->data;

		upper = (char *) de + EXT2_BLOCK_SIZE (s);
		while ((char *) de < upper)
		{
			/* this code is executed quadratically often
			 * do minimal checking `by hand'
			 */
			long de_len;

			if ((char *) de + namelen <= upper && ext2_match (namelen, name, de))
			{
				/* found a match -
				 * just to be sure, do a full check
				 */
				if (!ext2_check_dir_entry ("ext2_search_entry", dir, de, u, offset))
					goto failure;

				dentry = d_get_dir (dir, le2cpu32 (de->inode), de->name, de->name_len);
				if (dentry)
				{
					DEBUG (("Ext2-FS: ext2_search_entry: found (%i, %s)", dentry->namelen, dentry->name));
				}
				else
				{
					DEBUG (("Ext2-FS: ext2_search_entry: found but d_get_dir fail?"));
				}

				return dentry;
			}

			de_len = le2cpu16 (de->rec_len);
			if (de_len <= 0)
				goto failure;

			offset += de_len;
			de = (ext2_d2 *) ((char *) de + de_len);
		}
	}

failure:
	/* 4. update lookup fail cache
	 */
	update_lastlookup (dir, name, namelen);


	DEBUG (("Ext2-FS [%c]: ext2_search_entry: leave not found", DriveToLetter(dir->dev)));
	return NULL;
}

UNIT *
ext2_search_entry_i (COOKIE *dir, long inode, ext2_d2 **res_dir)
{
	SI *s = super [dir->dev];
	ulong size = le2cpu32 (dir->in.i_size);

	long block;
	long offset;

	DEBUG (("Ext2-FS [%c]: ext2_search_entry_i: #%li in #%li", DriveToLetter(dir->dev), inode, dir->inode));

	for (block = 0, offset = 0; offset < size; block++)
	{
		UNIT *u;
		ext2_d2 *de;
		char *upper;

		u = ext2_read (dir, block, NULL);
		if (!u)
		{
			DEBUG (("Ext2-FS: ext2_search_entry_i: ext2_read fail (%li)", block));
			goto failure;
		}

		de = (ext2_d2 *) u->data;

		upper = (char *) de + EXT2_BLOCK_SIZE (s);
		while ((char *) de < upper)
		{
			/* this code is executed quadratically often
			 * do minimal checking `by hand'
			 */
			long de_len;

			if (inode == le2cpu32 (de->inode))
			{
				/* found a match -
				 * just to be sure, do a full check
				 */
				if (!ext2_check_dir_entry ("ext2_search_entry_i", dir, de, u, offset))
					goto failure;

				DEBUG (("Ext2-FS: ext2_search_entry_i: found (%s)", de->name));

				*res_dir = de;
				return u;
			}

			de_len = le2cpu16 (de->rec_len);
			if (de_len <= 0)
				goto failure;

			offset += de_len;
			de = (ext2_d2 *) ((char *) de + de_len);
		}
	}

failure:

	DEBUG (("Ext2-FS [%c]: ext2_search_entry_i: leave not found", DriveToLetter(dir->dev)));
	return NULL;
}

/*
 *	ext2_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 */
UNIT *
ext2_find_entry (COOKIE *dir, const char *name, long namelen, ext2_d2 **res_dir)
{
	SI *s;
	ulong size;

	long block;
	long offset;

	DEBUG (("Ext2-FS [%c]: ext2_find_entry (%li, %s)", DriveToLetter(dir->dev), namelen, name));

	*res_dir = NULL;
	if (!dir)
		return NULL;

	if (namelen == 0)
		return NULL;

	s = super [dir->dev];
	size = le2cpu32 (dir->in.i_size);

	for (block = 0, offset = 0; offset < size; block++)
	{
		UNIT *u;
		ext2_d2 *de;
		char *upper;

		u = ext2_read (dir, block, NULL);
		if (!u)
		{
			DEBUG (("Ext2-FS: ext2_find_entry: ext2_read fail (%li)", block));
			goto failure;
		}

		de = (ext2_d2 *) u->data;

		upper = (char *) de + EXT2_BLOCK_SIZE (s);
		while ((char *) de < upper)
		{
			/* this code is executed quadratically often
			 * do minimal checking `by hand'
			 */
			long de_len;

			if ((char *) de + namelen <= upper && ext2_match (namelen, name, de))
			{
				/* found a match -
				 * just to be sure, do a full check
				 */
				if (!ext2_check_dir_entry ("ext2_find_entry", dir, de, u, offset))
					goto failure;

				*res_dir = de;
				return u;
			}

			de_len = le2cpu16 (de->rec_len);
			if (de_len <= 0)
				goto failure;

			offset += de_len;
			de = (ext2_d2 *) ((char *) de + de_len);
		}
	}

failure:
	DEBUG (("Ext2-FS [%c]: ext2_find_entry: leave not found", DriveToLetter(dir->dev)));
	return NULL;
}

/*
 *	ext2_add_entry()
 *
 * adds a file entry to the specified directory, using the same
 * semantics as ext2_find_entry(). It returns NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */
UNIT *
ext2_add_entry (COOKIE *dir, const char *name, long namelen, ext2_d2 **res_dir, long *err)
{
	SI *s;
	UNIT *u;
	ext2_d2 *de, *de1;

	ulong offset;
	ushort rec_len;

	*res_dir = NULL;

	if (!dir || !dir->in.i_links_count)
	{
		*err = EINVAL;
		return NULL;
	}

	s = dir->s;

	if (namelen > EXT2_NAME_LEN)
	{
		*err = ENAMETOOLONG;
		return NULL;
	}

	if (!namelen)
	{
		*err = EACCES;
		return NULL;
	}

	/* Is this a busy deleted directory?
	 * Can't create new files if so
	 */
	if (!dir->in.i_size)
	{
		*err = ENOENT;
		return NULL;
	}

	u = ext2_read (dir, 0, err);
	if (!u)
	{
		*err = ENOSPC;
		return NULL;
	}

	rec_len = EXT2_DIR_REC_LEN (namelen);
	offset = 0;
	de = (ext2_d2 *) u->data;

	while (1)
	{
		if ((unsigned long)de >= (unsigned long)u->data + EXT2_BLOCK_SIZE(s)) // ((char *) de >= EXT2_BLOCK_SIZE (s) + u->data)
		{
			u = ext2_bread (dir, offset >> EXT2_BLOCK_SIZE_BITS (s), err);
			if (!u)
			{
				*err = ENOSPC;
				return NULL;
			}

			if (le2cpu32 (dir->in.i_size) <= offset)
			{
				if (!dir->in.i_size)
				{
					*err = ENOENT;
					return NULL;
				}

				DEBUG (("creating next block"));

				de = (ext2_d2 *) u->data;
				de->inode = 0;
				de->rec_len = cpu2le16 (EXT2_BLOCK_SIZE (s));
				dir->in.i_size = cpu2le32 (offset + EXT2_BLOCK_SIZE (s));
				dir->in.i_flags = cpu2le32 (le2cpu32 (dir->in.i_flags) & ~EXT2_BTREE_FL);
				mark_inode_dirty (dir);
			}
			else
			{
				DEBUG (("skipping to next block"));

				de = (ext2_d2 *) u->data;
			}
		}

		if (!ext2_check_dir_entry ("ext2_add_entry", dir, de, u, offset))
		{
			*err = ENOENT;
			return NULL;
		}

		if (ext2_match (namelen, name, de))
		{
			*err = EEXIST;
			return NULL;
		}

		if ((de->inode == 0 && le2cpu16 (de->rec_len) >= rec_len)
			|| (le2cpu16 (de->rec_len) >= EXT2_DIR_REC_LEN (de->name_len) + rec_len))
		{
			offset += le2cpu16 (de->rec_len);
			if (de->inode)
			{
				de1 = (ext2_d2 *) ((char *) de + EXT2_DIR_REC_LEN (de->name_len));
				de1->rec_len = cpu2le16 (le2cpu16 (de->rec_len) - EXT2_DIR_REC_LEN (de->name_len));
				de->rec_len = cpu2le16 (EXT2_DIR_REC_LEN (de->name_len));
				de = de1;
			}

			de->inode = 0;
			de->name_len = namelen;
			de->file_type = 0;
			memcpy (de->name, name, namelen);

			dir->in.i_mtime = dir->in.i_ctime = cpu2le32 (CURRENT_TIME);
			dir->in.i_flags = cpu2le32 (le2cpu32 (dir->in.i_flags) & ~EXT2_BTREE_FL);
			dir->in.i_version = cpu2le32 (++event);
			mark_inode_dirty (dir);

			bio_MARK_MODIFIED (&bio, u);

			clear_lastlookup (dir);

			*res_dir = de;
			*err = E_OK;

			return u;
		}

		offset += le2cpu16 (de->rec_len);
		de = (ext2_d2 *) ((char *) de + le2cpu16 (de->rec_len));
	}

	/* never reached */
	return NULL;
}

/*
 * ext2_delete_entry deletes a directory entry by merging it with the
 * previous entry
 */
long
ext2_delete_entry (ext2_d2 *dir, UNIT *u)
{
	ext2_d2 *de = (ext2_d2 *) u->data;
	ext2_d2 *pde = NULL;
	long i = 0;

	while (i < u->size)
	{
		if (!ext2_check_dir_entry ("ext2_delete_entry", NULL, de, u, i))
			return EIO;

		if (de == dir)
		{
			if (pde)
				pde->rec_len =
					cpu2le16 (le2cpu16 (pde->rec_len) +
						le2cpu16 (dir->rec_len));

			dir->inode = 0;

			bio_MARK_MODIFIED (&bio, u);
			return 0;
		}

		i += le2cpu16 (de->rec_len);
		pde = de;
		de = (ext2_d2 *) ((char *) de + le2cpu16 (de->rec_len));
	}

	return ENOENT;
}
