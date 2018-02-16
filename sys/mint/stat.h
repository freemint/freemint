/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _mint_stat_h
# define _mint_stat_h

# ifdef __KERNEL__
# include "kcompiler.h"
# include "ktypes.h"
# else
# include <sys/types.h>
# endif
# include "time.h"


/* old stat structure */
struct xattr
{
	ushort	mode;
	long	index;
	ushort	dev;
	ushort	rdev;
	ushort	nlink;
	ushort	uid;
	ushort	gid;
	long	size;
	long	blksize;
	long	nblocks;
	ushort	mtime, mdate;
	ushort	atime, adate;
	ushort	ctime, cdate;
	short	attr;
	short	reserved2;
	long	reserved3[2];
};

/* structure for stat */
struct stat
{
	llong		dev;		/* inode's device */
	ulong		ino;		/* inode's number */
	ulong		mode;		/* inode protection mode */
	ulong		nlink;		/* number of hard links */
	ulong		uid;		/* user ID of the file's owner */
	ulong		gid;		/* group ID of the file's group */
	llong		rdev;		/* device type */
	struct time	atime;		/* time of last access, UTC */
	struct time	mtime;		/* time of last data modification, UTC */
	struct time	ctime;		/* time of last file status change, UTC */
	llong		size;		/* file size, in bytes */
	llong		blocks;		/* blocks allocated for file */
	ulong		blksize;	/* optimal blocksize for I/O */
	ulong		flags;		/* user defined flags for file */
	ulong		gen;		/* file generation number */
	
	long		res[7];		/* sizeof = 128 bytes */
};


/* file types */
# define S_IFMT		0170000		/* file type mask */
# define S_IFLNK	0160000		/* symbolic link */
# define S_IFMEM	0140000		/* memory region or process */
# define S_IFIFO	0120000		/* named pipe (fifo) */
# define S_IFREG 	0100000		/* regular file */
# define S_IFBLK	0060000		/* block special file */
# define S_IFDIR	0040000		/* directory file */
# define S_IFCHR	0020000		/* character special file */
# define S_IFSOCK	0010000		/* socket file */

# define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
# define S_ISMEM(m)	(((m) & S_IFMT) == S_IFMEM)
# define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
# define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
# define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
# define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
# define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
# define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

/* special bits: setuid, setgid, sticky bit */
# define S_ISUID	0004000		/* set user id on execution */
# define S_ISGID	0002000		/* set group id on execution */
# define S_ISVTX	0001000		/* sticky bit */

/* file access modes for user, group, and other */
# define S_IRWXU	0000700		/* RWX mask for owner */
# define S_IRUSR	0000400		/* R for owner */
# define S_IWUSR	0000200		/* W for owner */
# define S_IXUSR	0000100		/* X for owner */

# define S_IRWXG	0000070		/* RWX mask for group */
# define S_IRGRP	0000040		/* R for group */
# define S_IWGRP	0000020		/* W for group */
# define S_IXGRP	0000010		/* X for group */

# define S_IRWXO	0000007		/* RWX mask for other */
# define S_IROTH	0000004		/* R for other */
# define S_IWOTH	0000002		/* W for other */
# define S_IXOTH	0000001		/* X for other */

# define S_IRUGO	(S_IRUSR | S_IRGRP | S_IROTH)
# define S_IWUGO	(S_IWUSR | S_IWGRP | S_IWOTH)
# define S_IXUGO	(S_IXUSR | S_IXGRP | S_IXOTH)
# define S_IRWXUGO	(S_IRWXU | S_IRWXG | S_IRWXO)
# define S_IALLUGO	(S_ISUID | S_ISGID | S_ISVTX | S_IRWXUGO)

# define ACCESSPERMS	(S_IRWXUGO)
# define ALLPERMS	(S_IALLUGO)
# define DEFFILEMODE	(S_IRUGO | S_IWUGO)

# define S_BLKSIZE	512		/* block size used in the stat struct */


/*
 * Definitions of flags stored in file flags word.
 *
 * Super-user and owner changeable flags.
 */
# define UF_SETTABLE	0x0000ffff	/* mask of owner changeable flags */
# define UF_NODUMP	0x00000001	/* do not dump file */
# define UF_IMMUTABLE	0x00000002	/* file may not be changed */
# define UF_APPEND	0x00000004	/* writes to file may only append */
# define UF_OPAQUE	0x00000008	/* directory is opaque wrt. union */
/*
 * Super-user changeable flags.
 */
# define SF_SETTABLE	0xffff0000	/* mask of superuser changeable flags */
# define SF_ARCHIVED	0x00010000	/* file is archived */
# define SF_IMMUTABLE	0x00020000	/* file may not be changed */
# define SF_APPEND	0x00040000	/* writes to file may only append */

# ifdef __KERNEL__
# define OPAQUE		(UF_OPAQUE)
# define APPEND		(UF_APPEND | SF_APPEND)
# define IMMUTABLE	(UF_IMMUTABLE | SF_IMMUTABLE)
# endif /* __KERNEL__ */


# endif /* _mint_stat_h */
