/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_file_h
# define _mint_file_h

# include "kcompiler.h"
# include "ktypes.h"
# include "block_IO.h"		/* eXtended kernelinterface */
# include "dcntl.h"		/* dcntl opcodes are now in a seperate file */


struct fcookie
{
	FILESYS	*fs;		/* filesystem that knows about this cookie */
	ushort	dev;		/* device info (e.g. Rwabs device number) */
	ushort	aux;		/* extra data that the file system may want */
	long	index;		/* this+dev uniquely identifies a file */
};

struct timeval
{
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* microseconds */
};


# define TOS_NAMELEN 13

struct dtabuf
{
	ushort	index;		/* index into arrays in the PROC struct */
	long	magic;
# define SVALID	0x1234fedcL	/* magic for a valid search */
# define EVALID	0x5678ba90L	/* magic for an exhausted search */
	
	char	dta_pat [TOS_NAMELEN+1]; /* pointer to pattern, if necessary */
	char	dta_sattrib;	/* attributes being searched for */
	
	/* this stuff is returned to the user */
	char	dta_attrib;
	ushort	dta_time;
	ushort	dta_date;
	ulong	dta_size;
	char	dta_name [TOS_NAMELEN+1];
};


/* structure for opendir/readdir/closedir */
struct dirstruct
{
	fcookie fc;		/* cookie for this directory */
	ushort	index;		/* index of the current entry */
	ushort	flags;		/* flags (e.g. tos or not) */
# define TOS_SEARCH	0x01
	char	fsstuff[60];	/* anything else the file system wants */
				/* NOTE: this must be at least 45 bytes */
	DIR	*next;		/* linked together so we can close them
				 * on process termination */
};


/* structure for getxattr */
struct xattr
{
	ushort	mode;
	
/* file types */
# define S_IFMT		0170000	/* file type mask */
# define S_IFLNK	0160000	/* symbolic link */
# define S_IFMEM	0140000	/* memory region or process */
# define S_IFIFO	0120000	/* FIFO */
# define S_IFREG 	0100000	/* regular file */
# define S_IFBLK	0060000	/* block special file */
# define S_IFDIR	0040000	/* directory file */
# define S_IFCHR	0020000	/* character special file (BIOS) */
# define S_IFSOCK	0010000	/* socket file */

# define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
# define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
# define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
# define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
# define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
# define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
# define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

/* special bits: setuid, setgid, sticky bit */
# define S_ISUID	0004000
# define S_ISGID	0002000
# define S_ISVTX	0001000

/* file access modes for user, group, and other */
# define S_IRWXU	0000700
# define S_IRUSR	0000400
# define S_IWUSR	0000200
# define S_IXUSR	0000100

# define S_IRWXG	0000070
# define S_IRGRP	0000040
# define S_IWGRP	0000020
# define S_IXGRP	0000010

# define S_IRWXO	0000007
# define S_IROTH	0000004
# define S_IWOTH	0000002
# define S_IXOTH	0000001

# define S_IRWXUGO	(S_IRWXU | S_IRWXG | S_IRWXO)
# define S_IALLUGO	(S_ISUID | S_ISGID | S_ISVTX | S_IRWXUGO)
# define S_IRUGO	(S_IRUSR | S_IRGRP | S_IROTH)
# define S_IWUGO	(S_IWUSR | S_IWGRP | S_IWOTH)
# define S_IXUGO	(S_IXUSR | S_IXGRP | S_IXOTH)
	
	long	index;
	ushort	dev;
	ushort	rdev;		/* "real" device */
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
	
/* defines for TOS attribute bytes */
# define FA_RDONLY	0x01
# define FA_HIDDEN	0x02
# define FA_SYSTEM	0x04
# define FA_LABEL	0x08
# define FA_DIR		0x10
# define FA_CHANGED	0x20
# define FA_VFAT	0x0f	/* internal: VFAT entry */
# define FA_SYMLINK	0x40	/* internal: symbolic link */
	
	short	reserved2;
	long	reserved3[2];
};

typedef struct time TIME;
struct time
{
	long	high_time;
	long	time;		/* This has to be signed!  */
	ulong	nanoseconds;
};

