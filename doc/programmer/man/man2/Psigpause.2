.TH Psigpause 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Psigpause \- suspend process until a signal is received
.SH SYNOPSIS
.nf
void Psigpause( LONG mask );
.fi
.SH DESCRIPTION
.I Psigpause
sets a new signal mask and then
causes the calling process to go to sleep until a signal that is not being
ignored or masked is received. If a signal handler has been established for
that signal with the
.I Psignal
system call, then the handler is invoked before
.I Psigpause
returns; if the handler does a longjmp to a different point in the
program, if it exits the program, or if the signal handler was set
to SIG_DFL and the default action for the signal is to terminate the
process, then
.I Psigpause
will never return.
.PP
If
.I Psigpause
does return, then the signal mask is restored to what it was prior to
the
.I Psigpause
system call,
.I i.e.
the new signal mask specified by
.I mask
is only temporary.
.SH "SEE ALSO"
.IR Pause (2),
.IR Psigblock (2),
.IR Psignal (2),
.IR Psigsetmask (2)
.SH BUGS
If a signal handler doesn't return (for example, because it uses
.IR longjmp )
then the signal mask is left set to
.IR mask .
.SH AUTHOR
Alex Kiernan
