/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 *  File : nfs_xdr.h
 *         definitions for dealing with networking file systems
 */


# ifndef _nfs_xdr_h
# define _nfs_xdr_h


# include "global.h"
# include "xdr.h"


/* request numbers
 */

# define NFSPROC_NULL		0
# define NFSPROC_GETATTR	1
# define NFSPROC_SETATTR	2
# define NFSPROC_ROOT		3	/* obsolete in NFS V2 */
# define NFSPROC_NOOP		3	/* used for error returns */
# define NFSPROC_LOOKUP		4
# define NFSPROC_READLINK	5
# define NFSPROC_READ		6
# define NFSPROC_WRITECACHE	7
# define NFSPROC_WRITE		8
# define NFSPROC_CREATE		9
# define NFSPROC_REMOVE		10
# define NFSPROC_RENAME		11
# define NFSPROC_LINK		12
# define NFSPROC_SYMLINK	13
# define NFSPROC_MKDIR		14
# define NFSPROC_RMDIR		15
# define NFSPROC_READDIR	16
# define NFSPROC_STATFS		17

# define NFS_PROGRAM		100003
# define NFS_VERSION		2
# define NFS_MAXPROC		17


# define MAXDATA	8192	/* max number of bytes for read and write */
# define MAXPATHLEN	1024	/* max number of bytes in a pathname argument */
# define MAXNAMLEN	255	/* max number of bytes in a file name argument */
# define COOKIESIZE	4	/* size of opaque "cookie" passed by READDIR */
# define FHSIZE		32	/* size in bytes of the opaque file handle */


/* data types for nfs, see also rfc 1094 (nfs)
 */
typedef enum nfsstat nfsstat;
enum nfsstat
{
	NFS_OK			= 0,
	NFSERR_PERM		= 1,
	NFSERR_NOENT		= 2,
	NFSERR_IO		= 5,
	NFSERR_NXIO		= 6,
	NFSERR_ACCES		= 13,
	NFSERR_EXIST		= 17,
	NFSERR_NODEV		= 19,
	NFSERR_NOTDIR		= 20,
	NFSERR_ISDIR		= 21,
	NFSERR_FBIG		= 27,
	NFSERR_NOSPC		= 28,
	NFSERR_ROFS		= 30,
	NFSERR_NAMETOOLONG	= 63,
	NFSERR_NOTEMPTY		= 66,
	NFSERR_DQUOT		= 69,
	NFSERR_STALE		= 70,
	NFSERR_WFLUSH		= 99
};

bool_t xdr_nfsstat (xdrs *x, nfsstat *statp);
long xdr_size_nfsstat (nfsstat *sp);


typedef enum ftype ftype;
enum ftype
{
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5
};


# define N_IFMT		0170000	/* file type access mask */
# define N_IFDIR	0040000	/* directory file */
# define N_IFCHR	0020000	/* character special file */
# define N_IFBLK	0060000	/* block special file */
# define N_IFREG	0100000	/* regular file */
# define N_IFLNK	0120000	/* symbolic link */
# define N_IFSCK	0140000	/* named socket file */


bool_t xdr_ftype (xdrs *x, ftype *ftypep);
# define xdr_size_ftype(statp)  (sizeof (ulong))


typedef opaque nfscookie [COOKIESIZE];

bool_t xdr_nfscookie (xdrs *x, nfscookie cookie);
# define xdr_size_nfscookie(cookie)  (sizeof (nfscookie))


typedef struct nfs_fh
{
	opaque data[FHSIZE];
} nfs_fh;

bool_t xdr_nfsfh(xdrs *x, nfs_fh *fhp);
#define xdr_size_nfsfh(fhp)   (sizeof(nfs_fh))


typedef struct nfstime nfstime;
struct nfstime
{
	ulong seconds;
	ulong useconds;
};

bool_t xdr_nfstime (xdrs *x, nfstime *timep);
# define xdr_size_nfstime(tp)   (sizeof (nfstime))


typedef struct fattr fattr;
struct fattr
{
	ftype	type;
	ulong	mode;
	ulong	nlink;
	ulong	uid;
	ulong	gid;
	ulong	size;
	ulong	blocksize;
	ulong	rdev;
	ulong	blocks;
	ulong	fsid;
	ulong	fileid;
	nfstime	atime;
	nfstime	mtime;
	nfstime	ctime;
};

bool_t xdr_fattr (xdrs *x, fattr *fp);
# define xdr_size_fattr(fattrp)	(sizeof (fattr) - sizeof (ftype) + sizeof (ulong))


typedef struct sattr sattr;
struct sattr
{
	ulong	mode;
	ulong	uid;
	ulong	gid;
	ulong	size;
	nfstime	atime;
	nfstime	mtime;
};

bool_t xdr_sattr (xdrs *x, sattr *sp);
# define xdr_size_sattr(sattrp)	(sizeof (sattr))



