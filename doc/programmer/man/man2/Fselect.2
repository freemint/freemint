.TH Fselect 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fselect \- select file descriptors that are ready for reading or writing
.SH SYNOPSIS
.nf
WORD Fselect(WORD timeout, LONG *rfds, LONG *wfds, ((long)0));
.fi
.SH DESCRIPTION
.I Fselect
checks two sets of open file descriptors and determines which have data
ready to read, and/or which are ready to be written to. If none are ready
yet, the process goes to sleep until some member of the sets are ready or
until a specified amount of time has elapsed.
.PP
.I rfds
points to a long word which represents a set of GEMDOS file descriptors; bit
.I n
of this long word is set if file descriptor
.I n
is to be checked for input data. An empty set may optionally be represented
by a NULL pointer instead of a pointer to 0. Similarly,
.I wfds
points to a 32 bit long word which indicates which file descriptors are to
be checked for output status. When
.I Fselect
returns, the old values pointed to by
.I rfds
and
.I wfds
(if non-NULL) are overwritten by new long words indicating which file
descriptors are actually ready for reading or writing; these will always
form subsets of the file descriptors originally specified as being of
interest.
.PP
.I timeout
is a 16 bit unsigned integer specifying a maximum number of milliseconds
to wait before returning; if this number is 0, no maximum is set and
the call will block until one of the file descriptors specified is ready
for reading or writing, as appropriate. Thus,
.IR Fselect (0, 0L, 0L, 0L)
will block forever, whereas
.IR Fselect (1, 0L, 0L, 0L)
will pause for 1 millisecond.
.PP
The final argument, a long word, must always be 0 (it is reserved for
future enhancements).
.SH RETURNS
The sum of the numbers of bits set in the long words pointed to by
.I rfds
and
.IR wfds .
This will be 0 if the timeout expires without any of the specified file
descriptors becoming ready for reading or writing, as appropriate, and
nonzero otherwise.
.PP
EIHNDL if any handle specified by the long words pointed to by
.I rfds
or
.I wfds
is not a valid (open) GEMDOS handle.
.SH "SEE ALSO"
.IR Finstat (2),
.IR Foutstat (2)
.SH BUGS
The system timer is not actually accurate to the nearest millisecond, so
the timeout could last for up to 19 milliseconds longer than specified.
.PP
.I Fselect
does not work correctly on any BIOS device except the keyboard. To be
more precise, if data arrives on a BIOS device while the process is sleeping
in Fselect, the process will not be woken. To get around this bug, use
an explicit timeout.
.PP
Note that if an error condition occurs on one of the file descriptors
(for example, if the read status of a pipe with no more writers is being
requested) then
.I Fselect
will mark that file descriptor as being ready for reading (or writing, as
appropriate). This is not strictly speaking a bug, since a subsequent
read will not block (it will return an error condition), but programmers
should be aware of the possibility.
