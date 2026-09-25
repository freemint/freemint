/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

/*
 * File : nfs3_xdr.h
 *        data types of the NFS version 3 protocol, RFC 1813.
 *
 * The most important changes against NFS version 2 (RFC 1094), which the
 * old nfs.xfs implements:
 *
 *  - File handles are variable sized, up to 64 bytes (NFS2: fixed 32
 *    bytes). Linux uses handles whose length depends on the exported
 *    file system, so the length has to be carried around everywhere.
 *  - File sizes, offsets, file ids and directory cookies are 64 bits
 *    wide (NFS2: 32 bits). This is what limited NFS2 to 2/4 GB files.
 *  - fattr3.mode carries the protection bits only; the file type lives
 *    in the separate fattr3.type field. In NFS2 the type was encoded
 *    into the mode word as well, and clients used to look at it there.
 *  - fattr3 has `used' (bytes actually allocated, 64 bit) instead of
 *    `blocks' (512 byte units), no `blocksize', and a structured rdev.
 *  - sattr3 uses a discriminated union per attribute instead of the
 *    NFS2 convention of "all bits set means do not change".
 *  - Almost every reply carries optional attributes (post_op_attr) or
 *    weak cache consistency data (wcc_data), also in the error case.
 *    They must be decoded even when they are not used.
 *  - LOOKUP moved from procedure 4 to 3, STATFS was replaced by FSSTAT
 *    (18), and ACCESS (4), MKNOD (11), READDIRPLUS (17), FSINFO (19),
 *    PATHCONF (20) and COMMIT (21) were added.
 *  - CREATE/MKDIR/SYMLINK/MKNOD return the new file handle only
 *    optionally (post_op_fh3), so a LOOKUP may be needed afterwards.
 *  - WRITE knows about unstable writes (stable_how) that have to be
 *    flushed with COMMIT; this driver always writes FILE_SYNC.
 *  - The error code set was extended (NFS3ERR_INVAL, NFS3ERR_NOTSUPP,
 *    NFS3ERR_BAD_COOKIE, NFS3ERR_JUKEBOX, ...); NFS3ERR_WFLUSH is gone.
 */

# ifndef _nfs3_xdr_h
# define _nfs3_xdr_h


# include "global.h"
# include "xdr.h"


/* request numbers, RFC 1813, section 3
 */

# define NFSPROC3_NULL		0
# define NFSPROC3_GETATTR	1
# define NFSPROC3_SETATTR	2
# define NFSPROC3_LOOKUP	3
# define NFSPROC3_ACCESS	4
# define NFSPROC3_READLINK	5
# define NFSPROC3_READ		6
# define NFSPROC3_WRITE		7
# define NFSPROC3_CREATE	8
# define NFSPROC3_MKDIR		9
# define NFSPROC3_SYMLINK	10
# define NFSPROC3_MKNOD		11
# define NFSPROC3_REMOVE	12
# define NFSPROC3_RMDIR		13
# define NFSPROC3_RENAME	14
# define NFSPROC3_LINK		15
# define NFSPROC3_READDIR	16
# define NFSPROC3_READDIRPLUS	17
# define NFSPROC3_FSSTAT	18
# define NFSPROC3_FSINFO	19
# define NFSPROC3_PATHCONF	20
# define NFSPROC3_COMMIT	21

# define NFS_PROGRAM		100003
# define NFS3_VERSION		3
# define NFS3_MAXPROC		21


/* basic sizes, RFC 1813, section 2.5
 */
# define NFS3_FHSIZE		64	/* maximum size of a file handle */
# define NFS3_COOKIEVERFSIZE	8
# define NFS3_CREATEVERFSIZE	8
# define NFS3_WRITEVERFSIZE	8

# define MAXPATHLEN		1024	/* max bytes in a path name argument */
# define MAXNAMLEN		255	/* max bytes in a file name argument */

/* Upper limit for a single READ/WRITE request. NFS3 negotiates the real
 * values with FSINFO, this is only the limit we are willing to accept.
 */
# define MAXDATA		8192


/* nfsstat3, RFC 1813, section 2.6
 */
# define NFS3_OK		0
# define NFS3ERR_PERM		1
# define NFS3ERR_NOENT		2
# define NFS3ERR_IO		5
# define NFS3ERR_NXIO		6
# define NFS3ERR_ACCES		13
# define NFS3ERR_EXIST		17
# define NFS3ERR_XDEV		18
# define NFS3ERR_NODEV		19
# define NFS3ERR_NOTDIR		20
# define NFS3ERR_ISDIR		21
# define NFS3ERR_INVAL		22
# define NFS3ERR_FBIG		27
# define NFS3ERR_NOSPC		28
# define NFS3ERR_ROFS		30
# define NFS3ERR_MLINK		31
# define NFS3ERR_NAMETOOLONG	63
# define NFS3ERR_NOTEMPTY	66
# define NFS3ERR_DQUOT		69
# define NFS3ERR_STALE		70
# define NFS3ERR_REMOTE		71
# define NFS3ERR_BADHANDLE	10001
# define NFS3ERR_NOT_SYNC	10002
# define NFS3ERR_BAD_COOKIE	10003
# define NFS3ERR_NOTSUPP	10004
# define NFS3ERR_TOOSMALL	10005
# define NFS3ERR_SERVERFAULT	10006
# define NFS3ERR_BADTYPE	10007
# define NFS3ERR_JUKEBOX	10008


