.TH Dopendir 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Dopendir \- open a directory for reading
.SH SYNOPSIS
.nf
LONG Dopendir( char *name, WORD flag );
.fi
.SH DESCRIPTION
.I Dopendir
opens the directory whose name is pointed to by
.I name
for reading. A 32 bit directory handle is returned which may be passed to
.I Dreaddir
to actually read the directory.
.I flag
controls the way directory operations are performed. If flag == 1, then
the directory is read in "compatibility" mode, if flag == 0 then
directory operations are performed in "normal" mode. In "compatibility"
mode, file systems act as if the
.I Fsfirst
and
.I Fsnext
functions were being used; in particular, if it is possible file names
will be restricted to the DOS 8 character name + 3 character extension
convention, and will be in upper case.
In "normal" mode, file systems do not attempt to restrict the range of
names. Moreover, in this mode the
.I Dreaddir
system call will also return a file index number (similar to the Unix
inode number) along with the file name.
.PP
New programs should generally
use normal mode where possible.
.SH RETURNS
A 32 bit directory handle, on success.
Note that this handle may
be negative, but will never contain the pattern 0xFF in the
upper byte, whereas all errors do contain this pattern in the upper byte.
.PP
Note also that the handle must be treated by applications as an opaque
object: different kernel versions will treat the handles differently.
.PP
EPTHNF if
.I name
is not a valid directory
.PP
EACCDN if the directory is not accessible by this program
.PP
ENSMEM if the kernel is unable to allocate memory needed for the directory
operations
.SH "SEE ALSO"
.IR Dclosedir (2),
.IR Dreaddir (2),
.IR Drewinddir (2)
