.TH Pkill 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pkill \- send a signal to a process
.SH SYNOPSIS
.nf
#include <signal.h>

WORD Pkill( WORD pid, WORD sig);
.fi
.SH DESCRIPTION
.I Pkill
sends the signal described by
.I sig
to one or more processes, as follows:
.PP
If
.I pid
is a positive number, then the signal is sent to the process with that
process id.
.PP
If
.I pid
is 0, the signal is sent to all members of the process group of the calling
process (i.e. all processes which have the same process group number). This
includes, of course, the calling process itself.
.PP
If
.I pid
is less than 0, the signal is sent to all processes with process group number
.IR -pid .
.SH RETURNS
0 if successful. Note that if the current process is a recipient of the signal,
the
.I Pkill
call may not return at all if the process is killed.
.PP
ERANGE if
.I sig
is not a valid signal.
.PP
EFILNF if
.I pid
 > 0
and the indicated process has terminated or does not exist, or if
.I pid
 < 0
and there are no processes in the corresponding process group.
.PP
EACCDN if
.I pid 
> 0,
the sending process does not have an effective user id of 0,
and the recipient process has a different user id from the sending
process.
.SH "SEE ALSO"
.I Psetpgrp(2),
.IR Psignal (2)
.SH BUGS
The user id checks should also be performed when sending signals to
process groups. Do
.I not
rely on the lack of checks, as this will be corrected at some point.