/* ftype3, RFC 1813, section 2.5. Note that NFS2's NFNON is gone and
 * that sockets and fifos have their own type now.
 */
# define NF3REG		1
# define NF3DIR		2
# define NF3BLK		3
# define NF3CHR		4
# define NF3LNK		5
# define NF3SOCK	6
# define NF3FIFO	7


/* mode3 bits, RFC 1813, section 2.5. Unlike NFS2's fattr.mode these
 * carry no file type information at all.
 */
# define N3MODE_SUID	0004000
# define N3MODE_SGID	0002000
# define N3MODE_SVTX	0001000
# define N3MODE_PERM	0007777


/* time_how */
# define DONT_CHANGE		0
# define SET_TO_SERVER_TIME	1
# define SET_TO_CLIENT_TIME	2

/* createmode3 */
# define UNCHECKED	0
# define GUARDED	1
# define EXCLUSIVE	2

/* stable_how */
# define UNSTABLE	0
# define DATA_SYNC	1
# define FILE_SYNC	2

/* ACCESS3 bits */
# define ACCESS3_READ		0x0001
# define ACCESS3_LOOKUP		0x0002
# define ACCESS3_MODIFY		0x0004
# define ACCESS3_EXTEND		0x0008
# define ACCESS3_DELETE		0x0010
# define ACCESS3_EXECUTE	0x0020

/* FSINFO3 properties */
# define FSF3_LINK		0x0001
# define FSF3_SYMLINK		0x0002
# define FSF3_HOMOGENEOUS	0x0004
# define FSF3_CANSETTIME	0x0008


typedef uint64	fileid3;
typedef uint64	cookie3;
typedef uint64	size3;
typedef uint64	offset3;
typedef ulong	mode3;
typedef ulong	count3;
typedef ulong	uid3;
typedef ulong	gid3;


/* nfs_fh3: opaque data<NFS3_FHSIZE> */
typedef struct nfs_fh3
{
	ulong	len;
	opaque	data[NFS3_FHSIZE];
} nfs_fh3;

bool_t	xdr_nfs_fh3	(xdrs *x, nfs_fh3 *fhp);
long	xdr_size_nfs_fh3 (nfs_fh3 *fhp);


typedef struct nfstime3
{
	ulong	seconds;
	ulong	nseconds;
} nfstime3;

bool_t	xdr_nfstime3	(xdrs *x, nfstime3 *tp);
# define xdr_size_nfstime3(tp)	(2L * sizeof (ulong))


typedef struct specdata3
{
	ulong	specdata1;	/* major */
	ulong	specdata2;	/* minor */
} specdata3;

bool_t	xdr_specdata3	(xdrs *x, specdata3 *sp);


typedef struct fattr3
{
	enum_t		type;		/* ftype3 */
	mode3		mode;		/* protection bits only! */
	ulong		nlink;
	uid3		uid;
	gid3		gid;
	size3		size;
	size3		used;		/* bytes really allocated */
	specdata3	rdev;
	uint64		fsid;
	fileid3		fileid;
	nfstime3	atime;
	nfstime3	mtime;
	nfstime3	ctime;
} fattr3;

bool_t	xdr_fattr3	(xdrs *x, fattr3 *fp);
# define XDR_SIZE_FATTR3	(21L * sizeof (ulong))


/* post_op_attr: the attributes are a hint only, the server may leave
 * them out at any time.
 */
typedef struct post_op_attr
{
	bool_t	attributes_follow;
	fattr3	attributes;
} post_op_attr;

bool_t	xdr_post_op_attr (xdrs *x, post_op_attr *ap);


typedef struct wcc_attr
{
	size3		size;
	nfstime3	mtime;
	nfstime3	ctime;
} wcc_attr;

typedef struct pre_op_attr
{
	bool_t		attributes_follow;
	wcc_attr	attributes;
} pre_op_attr;

/* weak cache consistency data, returned by all modifying operations */
typedef struct wcc_data
{
	pre_op_attr	before;
	post_op_attr	after;
} wcc_data;

bool_t	xdr_wcc_data	(xdrs *x, wcc_data *wp);


typedef struct post_op_fh3
{
	bool_t	handle_follows;
	nfs_fh3	handle;
} post_op_fh3;

