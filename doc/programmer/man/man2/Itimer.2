.TH Tsetitimer 2 "MiNT Programmer's Manual" "Version 1.11" "July 23, 1994"
.SH NAME
Tsetitimer \- schedule an interval timer
.SH SYNOPSIS
.nf
LONG Tsetitimer( LONG which, LONG interval, LONG value, LONG ointerval, LONG ovalue );
.fi
.SH DESCRIPTION
.I Tsetitimer
is used to schedule a periodic timer.  One of three timers, specified by
.I which ,
may be selected.  Timer 0 runs in real time, and sends a SIGALRM upon
expiration.  Timer 1 runs in process virtual time (that is, only when the
process that scheduled the timer is running) and sends a SIGVTALRM upon
expiration.  Timer 2 runs both in process virtual time and when the
operating system is running on behalf of the process that scheduled it, and
sends a SIGPROF upon expiration.
.PP
.I value
is the address of a longword specifying the initial timer value in
milliseconds.  If
.I value
== 0, then the current timer value remains unchanged.  If the longword that
.I value
points to contains 0, the timer is cancelled.
.PP
Upon expiration of the initial timer value, the timer value is reset to the
longword pointed to by
.I interval .
If
.I interval
== 0, the previously requested interval remains unchanged. If the longword that
.I interval
points to contains 0, the timer will be cancelled upon its first expiration.
.PP
.I ovalue
and
.I ointerval
point to longwords into which will be written the current value and interval
of the timer.  If
.I ovalue
or
.I ointerval
== 0, the old values will not be passed back to the calling process.
.SH RETURNS
If an error occurs, a GEMDOS error code is returned.
.PP
Upon success, 0 is returned.
.SH "SEE ALSO"
.IR Pause (2),
.IR Psignal (2),
.IR Talarm (2)
.SH BUGS
Internal calculations are done in milliseconds, so setting a timer more than
2 million seconds or so into the future will not work correctly.
