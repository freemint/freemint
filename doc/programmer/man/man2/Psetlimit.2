.TH Psetlimit 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Psetlimit \- get or set limits on use of system resources
.SH SYNOPSIS
.nf
LONG Psetlimit( WORD lim, LONG value );
.fi
.SH DESCRIPTION
.I Psetlimit
gets or sets a resource limit for a process. Which limit is affected is
governed by the value of
.IR lim ,
as follows:
.nf
    1: get/set maximum CPU time for process (in milliseconds)
    2: get/set total maximum memory allowed for process
    3: get/set limit on Malloc'd memory for process
.fi
.PP
If
.I value
is negative, the limit is unchanged; if
.I value
is 0, the corresponding resource is unlimited; otherwise,
the resource limit is set to
.IR value .

.PP
Setting the "maximum memory" limit means the process is not allowed to grow
bigger than that size overall. Setting the "maximum Malloc'ed" limit means
that the process may allocate no more than that much memory. The difference
is that the latter limit applies above and beyond the text+data+bss size of
the process.

.PP
Using Psetlimit sets the corresponding limit for both the process and any
children it creates thereafter. Note that the limits apply to each process
individually; setting the child CPU limit value to 1000 and then using
\fIfork\fR(2) to create three children results in each of those children
getting a CPU limit value of one second. They do not have a
\fIcollective\fR or \fIsum total\fR limit of one second.

.PP
There is no restriction on \fIincreasing\fR a limit. Any process may
set any of its limits or its childrens' limits to a value greater than
its current limit, or even to zero (unlimited).

.PP
Memory limits do not apply during execution of Pexec; that is, if a process
is limited to (say) 256KB of memory, it can still exec a child which uses
more memory.

.PP
Memory limits are not retroactive: if a process owns 256KB of memory and
then calls Psetlimit to restrict itself to 128KB, it will not be
terminated, but no Malloc calls will succeed until its size drops below
128KB.

.PP
CPU limits are retroactive, however: if a process has used three CPU seconds
and calls Psetlimit to restrict itself to one second, it will immediately
receive SIGXCPU and terminate.

.SH RETURNS
The old limit (0 if there was no limit).

.SH BUGS
Since the limit applies to the process and its children, you can't
limit a child to, say, one millisecond of CPU time: the parent must
necessarily limit itself before creating the child, and if it has used
more than a millisecond itself, it will die with SIGXCPU.
A new set of
\fIlim\fR arguments is needed that apply to the caller's future children
without applying to the caller.

.PP
To alleviate this, the parent must create a child process via Pvfork, and
\fIthis\fR process (which hasn't consumed any CPU time yet) must call
Psetlimit and then Pexec to create the child that is meant to be limited.

.SH "SEE ALSO"
.IR Prusage (2)
