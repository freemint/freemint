/* structure for Fxattr */
#ifndef __XATTR
#define __XATTR
typedef struct xattr		XATTR;

struct xattr
{
	ushort	mode;
	long	index;
	ushort	dev;
	ushort	rdev;		/* "real" device */
	ushort	nlink;
	ushort	uid;
	ushort	gid;
	long	st_size;
	long	blksize;
	long	nblocks;
	ushort	mtime, mdate;
	ushort	atime, adate;
	ushort	ctime, cdate;
	short	attr;
	short	reserved2;
	long	reserved3[2];
};
#endif