/* structure for stat */
struct stat
{
	llong	dev;		/* inode's device */
	ulong	ino;		/* inode's number */
	ulong	mode;		/* inode protection mode */
	ulong	nlink;		/* number of hard links */
	ulong	uid;		/* user ID of the file's owner */
	ulong	gid;		/* group ID of the file's group */
	llong	rdev;		/* device type */
	TIME	atime;		/* time of last access, UTC */
	TIME	mtime;		/* time of last data modification, UTC */
	TIME	ctime;		/* time of last file status change, UTC */
	llong	size;		/* file size, in bytes */
	llong	blocks;		/* blocks allocated for file */
	ulong	blksize;	/* optimal blocksize for I/O */
	ulong	flags;		/* user defined flags for file */
	ulong	gen;		/* file generation number */
	
	long	res[7];		/* sizeof = 128 bytes */
};


struct fileptr
{
	short	links;		/* number of copies of this descriptor */
	ushort	flags;		/* file open mode and other file flags */
	long	pos;		/* position in file */
	long	devinfo;	/* device driver specific info */
	fcookie	fc;		/* file system cookie for this file */
	DEVDRV	*dev;		/* device driver that knows how to deal with this */
	FILEPTR	*next;		/* link to next fileptr for this file */
};


/* structure for internal kernel locks */
struct ilock
{
	struct flock l;		/* the actual lock */
	struct ilock *next;	/* next lock in the list */
	long reserved [4];	/* reserved for future expansion */
};


struct devdrv
{
	long _cdecl (*open)	(FILEPTR *f);
	long _cdecl (*write)	(FILEPTR *f, const char *buf, long bytes);
	long _cdecl (*read)	(FILEPTR *f, char *buf, long bytes);
	long _cdecl (*lseek)	(FILEPTR *f, long where, int whence);
	long _cdecl (*ioctl)	(FILEPTR *f, int mode, void *buf);
	long _cdecl (*datime)	(FILEPTR *f, ushort *timeptr, int rwflag);
	long _cdecl (*close)	(FILEPTR *f, int pid);
	long _cdecl (*select)	(FILEPTR *f, long proc, int mode);
	void _cdecl (*unselect)	(FILEPTR *f, long proc, int mode);
	
	/* extensions, check dev_descr.drvsize (size of DEVDRV struct) before calling:
	 * fast RAW tty byte io
	 */
	long _cdecl (*writeb)	(FILEPTR *f, const char *buf, long bytes);
	long _cdecl (*readb)	(FILEPTR *f, char *buf, long bytes);
	
	/* what about: scatter/gather io for DMA devices...
	 * long _cdecl (*writev)(FILEPTR *f, const struct iovec *iov, long cnt);
	 * long _cdecl (*readv)	(FILEPTR *f, struct iovec *iov, long cnt);
	 */
};


struct filesys
{
	/* kernel data
	 */
	FILESYS	*next;			/* link to next file system on chain */
	long	fsflags;
# define FS_KNOPARSE		0x0001	/* kernel shouldn't do parsing */
# define FS_CASESENSITIVE	0x0002	/* file names are case sensitive */
# define FS_NOXBIT		0x0004	/* if a file can be read, it can be executed */
# define FS_LONGPATH		0x0008	/* file system understands "size" argument to "getname" */
# define FS_NO_C_CACHE		0x0010	/* don't cache cookies for this filesystem */
# define FS_DO_SYNC		0x0020	/* file system has a sync function */
# define FS_OWN_MEDIACHANGE	0x0040	/* filesystem control self media change (dskchng) */
# define FS_REENTRANT_L1	0x0080	/* fs is level 1 reentrant */
# define FS_REENTRANT_L2	0x0100	/* fs is level 2 reentrant */
# define FS_EXT_1		0x0200	/* extensions level 1 - mknod & unmount */
# define FS_EXT_2		0x0400	/* extensions level 2 - additional place at the end */
# define FS_EXT_3		0x0800	/* extensions level 3 - stat & native UTC timestamps */
	