/* arguments for and results of nfs functions
 */

typedef struct attrstat attrstat;
struct attrstat
{
	enum_t	status;
	union
	{
		fattr	attributes;
	} attrstat_u;
};

bool_t xdr_attrstat (xdrs *x, attrstat *ap);
long xdr_size_attrstat (attrstat *ap);


typedef struct sattrargs sattrargs;
struct sattrargs
{
	nfs_fh	file;
	sattr	attributes;
};

bool_t xdr_sattrargs (xdrs *x, sattrargs *ap);
long xdr_size_sattrargs (sattrargs *ap);


typedef struct diropres diropres;
struct diropres
{
	enum_t	status;
	union
	{
		struct
		{
			nfs_fh	file;
			fattr	attributes;
		} diropok;
	} diropres_u;
};

bool_t xdr_diropres (xdrs *x, diropres *resp);
long xdr_size_diropres (diropres *resp);


typedef struct diropargs diropargs;
struct diropargs
{
	nfs_fh	dir;
	const char	*name;
};

bool_t xdr_diropargs (xdrs *x, diropargs *argp);
long xdr_size_diropargs (diropargs *argp);


typedef struct readlinkres readlinkres;
struct readlinkres
{
	enum_t	status;
	union
	{
		char *	data;
	} readlinkres_u;
};

bool_t xdr_readlinkres (xdrs *x, readlinkres *resp);
long xdr_size_readlinkres (readlinkres *resp);


typedef struct readres readres;
struct readres
{
	enum_t	status;
	union
	{
		struct
		{
			fattr	attributes;
			char *	data_val;
			ulong	data_len;
		} read_ok;
	} readres_u;
};

bool_t xdr_readres (xdrs *x, readres *resp);
long xdr_size_readres (readres *resp);


typedef struct readargs readargs;
struct readargs
{
	nfs_fh	file;
	ulong	offset;
	ulong	count;
	ulong	totalcount;
};

bool_t xdr_readargs (xdrs *x, readargs *argp);
long xdr_size_readargs (readargs *ap);


typedef struct writeargs writeargs;
struct writeargs
{
	nfs_fh	file;
	ulong	beginoffset;
	ulong	offset;
	ulong	totalcount;
	const char *	data_val;
	ulong	data_len;
};

bool_t xdr_writeargs (xdrs *x, writeargs *argp);
long xdr_size_writeargs (writeargs *argp);


typedef struct createargs createargs;
struct createargs
{
	diropargs	where;
	sattr		attributes;
};

bool_t xdr_createargs (xdrs *x, createargs *argp);
long xdr_size_createargs (createargs *ap);


typedef struct renameargs renameargs;
struct renameargs
{
	diropargs	from;
	diropargs	to;
};

bool_t xdr_renameargs (xdrs *x, renameargs *argp);
long xdr_size_renameargs (renameargs *ap);


typedef struct linkargs linkargs;
struct linkargs
{
	nfs_fh		from;
	diropargs	to;
};

bool_t xdr_linkargs (xdrs *x, linkargs *ap);
long xdr_size_linkargs (linkargs *ap);


typedef struct symlinkargs symlinkargs;
struct symlinkargs
{
	diropargs	from;
	const char *	to;
	sattr		attributes;
};

bool_t xdr_symlinkargs (xdrs *x, symlinkargs *ap);
long xdr_size_symlinkargs (symlinkargs *ap);


typedef struct entry entry;
struct entry
{
	ulong		fileid;
	char *		name;
	nfscookie	cookie;
	entry *		nextentry;
};

bool_t xdr_entry (xdrs *x, entry *ep);
long xdr_size_entry (entry *ep);


typedef struct readdirres readdirres;
struct readdirres
{
	enum_t	status;
	union
	{
		struct
		{
			entry *	entries;
			bool_t	eof;
		} readdirok;
	} readdirres_u;
};

bool_t xdr_readdirres (xdrs *x, readdirres *rp);
long xdr_size_readdirres (readdirres *rp);


typedef struct readdirargs readdirargs;
struct readdirargs
{
	nfs_fh		dir;
	nfscookie	cookie;
	ulong		count;
};

bool_t xdr_readdirargs (xdrs *x, readdirargs *ap);
long xdr_size_readdirargs (readdirargs *ap);


typedef struct statfsres statfsres;
struct statfsres
{
	enum_t status;
	union
	{
		struct
		{
			ulong	tsize;   /* optimum transfer size */
			ulong	bsize;   /* block size of the file system */
			ulong	blocks;  /* total number of blocks */
			ulong	bfree;   /* number of free blocks */
			ulong	bavail;  /* number of user-available blocks */
		} info;
	} statfsres_u;
};

bool_t xdr_statfsres (xdrs *x, statfsres *sp);
long xdr_size_statfsres (statfsres *sp);


# endif /* _nfs_xdr_h */
