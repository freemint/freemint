.TH Pvfork 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pvfork \- create a copy of the current process
.SH SYNOPSIS
.nf
WORD Pvfork();
.fi
.SH DESCRIPTION
.I Pvfork
creates a copy of the current process. Both the child
(the new process created) and the parent (the process which first made the
call) share the same address space, i.e. any changes that the
child makes to variables will also be noticed by the parent.
The new process begins execution with an apparent return from the
.I Pvfork
call.
.PP
Because the two processes share the same address space, including the same
stack, there could be many problems if they actually were running at the
same time. Therefore, the parent process is suspended until the child
process either exits or uses mode 200 of
.I Pexec
to overlay itself with a new program in a new address space.
.SH RETURNS
0 in the child
.PP
The new process id (a positive number), in the parent.
.PP
ENSMEM if there is not enough memory to create the new process.
.SH "SEE ALSO"
.IR Pexec (2),
.IR Pfork (2)
.SH BUGS
If the parent is in supervisor mode when this call is made, the child will
be started in user mode anyway; thus, it is strongly recommended that this
call be made only from user mode.