	/* filesystem functions
	 */
	long	_cdecl (*root)		(int drv, fcookie *fc);
	long	_cdecl (*lookup)	(fcookie *dir, const char *name, fcookie *fc);
	long	_cdecl (*creat)		(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
	DEVDRV*	_cdecl (*getdev)	(fcookie *file, long *devspecial);
	long	_cdecl (*getxattr)	(fcookie *file, XATTR *xattr);
	long	_cdecl (*chattr)	(fcookie *file, int attr);
	long	_cdecl (*chown)		(fcookie *file, int uid, int gid);
	long	_cdecl (*chmode)	(fcookie *file, unsigned mode);
	long	_cdecl (*mkdir)		(fcookie *dir, const char *name, unsigned mode);
	long	_cdecl (*rmdir)		(fcookie *dir, const char *name);
	long	_cdecl (*remove)	(fcookie *dir, const char *name);
	long	_cdecl (*getname)	(fcookie *relto, fcookie *dir, char *pathname, int size);
	long	_cdecl (*rename)	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
	long	_cdecl (*opendir)	(DIR *dirh, int tosflag);
	long	_cdecl (*readdir)	(DIR *dirh, char *name, int namelen, fcookie *fc);
	long	_cdecl (*rewinddir)	(DIR *dirh);
	long	_cdecl (*closedir)	(DIR *dirh);
	long	_cdecl (*pathconf)	(fcookie *dir, int which);
	long	_cdecl (*dfree)		(fcookie *dir, long *buf);
	long	_cdecl (*writelabel)	(fcookie *dir, const char *name);
	long	_cdecl (*readlabel)	(fcookie *dir, char *name, int namelen);
	long	_cdecl (*symlink)	(fcookie *dir, const char *name, const char *to);
	long	_cdecl (*readlink)	(fcookie *dir, char *buf, int len);
	long	_cdecl (*hardlink)	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
	long	_cdecl (*fscntl)	(fcookie *dir, const char *name, int cmd, long arg);
	long	_cdecl (*dskchng)	(int drv, int mode);
	long	_cdecl (*release)	(fcookie *);
	long	_cdecl (*dupcookie)	(fcookie *new, fcookie *old);
	long	_cdecl (*sync)		(void);
	long	_cdecl (*mknod)		(fcookie *dir, const char *name, ulong mode);
	long	_cdecl (*unmount)	(int drv);
	long	_cdecl (*stat64)	(fcookie *file, STAT *stat);
	
	long	res1, res2, res3;	/* reserved */
	
	/* experimental extension
	 */
	ulong	lock;			/* for non-blocking DMA */
	ulong	sleepers;		/* sleepers on this filesystem */
	void	_cdecl (*block)		(FILESYS *fs, ushort dev, const char *);
	void	_cdecl (*deblock)	(FILESYS *fs, ushort dev, const char *);
};


/* kerinfo - kernel interface table
 * --------------------------------
 * 
 * All functions should be called using the GCC calling conventions:
 * 
 * (1) parameters are passed on the stack, aligned on 16 bit boundaries
 * (2) registers d0-d2 and a0-a2 are scratch registers and may be modified
 *     by the called functions
 * (3) if the function returns a value, it will be returned in register
 *     d0
 * 
 * data types:
 * -----------
 * int   is 16bit
 * 
 * short is 16bit
 * long  is 32bit
 * 
 * ushort, ulong - unsigned version of short, long
 * 
 */

struct kerinfo
{
	ushort	maj_version;	/* kernel version number */
	ushort	min_version;	/* minor kernel version number */
	ushort	default_perm;	/* default file permissions */
	ushort	version;	/* version number */
	
	
	/* OS functions
	 * ------------
	 * NOTE: these tables are definitely READ ONLY!!!!
	 */
	Func	*bios_tab;	/* pointer to the BIOS entry points */
	Func	*dos_tab;	/* pointer to the GEMDOS entry points */
	
	
	/* media change vector
	 * -------------------
	 * call this if a device driver detects a disk
	 * change during a read or write operation. The parameter is the BIOS
	 * device number of the disk that changed.
	 */
	void	_cdecl (*drvchng)(ushort);
	
