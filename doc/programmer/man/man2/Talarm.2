.TH Talarm 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Talarm \- schedule an alarm to occur
.SH SYNOPSIS
.nf
LONG Talarm( LONG time );
.fi
.SH DESCRIPTION
If
.I time
is greater than 0, schedule a SIGALRM signal to be delivered to the
process that many seconds from now. Note that unless a handler for
SIGALRM has been established by means of the
.I Psignal
function, then the signal will kill the process.
.PP
If
.I time
== 0,
then any previously scheduled alarm is cancelled.
.PP
If
.I time
is negative, then no change is made to what alarms are scheduled; in this
case, only the returned value (the number of seconds left before any
alarm) is of interest.
.SH RETURNS
If an alarm had been scheduled before this call to
.IR Talarm ,
return the number of seconds remaining until that previously scheduled alarm.
.PP
Otherwise, return 0.
.SH "SEE ALSO"
.IR Pause (2),
.IR Psignal (2)
.SH BUGS
Internal calculations are done in milliseconds, not seconds, so the returned
value is not exactly accurate.
.PP
For the same reason, setting an alarm more than 2 million seconds or so
into the future will not work correctly.
