.TH Dreaddir 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Dreaddir \- read directory information
.SH SYNOPSIS
.nf
LONG Dreaddir( WORD len, LONG dirhandle, char *buf);
.fi
.SH DESCRIPTION
.I readdir
returns the next file in the directory whose handle
(from the
.I Dopendir
system call)
is
.IR dirhandle .
The file's name and (optionally) a 4 byte index for the file are placed
in the buffer pointed to by
.IR buf .
The file index is omitted if the directory was opened in "compatibility"
mode (see
.IR Dopendir (2)
for details); otherwise, it is placed first in the buffer, followed
by the (null terminated) name. If two names have the same index, then
they refer to the same file; the converse, however, is not true.
.PP
.I len
is the size of the buffer, in total; it should be large enough to
hold the index (if any), the file name, and the trailing 0.
.PP
Successive calls to
.I Dreaddir
will return all the names in the directory, one after another, unless
the
.I Drewinddir
system call is used to restart the reading at the beginning of
the directory.
.SH RETURNS
0 if successful
.PP
ERANGE if the buffer was not large enough to hold the index (if present)
and name
.PP
ENMFIL if there are no more file names to be read from the directory
.SH "SEE ALSO"
.IR Dclosedir (2),
.IR Dopendir (2)
.IR Drewinddir (2)