	/* Debugging stuff
	 * ---------------
	 * 
	 * trace - informational messages
	 * debug - error messages
	 * alert - really serious errors
	 * fatal - fatal errors
	 */
	void	_cdecl (*trace)(const char *, ...);
	void	_cdecl (*debug)(const char *, ...);
	void	_cdecl (*alert)(const char *, ...);
	EXITING _cdecl (*fatal)(const char *, ...) NORETURN;
	
	
	/* memory allocation functions
	 * ---------------------------
	 * 
	 * kmalloc(size) / kfree (ptr)
	 * 
	 * kmalloc and kfree should be used for most purposes, and act like
	 * malloc and free respectively; kmalloc'ed memory is *exclusive*
	 * kernel memory, supervisor proteced
	 * 
	 * umalloc(size) / ufree (ptr)
	 * 
	 * umalloc and ufree may be used to allocate/free memory that is
	 * attached to the current process, and which is freed automatically
	 * when the process exits; this is generally not of much use to a file
	 * system driver
	 */
	void *	_cdecl (*kmalloc)(ulong);
	void	_cdecl (*kfree)(void *);
	void *	_cdecl (*umalloc)(ulong);
	void	_cdecl (*ufree)(void *);
	
	
	/* utility functions for string manipulation
	 * -----------------------------------------
	 * 
	 * like the normal library functions
	 * 
	 * sprintf:
	 * --------
	 * kernel sprintf, like library sprintf but not so heavy; floating
	 * point formats are not supported!
	 * Also this sprintf will put at most SPRINTF_MAX characters into
	 * the output string.
	 */
	int	_cdecl (*kstrnicmp)(const char *, const char *, int);
	int	_cdecl (*kstricmp)(const char *, const char *);
	char *	_cdecl (*kstrlwr)(char *);
	char *	_cdecl (*kstrupr)(char *);
	int	_cdecl (*ksprintf)(char *, const char *, ...);
	
	
	/* utility functions for manipulating time
	 * ---------------------------------------
	 * 
	 * millis_time(ms, timeptr)
	 * 
	 * convert "ms" milliseconds into a DOS time (in td[0]) and date
	 * (in td[1]) convert a DOS style time and date into a Unix style
	 * time; returns the Unix time
	 * 
	 * unixtime(time, date)
	 * 
	 * convert a DOS style time and date into a Unix style time; returns
	 * the Unix time
	 * 
	 * dostime(time)
	 * 
	 * convert a Unix time into a DOS time (in the high word of the
	 * returned value) and date (in the low word)
	 */
	void	_cdecl (*millis_time)(ulong, short *);
	long	_cdecl (*unixtime)(ushort, ushort);
	long	_cdecl (*dostime)(long);
	
	
	/* utility functions for dealing with pauses and sleep
	 * ---------------------------------------------------
	 * 
	 * nap(n)
	 * 
	 * go to sleep temporarily for about "n" milliseconds
	 * (the exact time slept may vary)
	 * 
	 * sleep(que, cond)
	 * 
	 * wait on system queue "que" until a condition occurs
	 * 
	 * wake(que, cond)
	 * 
	 * wake all processes on queue "que" that are waiting for
	 * condition "cond"
	 * 
	 * wakeselect(p)
	 * 
	 * wake a process that is doing a select(); "param" should be the
	 * process code passed to select()
	 */
	void	_cdecl (*nap)(unsigned);
	int	_cdecl (*sleep)(int que, long cond);
	void	_cdecl (*wake)(int que, long cond);
	void	_cdecl (*wakeselect)(long param);
	
	
	/* file system utility functions
	 * -----------------------------
	 * 
	 * denyshare(list, f)
	 * 
	 * "list" is a list of open files, "f" is a new file that is being
	 * opened.
	 * If the file sharing mode of f conflicts with any of the FILEPTRs
	 * in the list, then this returns 1, otherwise 0.
	 * 
	 * denylock(list, newlock)
	 * 
	 * checks a list of locks to see if the new lock conflicts with any
	 * one in the list. If so, the first conflicting lock is returned;
	 * otherwise, a NULL is returned.
	 * This function is only available if maj_version > 0 or
	 * min_version >= 94. Otherwise, it will be a null pointer.
	 */
	int	_cdecl (*denyshare)(FILEPTR *, FILEPTR *);
	LOCK *	_cdecl (*denylock)(LOCK *, LOCK *);
	
	
	/* functions for adding/cancelling timeouts
	 * ----------------------------------------
	 * 
	 * addtimeout(delta, func)
	 * 
	 * schedules a callback to occur at some future time "delta" is the
	 * number of milliseconds before the timeout is to occur, and func is
	 * a pointer to the function which should be called at that time. Note
	 * that the kernel's timeout facility does not actually have a
	 * millisecond resolution, so the function will most likely be called
	 * some time after specified time has elapsed (i.e. don't rely on this
	 * for really accurate timing). Also note that the timeout may occur
	 * in the context of a different process than the one which scheduled
	 * the timeout, i.e. the given function should not make assumptions
	 * involving the process context, including memory space.
	 * addtimeout() returns a long "magic cookie" which may be passed to
	 * the canceltimeout() function to cancel the pending time out.
	 * This function is only available in MiNT versions 1.06 and later;
	 * otherwise the pointer will be NULL.
	 * 
	 * canceltimeout(which)
	 * 
	 * cancels a pending timeout. The parameter is the "magic cookie"
	 * returned from addtimeout(). If the timeout has already occured,
	 * canceltimeout does nothing, otherwise it removes the timeout from the
	 * kernel's schedule.
	 * 
	 * addroottimeout(delta, func, flags)
	 * 
	 * same as addtimeout() but the timeout is attached to the root
	 * process (MiNT) and is never implicitly (programm termination)
	 * canceled
	 * flags - bitvektor, only bit 1 defined at the moment
	 *         if bit 1 is set, addroottimeout is called from an interrupt
	 *         handler and use a static list for new timeouts
	 *         Useful for Softinterrupts!
	 *       - all other bits must be set to 0
	 * 
	 * cancelroottimeout(which)
	 * 
	 * same as canceltimeout() but for root timeouts
	 */
	TIMEOUT * _cdecl (*addtimeout)(long, void _cdecl (*)());
	void	_cdecl (*canceltimeout)(TIMEOUT *);
	TIMEOUT * _cdecl (*addroottimeout)(long, void _cdecl (*)(), ushort);
	void	_cdecl (*cancelroottimeout)(TIMEOUT *);
	
	
	/* miscellaneous other things
	 * --------------------------
	 * 
	 * interrupt safe kill and wake
	 * 
	 * ikill(p, sig)
	 * iwake(que, cond, pid)
	 */
	long	_cdecl (*ikill)(int, ushort);
	void	_cdecl (*iwake)(int, long, short);
	
