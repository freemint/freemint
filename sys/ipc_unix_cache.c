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
	long	cksum;
	long	un_index;
};

static struct lookup_cache f_cache[CACHE_ENTRIES];

/*
 * Generic checksum computation routine.
 * Returns the one's complement of the 16 bit one's complement sum over
 * the `nwords' words starting at `buf'.
 */
static long
chksum (void *buf, short nwords)
{
	long sum = 0;
	
	__asm__
	(
		"clrl	%%d0		\n\t"
#ifdef __mcoldfire__
		"mvzw	%2, %%d1		\n\t"
		"lsrl	#1, %%d1		\n\t"	/* # of longs in buf */
#else
		"movew	%2, %%d1		\n\t"
		"lsrw	#1, %%d1		\n\t"	/* # of longs in buf */
#endif
		"bcc	1f		\n\t"	/* multiple of 4 ? */
#ifdef __mcoldfire__
		"mvz.w	%1@+, %%d2	\n\t"
		"addl	%%d2, %0		\n\t"	/* no, add in extra word */
		"addxl	%%d0, %0		\n"
#else
		"addw	%1@+, %0	\n\t"	/* no, add in extra word */
		"addxw	%%d0, %0		\n"
#endif
		"1:			\n\t"
#ifdef __mcoldfire__
		"subql	#1, %%d1		\n\t"	/* decrement for dbeq */
#else
		"subqw	#1, %%d1		\n\t"	/* decrement for dbeq */
#endif
		"bmi	3f		\n"
		"2:			\n\t"
		"addl	%1@+, %0	\n\t"
		"addxl	%%d0, %0		\n\t"
#ifdef __mcoldfire__
		"subql	#1, %%d1		\n\t"
		"bpls	2b		\n"	/* loop over all longs */
#else
		"dbra	%%d1, 2b		\n"	/* loop over all longs */
#endif
		"3:			\n\t"
#ifdef __mcoldfire__
		"swap	%0		\n\t"	/* convert to short */
		"mvzw	%0, %%d1		\n\t"
		"clr.w	%0		\n\t"
		"swap	%0		\n\t"
		"addl	%%d1, %0		\n\t"
		"swap	%0		\n\t"
		"mvzw	%0, %%d1		\n\t"
		"clr.w	%0		\n\t"
		"swap	%0		\n\t"
		"addl	%%d1, %0		\n\t"
#else
		"movel	%0, %%d1		\n\t"	/* convert to short */
		"swap	%%d1		\n\t"
		"addw	%%d1, %0		\n\t"
		"addxw	%%d0, %0		\n\t"
#endif
		: "=d"(sum), "=a"(buf)
		: "g"(nwords), "1"(buf), "0"(sum)
#ifdef __mcoldfire__
		: "d0", "d1", "d2"
#else
		: "d0", "d1"
#endif
	);
	
	return ~sum;
}

long
un_cache_lookup (char *name, long *index)
{
	XATTR attr;
	short dirty_idx, i;
	long r, fd, stamp;
	long cksum;
	static short last_deleted = 0;

	r = sys_f_xattr (1, name, &attr);
	if (r)
	{
		DEBUG (("unix: un_cache_lookup: Fxattr(%s) -> %ld", name, r));
		return r;
	}

	dirty_idx = -1;
	stamp = MK_STAMP (attr.mtime, attr.mdate);
	for (i = 0; i < CACHE_ENTRIES; i++)
	{
		if (f_cache[i].valid == CACHE_VALID &&
			f_cache[i].inode == attr.index &&
			f_cache[i].dev == attr.dev)
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

	cksum = chksum(name,
		MIN((strlen(name) + 1), sizeof(struct sockaddr_un) - UN_PATH_OFFSET) >> 1);
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


	f_cache[dirty_idx].dev = attr.dev;
	f_cache[dirty_idx].inode = attr.index;
	f_cache[dirty_idx].stamp = stamp;
	f_cache[dirty_idx].cksum = cksum;
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
