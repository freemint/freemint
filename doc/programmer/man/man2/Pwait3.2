.TH Pwait 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pwait, Pwait3, Pwaitpid \- get exit status of children
.SH SYNOPSIS
.nf
LONG Pwait();
LONG Pwait3( WORD flag, LONG *rusage );
LONG Pwaitpid( WORD pid, WORD flag, LONG *rusage );
.fi
.SH DESCRIPTION
.I Pwaitpid
attempts to determine the exit code for certain stopped children or children
that have terminated.
.I pid
determines which children's exit status are of interest. If pid
is -1, all children are of interest. If
.I pid
is less than -1,
only children whose process group matches 
.I -pid
are of interest. If
.I pid
is equal to 0, only children with the same process group ID as the
caller are of interest. If
.I pid
is greater than 0, only the child with the given process ID is of
interest.
.PP
If bit 1 of
.I flag
is set, then children that are stopped due to job control are reported;
otherwise only children that have actually terminated are reported. A
stopped process will be reported at most once (unless it is re-started and
stopped again); similarly a terminated process will be
reported only once.
.PP
If the process does have children which are of interest to the
caller (as specified by
.IR pid ),
but none are currently stopped or terminated and not yet waited for,
then the behavior of
.I Pwaitpid
is controlled by bit 0 of
.IR flag .
If it is clear, the function will wait until some child is stopped or
terminates; if it is set, the function will return immediately.
.PP
The
.I rusage
parameter, if non-zero, should point to two long words, into which
information about the child's CPU time usage is placed, as follows:
.nf
rusage[0]: milliseconds spent by child in user space
rusage[1]: milliseconds spent by child in kernel space
.fi
.PP
The
.IR Pwait3 ()
system call is equivalent to
.IR Pwaitpid (-1,
.IR flag ,
(LONG *)0),
and is provided for backward compatibility. The
.IR Pwait ()
system call is equivalent to
.IR Pwaitpid (-1,
2, (LONG *)0)
and is provided only for compatibility with old
applications; new applications should not use this
system call.

.SH RETURNS
If a child process is found which matches the
.I pid
specification given,
its process id is placed in the upper 16 bits
of the 32 bit value returned, and its exit status (as passed to
.I Pterm
or
.IR Ptermres ,
as determined implicitly by
.IR Pterm0 ,
or as determined by the type of signal that stopped or killed the process)
is placed in the lower 16 bits. If the process was stopped or terminated
by signal
.IR n ,
then its exit status will be (
.IR n <<
8 ) |
.IR x .
where
.I x
is 127 if the process was stopped and 0 if the process was terminated.
.PP
0 if bit 0 of
.I flag
is set and the
.I Pwaitpid
system call would have otherwise blocked waiting for a child to exit
or stop.
.PP
EFILNF if no unwaited for children exist which are "of interest"
as specified by
.IR pid .

.SH "SEE ALSO"
.IR Pexec (2),
.IR Pterm (2),
.IR Pterm0 (2),
.IR Ptermres (2)
.SH BUGS
Note that
.I Pwait
waits for stopped processes, unlike the POSIX
.I wait
function.
.I Pwait
is obsolete anyway, and library writers are advised to use
.I Pwaitpid
or
.I Pwait3
to implement POSIX
.IR wait .