bool_t	xdr_post_op_fh3	(xdrs *x, post_op_fh3 *fp);


/* sattr3. Every member is only transmitted when its set_* flag says so,
 * which replaces NFS2's "a field of all ones means don't touch".
 */
typedef struct sattr3
{
	bool_t		set_mode;
	mode3		mode;
	bool_t		set_uid;
	uid3		uid;
	bool_t		set_gid;
	gid3		gid;
	bool_t		set_size;
	size3		size;
	enum_t		set_atime;	/* time_how */
	nfstime3	atime;
	enum_t		set_mtime;	/* time_how */
	nfstime3	mtime;
} sattr3;

bool_t	xdr_sattr3	(xdrs *x, sattr3 *sp);
long	xdr_size_sattr3	(sattr3 *sp);
void	sattr3_init	(sattr3 *sp);


typedef struct diropargs3
{
	nfs_fh3		dir;
	const char *	name;
} diropargs3;

bool_t	xdr_diropargs3	(xdrs *x, diropargs3 *ap);
long	xdr_size_diropargs3 (diropargs3 *ap);


/* GETATTR */
typedef struct getattr3res
{
	enum_t	status;
	fattr3	obj_attributes;
} getattr3res;

bool_t	xdr_getattr3res	(xdrs *x, getattr3res *rp);


/* SETATTR */
typedef struct setattr3args
{
	nfs_fh3	object;
	sattr3	new_attributes;
	bool_t	check;			/* sattrguard3 */
	nfstime3 obj_ctime;
} setattr3args;

bool_t	xdr_setattr3args (xdrs *x, setattr3args *ap);
long	xdr_size_setattr3args (setattr3args *ap);

typedef struct wcc3res
{
	enum_t		status;
	wcc_data	wcc;
} wcc3res;

bool_t	xdr_wcc3res	(xdrs *x, wcc3res *rp);


/* LOOKUP */
typedef struct lookup3res
{
	enum_t		status;
	nfs_fh3		object;
	post_op_attr	obj_attributes;
	post_op_attr	dir_attributes;
} lookup3res;

bool_t	xdr_lookup3res	(xdrs *x, lookup3res *rp);


/* ACCESS */
typedef struct access3args
{
	nfs_fh3	object;
	ulong	access;
} access3args;

bool_t	xdr_access3args	(xdrs *x, access3args *ap);
long	xdr_size_access3args (access3args *ap);

typedef struct access3res
{
	enum_t		status;
	post_op_attr	obj_attributes;
	ulong		access;
} access3res;

bool_t	xdr_access3res	(xdrs *x, access3res *rp);


/* READLINK */
typedef struct readlink3res
{
	enum_t		status;
	post_op_attr	symlink_attributes;
	char *		data;		/* buffer supplied by the caller */
} readlink3res;

bool_t	xdr_readlink3res (xdrs *x, readlink3res *rp);


/* READ */
typedef struct read3args
{
	nfs_fh3	file;
	offset3	offset;
	count3	count;
} read3args;

bool_t	xdr_read3args	(xdrs *x, read3args *ap);
long	xdr_size_read3args (read3args *ap);

typedef struct read3res
{
	enum_t		status;
	post_op_attr	file_attributes;
	count3		count;
	bool_t		eof;
	char *		data_val;	/* buffer supplied by the caller */
	ulong		data_len;
	ulong		data_max;	/* size of that buffer */
} read3res;

bool_t	xdr_read3res	(xdrs *x, read3res *rp);


/* WRITE */
typedef struct write3args
{
	nfs_fh3		file;
	offset3		offset;
	count3		count;
	enum_t		stable;		/* stable_how */
	const char *	data_val;
	ulong		data_len;
} write3args;

bool_t	xdr_write3args	(xdrs *x, write3args *ap);
long	xdr_size_write3args (write3args *ap);

typedef struct write3res
{
	enum_t		status;
	wcc_data	file_wcc;
	count3		count;
	enum_t		committed;
	char		verf[NFS3_WRITEVERFSIZE];
} write3res;

bool_t	xdr_write3res	(xdrs *x, write3res *rp);


/* CREATE, MKDIR, SYMLINK and MKNOD all share this result */
typedef struct create3res
{
	enum_t		status;
	post_op_fh3	obj;
	post_op_attr	obj_attributes;
	wcc_data	dir_wcc;
} create3res;

bool_t	xdr_create3res	(xdrs *x, create3res *rp);

typedef struct create3args
{
	diropargs3	where;
	enum_t		how;		/* createmode3 */
	sattr3		obj_attributes;	/* UNCHECKED and GUARDED */
	char		verf[NFS3_CREATEVERFSIZE];	/* EXCLUSIVE */
} create3args;

