Appendix E: MiNT File Systems

MiNT allows loadable file systems, which means that it should be quite
easy to implement networked file systems, dynamically re-sizable ram
disks, or other nifty things (for example, Stephen Henson's minix.xfs
file system allows access to Minix partitions from TOS). Writing
these is not difficult, but there are a lot of data structures that
must be understood first. (These data structures are given in the
Appendix D.)

A note on conventions: a declaration like:
	short foo P_((char *bar, long baz));
means that "foo" is a function that returns a 16 bit integer, and that
expects a pointer to a character and a 32 bit integer as its two
arguments. "ushort" is an unsigned 16 bit integer; "ulong" is an
unsigned 32 bit integer.

File Cookies

Files and directories are represented in the kernel by "cookies".
The contents of the cookie are mostly file system dependent, i.e. the
kernel interprets only the "fs" and "dev" field of the cookie, and the
contents of the other fields may be used by a file system as it sees fit.

A file cookie has the following structure:

typedef struct f_cookie {
	FILESYS *fs;		/* file system that knows about this cookie */
	ushort	dev;		/* device info (e.g. Rwabs device number) */
	ushort	aux;		/* extra data that the file system may want */
	long	index;		/* this+dev uniquely identifies a file */
} fcookie;

(The "FILESYS" data type is defined below.)
The interpretation of the "aux" field is entirely file system dependent. The
"index" field is not presently used by the kernel, but file systems
should (if possible) make this field uniquely identify a file or directory
on a device.

File System Structure

This is the structure that tells the kernel about the file system,
and gives the entry points for routines which the kernel can call in order
to manipulate files and directories. Note that actual input/output
operations are performed by a device driver; most file systems have just
one associated device driver, but some may have more. See the section on
device drivers for more information on these.

Unless otherwise specified, all of the functions should return 0 for
success and an appropriate (long negative) error code for failure. Also,
note that it is the kernel's responsibility to do all access checking;
the file system may assume that the file's permissions have been checked
and are compatible with the current process' uid and the operation
selected.

Parameters are passed to file system driver functions on the stack.
The file system drivers should preserve registers d2-d7 and a2-a7,
and return any results in register d0. Note that this may differ from
your compiler's default conventions (for example, Alcyon C preserves
only registers d3-d7 and a3-a7); in this case, an assembly language
wrapper will be necessary.

