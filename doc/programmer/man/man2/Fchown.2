.TH Fchown 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fchown \- change a file's ownership
.SH SYNOPSIS
.nf
LONG Fchown( char *name, WORD uid, WORD gid );
.fi
.SH DESCRIPTION
.I Fchown
changes a file's user and group ownership to
.I uid
and
.I gid
respectively. These ownerships determine access rights to the file.
.PP
Only a process with effective uid 0 or whose effective uid matches
the user ownership of the file may make this call. In the latter case,
the new
.I uid
must match the old one, and the calling process must also be a member
of the group specified by
.IR gid .
.SH RETURNS
0 on success
.PP
EACCDN if the calling process has an effective uid which differs from the
owner of the file and which is not 0.
.PP
EINVFN if the file system on which the file is located does not support
a notion of ownership. (This is true of the normal TOS file system.)
.PP
EFILNF if the file is not found
.PP
EPTHNF if the path to the file is not found.
.SH "SEE ALSO"
.IR Fchmod (2),
.IR Fxattr (2)
.SH BUGS
Most file systems don't understand file ownership.
.PP
In practice, the call is useful only to processes with effective uid 0.
