.TH Fchmod 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fchmod \- set file access permissions
.SH SYNOPSIS
.nf
#include <filesys.h>

LONG Fchmod( char *name, WORD mode );
.fi
.SH DESCRIPTION
.I Fchmod
changes the file access permissions for the file named
.IR name .
The new access permissions are given in the word
.IR mode ,
which may be constructed by or'ing together the following symbolic constants
(defined in filesys.h):
.IP S_IRUSR, S_IWUSR, S_IXUSR
Read, write, and execute permission (respectively) for the owner of the file.
.IP S_IRGRP, S_IWGRP, S_IXGRP
Read, write, and execute permission (respectively) for the file's group.
.IP S_IROTH, S_IWOTH, S_IXOTH
Read, write, and execute permission for everybody else.
.IP S_ISUID
Set user id bit. When this file is executed, make the effective user id
of the resulting process be equal to the user id of the file, rather
than the real user id. Note that this can be very dangerous, and should
be used with care!
.IP S_ISGID
Set group id bit. When this file is executed, make the effective group id
of the resulting process be equal to the group id of the file, rather
than the real group id. Note that this can be very dangerous, and should
be used with care!
.PP
Note that not all file systems support all of these bits; bits not supported
by a file system will be ignored. The set user id
and set group id bits are meaningful only for regular files which have some
execute permissions.
.PP
Note also that "execute" permission for a directory means permission to
search the directory for a file name or name component.
.SH RETURNS
0 on success
.PP
EACCDN if the calling process has an effective uid which differs from the
owner of the file and which is not 0.
.PP
EFILNF if the file is not found
.PP
EPTHNF if the path to the file is not found.
.SH "SEE ALSO"
.IR Fattrib (2),
.IR Fxattr (2)
