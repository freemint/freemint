/*
 * This file implements some sort of filename -> un_index conversion
 * cache. Speeds up for real FS's (where the inode numbers are NOT
 * junk) and slows down for the silly FS's where the inode numbers
 * are junk.
 *
 * 12/12/93, kay roemer.
 */

# include "ipc_unix_cache.h"

# include "mint/proc.h"
# include "mint/stat.h"
# include "libkern/libkern.h"

# include "dosdir.h"
# include "dosfile.h"
# include "ipc_unix.h"


# define CACHE_ENTRIES	10

struct lookup_cache
{
	short	valid;
# define CACHE_VALID	1
# define CACHE_DIRTY	0
	short	dev;
	long	inode;
	long	stamp;
# define MK_STAMP(time, date)	((((long)(date)) << 16) | (time))
	long	un_index;
};

static struct lookup_cache f_cache[CACHE_ENTRIES];

long
un_cache_lookup (char *name, long *index, XATTR *attr)
{
	short dirty_idx, i;
	long r, fd, stamp;

	static short last_deleted = 0;

	dirty_idx = -1;
	stamp = MK_STAMP (attr->mtime, attr->mdate);
	for (i = 0; i < CACHE_ENTRIES; i++)
	{
		if (f_cache[i].valid == CACHE_VALID &&
			f_cache[i].inode == attr->index &&
			f_cache[i].dev == attr->dev)
		{
			if (f_cache[i].stamp == stamp)
			{
				*index = f_cache[i].un_index;
				return 0;
			}
			f_cache[i].valid = CACHE_DIRTY;
			dirty_idx = i;
			break;
		}
	}

	if (dirty_idx < 0)
	{
		for (i = 0; i < CACHE_ENTRIES; i++)
		{
			if (f_cache[i].valid == CACHE_DIRTY)
			{
				dirty_idx = i;
				break;
			}
		}
	}

	if (dirty_idx < 0)
	{
		dirty_idx = last_deleted++;
		if (last_deleted >= CACHE_ENTRIES)
			last_deleted = 0;
	}

	/*
	 * read back the index that was wriiten in unix_bind
	 */
	fd = sys_f_open (name, O_RDONLY);
	if (fd < 0)
	{
		DEBUG (("unix: un_cache_lookup: Fopen(%s) -> %ld", name, fd));
		return fd;
	}
	r = sys_f_read (fd, sizeof (*index), (char *)index);
	sys_f_close(fd);
	if (r != sizeof(*index))
	{
		DEBUG (("unix: un_cache_lookup: could not read idx from file %s",
			name));
		return EACCES;
	}


	f_cache[dirty_idx].dev = attr->dev;
	f_cache[dirty_idx].inode = attr->index;
	f_cache[dirty_idx].stamp = stamp;
	f_cache[dirty_idx].un_index = *index;
	f_cache[dirty_idx].valid = CACHE_VALID;

	return 0;
}

long
un_cache_remove (char *name)
{
	XATTR attr;
	long r;
	short i;

	/*
	 * Should return failure.
	 */
	r = sys_f_xattr (1, name, &attr);
	if (r == 0)
	{
		DEBUG (("unix: un_cache_remove: Fxattr(%s), file exists", name));
		return EADDRINUSE;
	}

	for (i = 0; i < CACHE_ENTRIES; i++)
	{
		if (f_cache[i].valid == CACHE_VALID &&
			f_cache[i].inode == attr.index &&
			f_cache[i].dev == attr.dev)
		{
			f_cache[i].valid = CACHE_DIRTY;
			return 0;
		}
	}

	return 0;
}
