.TH Psigblock 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Psigblock, Psigsetmask \- block or unblock delivery of signals
.SH SYNOPSIS
.nf
LONG Psigblock( LONG mask );
LONG Psigsetmask( LONG mask );
.fi
.SH DESCRIPTION
.I Psigblock
adds the set of signals defined by the variable
.I mask
to the set of signals which are blocked from delivery.
Each bit of
.I mask
represents a signal; if bit
.I n
of
.I mask
is set, then signal number
.I n
is blocked.
.I Psigblock
returns the set of blocked signals as it was prior to the new signals being
added to it; the old set can thus be restored with the
.I Psigsetmask
call.
.PP
.I Psigsetmask
replaces the set of blocked signals with the set in
.IR mask ;
the bits of
.I mask
have the same meaning as they do for
.IR Psigblock ,
except that bits that are set to 0 will cause the corresponding signals
to no longer be blocked.
.I Psigsetmask
returns the old set of blocked signals.
.PP
Note that certain signals (e.g. SIGKILL) cannot be blocked, and the kernel
will (silently) clear the corresponding bits in
.I mask
before changing the blocked signal set.
.PP
Blocked signals remain blocked across
.I Pfork
and
.I Pvfork
calls. After a
.I Pexec
call, the child process will always start with an empty set of blocked
signals, regardless of which signals were blocked by the parent.
.SH "SEE ALSO"
.IR Pkill (2),
.IR Psignal (2),
.IR Psigpending (2)