	/* 1.15 extensions */
	BIO	*bio;	/* buffered block I/O, see block_IO.doc */
	
	/* version 1 extension */
	TIMEVAL	*xtime;	/* pointer to current kernel time - UTC */
	
	long	res;	/* reserved */
	
	/* version 2 extension
	 * pointers are valid if != NULL
	 */
	
	long	_cdecl (*add_rsvfentry)(char *, char, char);
	long	_cdecl (*del_rsvfentry)(char *);
	long	_cdecl (*killgroup)(int, ushort, int);
	
	/* easy to use DMA interface, see dma.c for more details */
	DMA	*dma;
	
	/* reserved, set to 0 */
	long	res2 [18];
};


/* flags for open() modes */
# define O_RWMODE  	0x03	/* isolates file read/write mode */
#  define O_RDONLY	0x00
#  define O_WRONLY	0x01
#  define O_RDWR	0x02
#  define O_EXEC	0x03	/* execute file; used by kernel only */

/* 0x04 is for future expansion */
# define O_APPEND	0x08	/* all writes go to end of file */

# define O_SHMODE	0x70	/* isolates file sharing mode */
#  define O_COMPAT	0x00	/* compatibility mode */
#  define O_DENYRW	0x10	/* deny both read and write access */
#  define O_DENYW	0x20	/* deny write access to others */
#  define O_DENYR	0x30	/* deny read access to others */
#  define O_DENYNONE	0x40	/* don't deny any access to others */

# define O_NOINHERIT	0x80	/* private file (not passed to child) */

# define O_NDELAY	0x100	/* don't block for I/O on this file */
# define O_CREAT	0x200	/* create file if it doesn't exist */
# define O_TRUNC	0x400	/* truncate file to 0 bytes if it does exist */
# define O_EXCL		0x800	/* fail open if file exists */

# define O_USER		0x0fff	/* isolates user-settable flag bits */

# define O_GLOBAL	0x1000	/* for opening a global file */

/* kernel mode bits -- the user can't set these! */
# define O_TTY		0x2000
# define O_HEAD		0x4000
# define O_LOCK		0x8000

/* macros to be applied to FILEPTRS to determine their type */
# define is_terminal(f) (f->flags & O_TTY)

/* lseek() origins */
# define SEEK_SET	0		/* from beginning of file */
# define SEEK_CUR	1		/* from current location */
# define SEEK_END	2		/* from end of file */

/* The requests for Dpathconf() */
# define DP_INQUIRE	-1
# define DP_IOPEN	0		/* internal limit on # of open files */
# define DP_MAXLINKS	1		/* max number of hard links to a file */
# define DP_PATHMAX	2		/* max path name length */
# define DP_NAMEMAX	3		/* max length of an individual file name */
# define DP_ATOMIC	4		/* # of bytes that can be written atomically */

# define DP_TRUNC	5		/* file name truncation behavior */
#  define DP_NOTRUNC	0		/* long filenames give an error */
#  define DP_AUTOTRUNC	1		/* long filenames truncated */
#  define DP_DOSTRUNC	2		/* DOS truncation rules in effect */

# define DP_CASE	6
#  define DP_CASESENS	0		/* case sensitive */
#  define DP_CASECONV	1		/* case always converted */
#  define DP_CASEINSENS	2		/* case insensitive, preserved */

# define DP_MODEATTR	7
#  define DP_ATTRBITS	0x000000ffL	/* mask for valid TOS attribs */
#  define DP_MODEBITS	0x000fff00L	/* mask for valid Unix file modes */
#  define DP_FILETYPS	0xfff00000L	/* mask for valid file types */
#  define DP_FT_DIR	0x00100000L	/* directories (always if . is there) */
#  define DP_FT_CHR	0x00200000L	/* character special files */
#  define DP_FT_BLK	0x00400000L	/* block special files, currently unused */
#  define DP_FT_REG	0x00800000L	/* regular files */
#  define DP_FT_LNK	0x01000000L	/* symbolic links */
#  define DP_FT_SOCK	0x02000000L	/* sockets, currently unused */
#  define DP_FT_FIFO	0x04000000L	/* pipes */
#  define DP_FT_MEM	0x08000000L	/* shared memory or proc files */

# define DP_XATTRFIELDS	8
#  define DP_INDEX	0x0001
#  define DP_DEV	0x0002
#  define DP_RDEV	0x0004
#  define DP_NLINK	0x0008
#  define DP_UID	0x0010
#  define DP_GID	0x0020
#  define DP_BLKSIZE	0x0040
#  define DP_SIZE	0x0080
#  define DP_NBLOCKS	0x0100
#  define DP_ATIME	0x0200
#  define DP_CTIME	0x0400
#  define DP_MTIME	0x0800

# define DP_VOLNAMEMAX	9		/* max length of a volume name
					 * (0 if volume names not supported)
					 */

/* Dpathconf and Sysconf return this when a value is not limited
 * (or is limited only by available memory)
 */
# define UNLIMITED	0x7fffffffL


/* various character constants and defines for TTY's */
# define MiNTEOF	0x0000ff1a	/* 1a == ^Z */

/* defines for tty_read */
# define RAW		0
# define COOKED		0x1
# define NOECHO		0
# define ECHO		0x2
# define ESCSEQ		0x04		/* cursor keys, etc. get escape sequences */


/* terminal control constants (tty.sg_flags)
 */
# define T_CRMOD	0x0001
# define T_CBREAK	0x0002
# define T_ECHO		0x0004
# define T_XTABS_notyet	0x0008		/* unimplemented */
# define T_RAW		0x0010
# define T_LCASE_notyet	0x0020		/* unimplemented */

# define T_NOFLSH	0x0040		/* don't flush buffer when signals
					   are received */
# define T_TOS		0x0080
# define T_TOSTOP	0x0100
# define T_XKEY		0x0200		/* Fread returns escape sequences for
					   cursor keys, etc. */
# define T_ECHOCTL	0x0400		/* echo ctl chars as ^x */
/* 0x0800 still available */

# define T_TANDEM	0x1000
# define T_RTSCTS	0x2000
# define T_EVENP	0x4000		/* EVENP and ODDP are mutually exclusive */
# define T_ODDP		0x8000

# define TF_FLAGS	0xF000

/* some flags for TIOC[GS]FLAGS */
# define TF_BRKINT	0x0080		/* allow breaks interrupt (like ^C) */
# define TF_CAR		0x0800		/* nonlocal mode, require carrier */
# define TF_NLOCAL	TF_CAR

# define TF_STOPBITS	0x0003
# define TF_1STOP	 0x001
# define TF_15STOP	 0x002
# define TF_2STOP	 0x003

# define TF_CHARBITS	0x000C
# define TF_8BIT	 0x000
# define TF_7BIT	 0x004
# define TF_6BIT	 0x008
# define TF_5BIT	 0x00C

/* the following are terminal status flags (tty.state) */
/* (the low byte of tty.state indicates a part of an escape sequence still
 * hasn't been read by Fread, and is an index into that escape sequence)
 */
# define TS_ESC		0x00ff
# define TS_BLIND	0x0800		/* tty is `blind' i.e. has no carrier
					   (cleared in local mode) */
# define TS_HOLD	0x1000		/* hold (e.g. ^S/^Q) */
# define TS_HPCL	0x4000		/* hang up on close */
# define TS_COOKED	0x8000		/* interpret control chars */

/* structures for terminals
 */
struct tchars
{
	char	t_intrc;
	char	t_quitc;
	char	t_startc;
	char	t_stopc;
	char	t_eofc;
	char	t_brkc;
};

struct ltchars
{
	char	t_suspc;
	char	t_dsuspc;
	char	t_rprntc;
	char	t_flushc;
	char	t_werasc;
	char	t_lnextc;
};

struct sgttyb
{
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	ushort	sg_flags;
};

struct winsize
{
	short	ws_row;
	short	ws_col;
	short	ws_xpixel;
	short	ws_ypixel;
};

struct xkey
{
	short	xk_num;
	char	xk_def[8];
};

struct tty
{
	short		pgrp;		/* process group of terminal */
	short		state;		/* terminal status, e.g. stopped */
	short		use_cnt;	/* number of times terminal is open */
	short		aux_cnt;	/* number of times terminal is open as /dev/aux */
	struct sgttyb 	sg;
	struct tchars 	tc;
	struct ltchars 	ltc;
	struct winsize	wsiz;
	long		rsel;		/* selecting process for read */
	long		wsel;		/* selecting process for write */
	char		*xkey;		/* extended keyboard table */
	long		hup_ospeed;	/* saved ospeed while hanging up */
	ushort		vmin, vtime;	/* min chars, timeout for RAW reads */
	long		resrvd[1];	/* for future expansion */
};


struct dev_descr
{
	DEVDRV	*driver;
	short	dinfo;
	short	flags;
	struct tty *tty;
	long	drvsize;		/* size of DEVDRV struct */
	long	fmode;
	BDEVMAP	*bdevmap;
	int	bdev;
	int	reserved;
};


struct fs_descr
{
	FILESYS	*file_system;
	short	dev_no;    	/* this is filled in by MiNT if arg == FS_MOUNT */
	long	flags;
	long	reserved[4];
};


/* number of BIOS drives */
# define NUM_DRIVES		32

# define BIOSDRV		(NUM_DRIVES)
# define PIPEDRV		(BIOSDRV + 1)
# define PROCDRV		(PIPEDRV + 1)
# define SHM_DRV		(PROCDRV + 1)
# define KERNDRV		(SHM_DRV + WITH_KERNFS)
# define UNI_NUM_DRVS		(KERNDRV + 1)

# define UNIDRV			('U' - 'A')
# define PSEUDODRVS		((1L << UNIDRV))

/* various fields for the "rdev" device numbers */
# define BIOS_DRIVE_RDEV 	0x0000
# define BIOS_RDEV		0x0100
# define FAKE_RDEV		0x0200
# define PIPE_RDEV		0x7e00
# define UNK_RDEV		0x7f00
# define PROC_RDEV_BASE		0xa000


# endif /* _mint_file_h */
