.TH Fxattr 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fxattr \- get extended attributes for a file
.SH SYNOPSIS
.nf
#include <filesys.h>

LONG Fxattr( WORD flag, char *name, XATTR *xattr );
.fi
.SH DESCRIPTION
.I Fxattr
gets file attributes for the file named
.IR name 
and stores them in the structure pointed to by
.IR xattr .
This structure is defined in the file filesys.h, and contains the following
fields of interest:
.IP "unsigned short mode"
This field gives the file type and access permissions;
(mode & S_IFMT) gives the file type (one of S_IFCHR, S_IFDIR, S_IFREG, S_IFIFO,
S_IMEM, or S_IFLNK); (mode & ~S_IFMT) gives the file access mode according
to the POSIX standard. See filesys.h for the definitions and meanings of
the constants.
.IP "long index"
An index for the file. Together with the "dev" field, this is intended to
give a unique way of identifying the file. Note, however, that not all
file systems are able to support this meaning, so it is best not to use
this field unless absolutely necessary.
.IP "unsigned short dev"
The device number for the file. This may be a BIOS device number as passed
to the
.I Rwabs
function, or it may be a device number concocted by the file system to
represent a remote device.
.IP "unsigned short nlink"
Number of hard links to the file. Normally this field will be 1.
.IP "unsigned short uid"
The user id of the owner of the file.
.IP "unsigned short gid"
The group id of the owner of the file.
.IP "long size"
The length of the file, in bytes.
.IP "long blksize"
The size of blocks on this file system.
.IP "long nblocks"
The number of physical blocks occupied by the file on the disk; this count
includes any blocks that have been reserved for the file but do not yet
have data in them, and any blocks that the file system uses internally
to keep track of file data (e.g. Unix indirect blocks).
.IP "short mtime"
The time of the last modification to the file, in standard GEMDOS format.
.IP "short mdate"
The date of the last modification, in standard GEMDOS format.
.IP "short atime, adate"
The time and date of the last access to the file, in GEMDOS format.
Filesystems that do not keep this time will return the values given
in "mtime" and "mdate" for these fields as well.
.IP "short ctime, cdate"
The time and date of the file's creation, in GEMDOS format.
Filesystems that do not keep this time will return the values given
in "mtime" and "mdate" for these fields as well.
.IP "short attr"
The standard TOS attributes for the file, as returned by
.I Fattrib
and/or
.IR Fsfirst .
.PP
The
.I flag
parameter controls whether or not symbolic links should be followed. If
it is 0, then symbolic links are followed (like the Unix
.I stat
function). If
.I flag
is 1, then links are not followed and the information returned is for
the symbolic link itself (if the named file is a symbolic link); this
behavior is like that of the Unix
.I lstat
system call.
.SH RETURNS
0 on success
.PP
EFILNF if the file is not found
.PP
EPTHNF if the path to the file is not found.
.SH "SEE ALSO"
.IR Fattrib (2),
.IR Fsnext (2)

