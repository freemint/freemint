.TH Pfork 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pfork \- create a copy of the current process
.SH SYNOPSIS
.nf
WORD Pfork();
.fi
.SH DESCRIPTION
.I Pfork
creates a copy of the current process. The child
(the new process created) inherits a copy of the parent's address
space, not the parent's original memory, and so changes to variables
in the child do not affect the parent in any way.
The new process begins execution with an apparent return from the
.I Pfork
call.
.SH RETURNS
0 in the child
.PP
The new process id (a positive number), in the parent.
.PP
ENSMEM if there is not enough memory to create the new process.
.SH "SEE ALSO"
.IR Pexec (2),
.IR Pvfork (2)
.SH BUGS
If the parent is in supervisor mode when this call is made, the child will
be started in user mode anyway; thus, it is strongly recommended that this
call be made only from user mode.
.PP
The current implementation of
.I Pfork
acts like
.I Pvfork
in that the parent is suspended until the child either exits or overlays
itself with
.I Pexec
mode 200. Do not rely on this behavior, as it will be changed in future
versions of MiNT.