bool_t	xdr_create3args	(xdrs *x, create3args *ap);
long	xdr_size_create3args (create3args *ap);

typedef struct mkdir3args
{
	diropargs3	where;
	sattr3		attributes;
} mkdir3args;

bool_t	xdr_mkdir3args	(xdrs *x, mkdir3args *ap);
long	xdr_size_mkdir3args (mkdir3args *ap);

/* NOTE: unlike NFS2's symlinkargs the attributes come BEFORE the path */
typedef struct symlink3args
{
	diropargs3	where;
	sattr3		symlink_attributes;
	const char *	symlink_data;
} symlink3args;

bool_t	xdr_symlink3args (xdrs *x, symlink3args *ap);
long	xdr_size_symlink3args (symlink3args *ap);


/* REMOVE, RMDIR: arguments are diropargs3, result is wcc3res */

/* RENAME */
typedef struct rename3args
{
	diropargs3	from;
	diropargs3	to;
} rename3args;

bool_t	xdr_rename3args	(xdrs *x, rename3args *ap);
long	xdr_size_rename3args (rename3args *ap);

typedef struct rename3res
{
	enum_t		status;
	wcc_data	fromdir_wcc;
	wcc_data	todir_wcc;
} rename3res;

bool_t	xdr_rename3res	(xdrs *x, rename3res *rp);


/* LINK */
typedef struct link3args
{
	nfs_fh3		file;
	diropargs3	link;
} link3args;

bool_t	xdr_link3args	(xdrs *x, link3args *ap);
long	xdr_size_link3args (link3args *ap);

typedef struct link3res
{
	enum_t		status;
	post_op_attr	file_attributes;
	wcc_data	linkdir_wcc;
} link3res;

bool_t	xdr_link3res	(xdrs *x, link3res *rp);


/* READDIR */
typedef struct readdir3args
{
	nfs_fh3	dir;
	cookie3	cookie;
	char	cookieverf[NFS3_COOKIEVERFSIZE];
	count3	count;
} readdir3args;

bool_t	xdr_readdir3args (xdrs *x, readdir3args *ap);
long	xdr_size_readdir3args (readdir3args *ap);

typedef struct entry3 entry3;
struct entry3
{
	fileid3	fileid;
	char *	name;
	cookie3	cookie;
	entry3 *nextentry;
};

typedef struct readdir3res
{
	enum_t		status;
	post_op_attr	dir_attributes;
	char		cookieverf[NFS3_COOKIEVERFSIZE];
	entry3 *	entries;
	bool_t		eof;

	/* buffer the decoded entry list is built in; supplied by the
	 * caller, see decode_dirlist3()
	 */
	char *		buffer;
	long		buflen;
} readdir3res;

bool_t	xdr_readdir3res	(xdrs *x, readdir3res *rp);


/* FSSTAT. Unlike NFS2's STATFS this counts bytes, not blocks. */
typedef struct fsstat3res
{
	enum_t		status;
	post_op_attr	obj_attributes;
	size3		tbytes;
	size3		fbytes;
	size3		abytes;
	size3		tfiles;
	size3		ffiles;
	size3		afiles;
	ulong		invarsec;
} fsstat3res;

bool_t	xdr_fsstat3res	(xdrs *x, fsstat3res *rp);


/* FSINFO. New in NFS3: the client asks the server for the transfer
 * sizes instead of guessing them.
 */
typedef struct fsinfo3res
{
	enum_t		status;
	post_op_attr	obj_attributes;
	ulong		rtmax;
	ulong		rtpref;
	ulong		rtmult;
	ulong		wtmax;
	ulong		wtpref;
	ulong		wtmult;
	ulong		dtpref;
	size3		maxfilesize;
	nfstime3	time_delta;
	ulong		properties;
} fsinfo3res;

bool_t	xdr_fsinfo3res	(xdrs *x, fsinfo3res *rp);


/* PATHCONF */
typedef struct pathconf3res
{
	enum_t		status;
	post_op_attr	obj_attributes;
	ulong		linkmax;
	ulong		name_max;
	bool_t		no_trunc;
	bool_t		chown_restricted;
	bool_t		case_insensitive;
	bool_t		case_preserving;
} pathconf3res;

bool_t	xdr_pathconf3res (xdrs *x, pathconf3res *rp);


/* COMMIT */
typedef struct commit3args
{
	nfs_fh3	file;
	offset3	offset;
	count3	count;
} commit3args;

bool_t	xdr_commit3args	(xdrs *x, commit3args *ap);
long	xdr_size_commit3args (commit3args *ap);

typedef struct commit3res
{
	enum_t		status;
	wcc_data	file_wcc;
	char		verf[NFS3_WRITEVERFSIZE];
} commit3res;

bool_t	xdr_commit3res	(xdrs *x, commit3res *rp);


# endif /* _nfs3_xdr_h */