typedef struct filesys {
	struct	filesys	*next;

This is a link to the next file system in the kernel's list. It will be
filled in by the kernel; the file system should leave it as NULL.

	long	fsflags;
These flags give some information about the file system. Currently, three
flags are defined:

#define FS_KNOPARSE	 0x01	/* kernel shouldn't do parsing */
#define FS_CASESENSITIVE 0x02	/* file names are case sensitive */
#define FS_NOXBIT	 0x04	/* if a file can be read, it can be executed */
#define FS_LONGPATH	 0x08	/* the 'getname' function obeys the
				   'size' argument restrictions */

Other bits may be defined in future releases of MiNT; for now all other
bits in this flag should be 0. Most file systems will have only the
FS_NOXBIT and FS_LONGPATH flags; networked file systems may have FS_KNOPARSE
for reasons of efficiency, and file systems that must be compatible with Unix
or similar specifications may be case sensitive and hence have FS_CASESENSITIVE
set.
See the description of the (*getname) function for details on the effects
of the FS_LONGPATH flag.

	long	(*root) P_((short drv, fcookie *fc));
This is the entry point for a routine to find a file cookie for the root
directory of BIOS device "drv" (an integer in the range 0-31 inclusive).
This function is called by the kernel when initializing a drive; the kernel
will query each file system in turn for a file cookie representing the
root directory of the drive. If the file system recognizes the data on
the drive as being valid for a file system that it recognizes, it should
fill in the cookie pointed to by "fc" and return 0. Otherwise, it should
return a negative error code (EDRIVE is a good choice) to indicate that the
drive must belong to another file system. Note that this function is called
at boot up time and also at any time when media change is detected on a drive.

	long	(*lookup) P_((fcookie *dir, char *name, fcookie *fc));
Translate a file name into a cookie. "dir" is the cookie for a directory,
returned by a previous call to (*lookup) or (*root). "name" is either the
name of a file in that directory (if fsflags & FS_KNOPARSE == 0) or a path
name relative to that directory (if fsflags & FS_KNOPARSE == FS_KNOPARSE).
If the file is not found, an appropriate error code (like EFILNF) should be
returned. If the file is found, the cookie "*fc" should be filled in with
appropriate data, and either 0 or EMOUNT returned. EMOUNT should be returned
only if "name" is ".." and "dir" represents the root directory of a drive;
0 should be returned otherwise. Note that a lookup call with a null name
or with "." should always succeed and return a cookie representing the
directory itself. Also note that symbolic links should *never* be followed.

	long	(*creat) P_((fcookie *dir, char *name, ushort mode,
				short attrib, fcookie *fc)
Create a new file named "name" in the directory whose cookie is "dir".
"mode" gives the file's type and access permissions, as follows:
	/* file types */
	#define S_IFMT	0170000		/* mask to select file type */
	#define S_IFCHR	0020000		/* BIOS special file */
	#define S_IFDIR	0040000		/* directory file */
	#define S_IFREG 0100000		/* regular file */
	#define S_IFIFO 0120000		/* FIFO */
	#define S_IMEM	0140000		/* memory region or process */
	#define S_IFLNK	0160000		/* symbolic link */

	/* special bits: setuid, setgid, sticky bit */
	#define S_ISUID	04000		/* change euid when executing this file */
	#define S_ISGID 02000		/* change egid when executing this file */
	#define S_ISVTX	01000		/* not implemented */

	/* file access modes for user, group, and other*/
	#define S_IRUSR	0400		/* read access for user */
	#define S_IWUSR 0200		/* write access for user */
	#define S_IXUSR 0100		/* execute access for user */
	#define S_IRGRP 0040		/* ditto for group... */
	#define S_IWGRP	0020
	#define S_IXGRP	0010
	#define S_IROTH	0004		/* ditto for everyone else */
	#define S_IWOTH	0002
	#define S_IXOTH	0001

"attrib" gives the standard TOS attribute byte. This is slightly redundant
with "mode" (i.e. the FA_RDONLY bit should agree with the settings of
S_IWUSR, S_IWGRP, and S_IWOTH, and FA_DIR should be set if and only if
the file's format is S_IFDIR) but is provided for convenience for standard
DOS compatible file systems.

The kernel will make the "creat" call only after using "lookup" in an attempt
to find the file. The file system should create the file (if possible) and
set the cookie pointed to by "fc" to represent the newly created file.
If an error of any sort occurs, an appropriate error number is returned,
otherwise 0 is returned. Also note that the kernel will not try to
create directories this way; it will use "mkdir" (q.v.) instead.

	DEVDRV *(*getdev) P_((fcookie *fc, long *devspecial))
Get the device driver which should be used to do i/o on the file whose
cookie is "fc". If an error occurs, a NULL pointer should be returned
and an error code placed in the long pointed to by "devspecial"; otherwise,
"devspecial" should be set to a device-driver specific value which will
be placed by the kernel in the "devinfo" field of the FILEPTR structure
passed to the device driver's open routine. (The interpretation of
this value is a matter for the file system and the device driver, the
kernel doesn't care.)

If the call to (*getdev) succeeds, a pointer to a device driver structure
(see below) is returned; if it fails, a NULL pointer should be returned and
an appropriate error number placed in *devspecial.

	long	(*getxattr) P_((fcookie *file, XATTR *xattr));
Get a file's attributes. The XATTR structure pointed to by "xattr" should
be filled in with the data for the file or directory represented by
the cookie "*file", and 0 returned. If a fatal error occurs (e.g. media
change) an error code is returned instead. The XATTR structure is defined
as follows:

/* structure for getxattr */
	typedef struct xattr {
	ushort	mode;
file types and permissions; same as the mode passed to (*creat) (see above)
	long	index;
file index; this should if possible be a unique number for the file, so that
no two files on the same physical drive could have the same index. It is
not mandatory that this match the "index" field of the fcookie structure
for the file.
	ushort	dev;
physical device on which the file is located; normally set to file->dev
	ushort	reserved1;
set to 0
	ushort	nlink;
number of hard links to the file; normally 1
	ushort	uid;
a number representing the user that owns the file
	ushort	gid;
the group ownership of the file
	long	size;
length of the file, in bytes
	short	mtime, mdate;
last modification time and date of the file, in standard GEMDOS format
	short	atime, adate;
last access time and date for the file, in standard GEMDOS format; if the
file system does not keep separate record of these, they should be the same
as mtime and mdate
	short	ctime, cdate;
file creation time and date, in standard GEMDOS format; if the file system
does not keep separate record of these, they should be the same as mtime
and mdate
	short	attr;
TOS attribute byte for the file in the lower 8 bits; the upper 8 should
be 0
	short	reserved2;
reserved, set to 0
	long	reserved3[2];
reserved, set both long words to 0
	} XATTR;

	long	(*chattr) P_((fcookie *file, short attr));
Change the TOS attributes of the file whose cookie is "*file" to "attr".
Only the lower 8 bits of "attr" should be considered significant, for now.
The kernel will not allow changes if the file's current attributes include
the FA_DIR bit or the FA_LABEL bit. Not all filesystems will support all
TOS attribute bits, but FA_RDONLY should probably be supported if possible;
usually setting the FA_RDONLY bit should be equivalent to turning off all
write permissions to the file.

	long	(*chown) P_((fcookie *file, short uid, short gid));
Change a file's user and group ownership to "uid" and "gid" respectively.
The kernel checks access permissions before making this call, so file
systems do not have to. If the file system does not support a concept
of ownership, or does not allow changes to ownership, it should return
EINVFN.

	long	(*chmode) P_((fcookie *file, ushort mode));
Change a file's access permissions. "mode" is similar to the field in
the XATTR structure or the value passed to creat, except that _only_
the permission bits are significant; (mode & S_IFMT) will always be 0.
In the event that the file system supports only a subset of permissions
(e.g. the TOS file system can only control write access to the file)
then it may consider only the relevant bits of "mode".

	long	(*mkdir) P_((fcookie *dir, char *name, ushort mode));
Make a new subdirectory called "name" of the directory whose cookie is
"*dir". The new directory should have the file permissions given by
"mode & ~S_IFMT". Note that the file system should do all appropriate
initializations for the new directory, including making entries for
"." and "..". Note also that the kernel verifies that "mode & S_IFMT"
is S_IFDIR before making this call.

	long	(*rmdir) P_((fcookie *dir, char *name));
Delete the subdirectory called "name" of the directory whose cookie is
"*dir". It is also a good idea to allow removal of symbolic links via
this call, since a symbolic link to a directory looks like a directory
to a normal TOS program.

	long	(*remove) P_((fcookie *dir, char *name));
Delete the file called "name" in the directory "*dir". This function should
act like the Unix "unlink" call, i.e. if the file has more than 1 hard
link to it, only this particular link to the file should be removed and
the file contents should not be affected. Directories should not be removed
by this function. Symbolic links definitely should be removed by this
function; whether other types of special files are removed by this function
is up to the file system.

	long	(*getname) P_((fcookie *relto, fcookie *dir, char *pathname,
			       short size));
This is analogous to the "getcwd()" operation in Unix. It should get the name
of the directory whose cookie is "*dir", expressed as a path relative
to the directory whose cookie is "*relto"; normally, this is the root
directory, but the file system should not assume this. The resulting path
is placed in the array pointed to by pathname, which is at least
"size" bytes long. If *relto and *dir are the same directory, then an empty
string should be placed in *pathname. If the complete path name (including
trailing 0, if any) will not fit into the space provided, then ERANGE
should be returned; in this case *pathname may or may not have any
meaningful information stored into it (i.e., a partial pathname may be,
but need not be, returned).

Note: The "size" argument is a new feature (added in MiNT 0.97). New file
systems should always respect this argument, and indicate this by setting
the FS_LONGPATH bit in the "fsflags" field of the FILESYS structure.
If FS_LONGPATH is clear, the kernel will assume that the file system is
an old file system which always assumes a 128 byte maximum path size.

Example: if "*relto" is the directory "\FOO", and "*dir" is the directory
"\FOO\BAR\SUB", then after the call to (*getname) "pathname" should contain
"\BAR\SUB" (assuming that "size" is at least 9).

	long	(*rename) P_((fcookie *olddir, char *oldname,
			    fcookie *newdir, char *newname));
Rename the file with name "oldname" contained in the directory whose cookie
is "*olddir" to the name "newname" in the directory whose cookie is "*newdir".
The file system need not actually support cross-directory renames, or
indeed any sort of renames at all; if no renames at all are supported, EINVFN
should be returned.

	long	(*opendir) P_((DIR *dirh, short tosflag));
Open a directory for reading. "dirh" is a pointer to a structure as defined
below; the file cookie for the directory being opened may be found there.
"tosflag" is a copy of "dirh->flags" and is in a sense redundant. The
file system should initialize the "fsstuff" and "index" fields of "*dirh"
to whatever it needs for carrying out a successful search.

/* structure for opendir/readdir/closedir */
typedef struct dirstruct {
	fcookie fc;		/* cookie for this directory */
	ushort	index;		/* index of the current entry */
	ushort	flags;		/* flags (e.g. tos or not) */
#define TOS_SEARCH	0x01
/* if TOS_SEARCH is set, this call originated from a TOS Fsfirst() system
 * call -- if possible, the returned names should be acceptable to a
 * "naive" TOS program
 */
	char	fsstuff[60];	/* anything else the file system wants */
} DIR;

	long	(*readdir) P_((DIR *dirh, char *name, short namelen, fcookie *fc));
Read the next name from the directory whose DIR structure (see above)
is "*dirh". The name should be copied into "name" (if dirh->flags & TOS_SEARCH
is nonzero) or "name+4" if dirh->flags & TOS_SEARCH is 0; in the latter case,
the first 4 bytes of "name" should be a unique index for the file. "namelen"
is the total size of the buffer for "name"; if the next file name (plus index,
if appopriate, and including the trailing 0) is too long for the buffer, as
much of it as will fit should be copied in and ENAMETOOLONG returned. If no
more file names remain unread in the directory, ENMFIL should be returned
and "name" left unchanged. Otherwise, 0 should be returned.
Note that volume labels should not be read by this function; only
"readlabel" (q.v.) should see these.

	long	(*rewinddir) P_((DIR *dirh));
Reset the file system specific fields of "*dirh" so that the next call to
"readdir" on this directory will return the first file name in the directory.

	long	(*closedir) P_((DIR *dirh));
Called by the kernel when the directory "*dirh" is not going to be searched
any more; if the file system needs to clean up any structures or free memory
allocated for the search it can do so here.

	long	(*pathconf) P_((fcookie *dir, short which));
Get path configuration information for the directory whose cookie is
"*dir". "which" indicates what kind of information should be returned,
as follows:

/* The requests for pathconf() */
#define DP_IOPEN	0	/* internal limit on # of open files */
#define DP_MAXLINKS	1	/* max number of hard links to a file */
#define DP_PATHMAX	2	/* max path name length */
#define DP_NAMEMAX	3	/* max length of an individual file name */
#define DP_ATOMIC	4	/* # of bytes that can be written atomically */
#define DP_TRUNC	5	/* file name truncation behavior */
/* possible return values for DP_TRUNC */
#	define	DP_NOTRUNC	0	/* long names cause an error */
#	define	DP_AUTOTRUNC	1	/* long names are truncated */
#	define	DP_DOSTRUNC	2	/* DOS 8+3 rules are used */
#define DP_CASE		6	/* file name case conversion */
/* possible values returned for DP_CASE */
#	define	DP_CASESENS	0	/* case sensitive */
#	define	DP_CASECONV	1	/* case always converted */
#	define	DP_CASEINSENS	2	/* case insensitive, preserved */

#define DP_MAXREQ	6	/* highest legal request */
/* Dpathconf and Sysconf return this when a value is not limited
   (or is limited only by available memory) */
#define UNLIMITED	0x7fffffffL

	long	(*dfree) P_((fcookie *dir, long *buf));
Determine bytes used and free on the disk that the directory whose cookie
is "*dir" is contained on. "buf" points to the same kind of buffer that
the "Dfree" system call uses; see the documentation for that call.

	long	(*writelabel) P_((fcookie *dir, char *name));
Create a volume label with the indicated name on the drive which contains
the directory whose cookie is "*dir". If a label already exists, the file
system may either fail the call with EACCDN or re-write the label. If the
file system doesn't support the notion of labels, it should return EINVFN.

	long	(*readlabel) P_((fcookie *dir, char *name, short namelen));
Read the volume label for the disk whose root directory has the cookie
"*dir" into the buffer "name", which is "namelen" bytes long. If the
volume label (including trailing 0) won't fit, return ENAMETOOLONG. If
*dir is not the cookie of a root directory, or if no volume label
exists (perhaps because the file system doesn't support them), return
EFILNF.

	long	(*symlink) P_((fcookie *dir, char *name, char *to));
Create a symbolic link called "name" in the directory whose cookie is
"*dir". The link should contain the 0-terminated string "to". If the file
system doesn't support symbolic links, it should return EINVFN.

	long	(*readlink) P_((fcookie *file, char *buf, short buflen));
Read the contents of the symbolic link whose cookie is "*file" into the
buffer "buf", which is "buflen" bytes long. If the contents (including the
trailing 0) won't fit, return ENAMETOOLONG; if the file system doesn't
do symbolic links, return EINVFN.

	long	(*hardlink) P_((fcookie *fromdir, char *fromname,
				fcookie *todir, char *toname));
Create a hard link called "toname" in the directory whose cookie is
"*todir" for the file named "fromname" in the directory whose cookie is
"*fromdir". If the file system doesn't do hard links, return EINVFN.

	long	(*fscntl) P_((fcookie *dir, char *name, short cmd, long arg));
Perform an operation on the file whose name is "name", in the directory with
cookie "*dir". "cmd" and "arg" specify the operation, and are file system
specific. See the documentation for Dcntl() for more details. Most file
systems will just return EINVFN for any values of "cmd" and "arg"; this call
is here so that you can provide users with a way to manipulate various special
features of your file system.

	long	(*dskchng) P_((short drv));
Check for media change. "drv" is the BIOS device number on which the
kernel thinks there has been a change. This function is called only
when the kernel detects what the BIOS claims is a definite disk change
(i.e. Mediach returning 2 or Mediach returning 1 and Rwabs returning -14).
This may be the result of a program trying to force a media change; if the
file system agrees that a change has occured, it should perform any
appropriate actions (e.g. invalidating buffers) and return 1; the kernel
will then invalidate any open files or directories on the device and
re-check what file system the device belongs  If no change, in fact, occured,
a 0 should be returned to tell the kernel not to worry.

	long	(*release) P_((fcookie *fc));
Called by the kernel to indicate that the cookie "*fc" is no longer
in use. "*fc" was previously filled in by either the (*root)(),
(*lookup)(), (*creat)(), (*readdir)(), or (*dupcookie)() call. "release"
is called exactly once for each cookie, and should always succeed and
return 0 to the kernel.

	long	(*dupcookie) P_((fcookie *dest, fcookie *src));
Copy the fcookie given in "src" into "dest," and perform whatever
file system specific bookkeeping is necessary to keep track of
how many times a cookie is in use (if the file system cares).
If the file system doesn't care, it should just do the
equivalent of *dest = *src.

} FILESYS;

typedef struct fileptr {
	short	links;	    /* number of copies of this descriptor */
	ushort	flags;	    /* file open mode and other file flags */
#define O_RWMODE  	0x03	/* isolates file read/write mode */
#	define O_RDONLY	0x00
#	define O_WRONLY	0x01
#	define O_RDWR	0x02
#	define O_EXEC	0x03	/* execute file; used by kernel only */

#define O_APPEND	0x08	/* all writes go to the end of the file */

#define O_SHMODE	0x70	/* isolates file sharing mode */
#	define O_COMPAT	0x00	/* compatibility mode */
#	define O_DENYRW	0x10	/* deny both read and write access */
#	define O_DENYW	0x20	/* deny write access to others */
#	define O_DENYR	0x30	/* deny read access to others */
#	define O_DENYNONE 0x40	/* don't deny any access to others */

#define O_NOINHERIT	0x80	/* children can't access via this file descriptor */
#define O_NDELAY	0x100	/* don't block for i/o on this file */
#define O_CREAT		0x200	/* create file if it doesn't exist */
#define O_TRUNC		0x400	/* truncate file to 0 bytes if it does exist */
#define O_EXCL		0x800	/* fail open if file exists */
#define O_TTY		0x2000	/* file is a terminal */
#define O_HEAD		0x4000	/* file is a pseudo-terminal "master" */
#define O_LOCK		0x8000	/* file has been locked */

The "flags" is constructed by or'ing together exactly one read/write mode,
one sharing mode, and any number of the other bits. Device drivers can
ignore the O_CREAT flag, since file creation is handled by the kernel.
The O_TRUNC flag, however, should be respected; the file should be
truncated to 0 length if this flag is set.

	long	pos;	    /* position in file */
The kernel doesn't actually use this field, except to initialize it to
0; it is recommended that device drivers that allow seeking should use it
to store the current position in the file (relative to the start of
the file). Other device drivers may use it for other purposes.
	long	devinfo;    /* device driver specific info */
This field is passed back to the kernel from the file system from the
"getdev" call; its interpretation is file system specific, except that
if this is a terminal device (i.e. the O_TTY bit is set in "flags") then
this must be a pointer to a struct tty for this terminal.
	fcookie	fc;	    /* file system cookie for this file */
This is the cookie for the file, as returned by the file system "lookup"
function during opening of the file.
	struct devdrv *dev; /* device driver that knows how to deal with this */
This is the device driver returned by the "getdev" call.
	struct fileptr *next; /* link to next fileptr for this file */
This field may be used by device drivers to keep a linked list of file
pointers that refer to the same physical file, for example, in order to
implement file sharing or locking code.
} FILEPTR;


The Device Driver Structure

All of the functions in the device driver structure, like those in the
file system structure, are called in supervisor mode and with the
GCC calling conventions. They must preserve registers d2-d7 and a2-a7,
and results are to be returned in register d0. The BIOS, XBIOS,
GEMDOS, AES, and VDI must not be called directly from the device
driver; but GEMDOS and BIOS functions may be called indirectly via
the tables found in the kerinfo structure.

typedef struct devdrv {
	long (*open)	P_((FILEPTR *f));
This routine is called by the kernel during a file "open", after it has
constructed a FILEPTR for the file being opened and determined the device
driver. The device driver should check the contents of the FILEPTR and
make any changes or initializations necessary. If for some reason the open
call should be failed, an appropriate error code must be returned (in which
case the kernel will free the FILEPTR structure automatically). For example,
if the file sharing mode in f->flags is not compatible with the sharing
mode of another open FILEPTR referring to the same physical file, EACCDN should
be returned.

	long (*write)	P_((FILEPTR *f, char *buf, long bytes));
Write "bytes" bytes from the buffer pointed to by "buf" to the file with
FILEPTR "f". Return the number of bytes actually written. If the file
pointer has the O_APPEND bit set, the kernel will automatically perform
an "lseek" to the end of the file before calling the "write" function.
If the device driver cannot ensure the atomicity of the "lseek" + "write"
combination, it should take whatever steps are necessary here to ensure
that files with O_APPEND really do have all writes go to the end of the
file.

	long (*read)	P_((FILEPTR *f, char *buf, long bytes));
Read "bytes" bytes from the file with FILEPTR "f" into the buffer pointed
to by "buf". Return the number of bytes actually read.

	long (*lseek)	P_((FILEPTR *f, long where, short whence));
Seek to a new position in the file. "where" is the new position; "whence"
says what "where" is relative to, as follows:
/* lseek() origins */
#define	SEEK_SET	0		/* from beginning of file */
#define	SEEK_CUR	1		/* from current location */
#define	SEEK_END	2		/* from end of file */

	long (*ioctl)	P_((FILEPTR *f, short mode, void *buf));
Perform a device specific function. "mode" is the function desired. All devices
should support the FIONREAD and FIONWRITE functions, and the file locking
Fcntl functions if appropriate (see the documentation for the GEMDOS
Fcntl function).

	long (*datime)	P_((FILEPTR *f, short *timeptr, short rwflag));
Get or set the date/time of the file. "timeptr" is a pointer to two words,
the first of which is the time and the second of which is the date.
If "rwflag" is 0, the time and date of the file should be placed into timeptr.
If "rwflag" is nonzero, then the time and date of the file should be set to
agree with the time and date pointed to by timeptr.

	long (*close)	P_((FILEPTR *f, short pid));
Called every time an open file is closed. Note that the file is "really"
being closed if f->links == 0, otherwise, the FILEPTR is still being used
by some process. However, if the device driver supports file locking
then all locks held on the file by process pid should be released on
any close, even if f->links > 0. Some things to watch out for:
(1) "pid" is not necessarily the current process; some system calls
    (e.g. Fmidipipe, Pexec) can sometimes close files in a process
    other than the current one.
(2) Device drivers should set the O_LOCK bit on f->flag when the F_SETLK
    or F_SETLKW ioctl is made; they can then test for this bit when the file
    is being closed, and remove locks only if O_LOCK is set. Note that all locks
    held by process pid and referring to the same physical file as "f" may
    be removed if O_LOCK is set, not just the locks that were associated with
    the particular FILEPTR "f". If the FILEPTR has never had any lock Fcntl()
    calls made on it, locks on the associated physical file need not be (and
    should not be) removed when it is closed.

	long (*select)	P_((FILEPTR *f, long proc, short mode));
Called by Fselect() when "f" is one of the file handles a user has chosen to
do a select on. If mode is O_RDONLY, the select is for reading; if
it is O_WRONLY, it is for writing (if it is for both reading and writing,
the function will be called twice). The select function should return
1 if the device is ready for reading or writing (i.e. if a read or write
call to the device will not block); otherwise, it should take whatever
steps are necessary to arrange to wake up the process whose PROC structure is
pointed to by "proc" when the appropriate I/O on the device becomes possible.
Normally, this will be done by calling the "wakeselect" function (as passed
by the kernel in "struct kerinfo") with "proc" as its parameter.

	void (*unselect) P_((FILEPTR *f, long proc, short mode));
Called when the kernel is returning from an Fselect that had previously
selected this file or device; the device driver should no longer notify
"proc" when I/O is possible for this file or device. "mode" is the same
mode as was passed to the select() function (see above), i.e. either
O_RDONLY or O_WRONLY; as with select(), unselect() will be called twice
if both input and output were selected for.

	long reserved[3];
Reserved longwords for future expansion. These must be set to 0.
} DEVDRV;



How the File System Is Booted

A loadable file system is an ordinary TOS executable file with an
extension of ".xfs". MiNT searches its current directory (normally the
root directory of the boot disk) when it is starting for all such
files, and loads them with Pexec mode 3. It then does a jump to
subroutine call to the first instruction of the loaded program,
passing on the stack a pointer to a structure of type "struct kerinfo"
(see below) which describes the version of MiNT and provides entry points
for various utility functions.

The file system should *not* set up a stack or shrink its basepage
(so the ordinary C startup code is not necessary). MiNT has already provided
a stack of about 8K or so, and has shrunk the basepage to the bare minimum.
The file system initialization point (like all file system functions) is
called in supervisor mode; it must never switch back to user mode. It
may use registers d0, d1, a0, and a1 as scratch registers; all other
registers must be preserved.

What the file system initialization code *should* do is to check that an
appropriate version of MiNT is running and to otherwise check the system
configuration to see if it is appropriate for the file system driver.
If so, a pointer to a FILESYS structure should be returned; if not, a NULL
pointer should be returned.

Note that it is not necessary to actually check during initialization for
the presence of disks with the appropriate file system types; MiNT will
call the file system "root" function for each drive in the system, and so
such checks should be done in the "root" function.

If the file system driver wishes to add new drives to the system,
it should update the drive configuration variable stored at 0x4c2 to
reflect the presence of these new drives.

File system drivers should *not* make any calls to the BIOS or GEMDOS
directly; all such calls should be made through the vectors provided by
the kernel as part of the struct kerinfo. File system drivers should
never call the AES, VDI, or XBIOS.

All functions made visible to file systems through the kerinfo
structure. See 'kerinfo.txt' for more details about the kerinfo
structure.
