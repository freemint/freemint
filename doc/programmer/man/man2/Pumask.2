.TH Pumask 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pumask \- set file and directory creation mask
.SH SYNOPSIS
.nf
WORD Pumask( WORD mode );
.fi
.SH DESCRIPTION
.I Pumask
changes the file and directory creation mask of the current process to
the unsigned 16 bit quantity specified in
.IR mode .
The old value of the creation mask is returned. Child processes inherit
the new value for the mask.
.PP
When a new file is created with
.I Fcreate
or a new directory created with
.I Dcreate
the initial access permissions (see
.I Fchmod
for a description of these) for the newly created file or directory are
normally set so that all permissions are granted (except that execute
permission is not normally granted for files). The creation mask set
by
.I Pumask
determines which permissions are
.I not
to be granted by default. Thus, files created after a
Pumask(S_IWOTH|S_IWGRP|S_IXOTH) call will be readable by anyone,
but writable only by the owner; moreover, directories created after this
call would be searchable by the owner and members of the same group,
but not by anyone else.
.SH "SEE ALSO"
.IR Dcreate (2),
.IR Fchmod (2),
.IR Fcreate (2)
.SH BUGS
.I Pumask
doesn't affect explicit
.I Fchmod
calls, as some might expect.
