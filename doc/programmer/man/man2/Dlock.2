.TH Dlock 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Dlock \- lock or unlock a BIOS disk device
.SH SYNOPSIS
.nf
LONG Dlock( WORD mode, WORD drv );
.fi
.SH DESCRIPTION
.I Dlock
is used to lock or unlock the BIOS device indicated by
.IR drv .
No GEMDOS file operations are permitted on a locked drive. Thus,
the
.I Dlock
call provides a mechanism for disk formatters or re-organizers to
lock out other processes while low-level BIOS or XBIOS operations
are in progress on the device.
.PP
If bit 0 of
.I mode
is 1, the drive is locked; if it is
.I 0
then the drive is unlocked and may be used again by other programs.
If a process terminates while holding a lock on a drive, that drive
is automatically unlocked.
.PP
Bit 1 of
.I mode
indicates what return values are desired. If this bit is 1, 
and if the drive is locked by a user process different from the
caller, or if some process has files open on the drive, then that
process' id will be returned (see below). This may be used to report
a more useful error message to the user (by telling him or her which
process is using the drive in question when a lock operation fails).
.PP
All other bits of
.I mode
are reserved and must be set to 0.
.PP
A lock operation followed immediately by an unlock is very similar
to a media change, except that the lock operation will fail if
there are open files that refer to the indicated drive.

.SH RETURNS
0 if the lock/unlock operation was successful
.PP
EDRIVE if
.I drv
is not a valid BIOS device number
.PP
For a lock operation, the following error codes may
be returned:
.PP
A positive process id if bit 1 of
.I mode
is set, and the drive is already locked or in use by another
user process.
.PP
EACCDN if bit 1 of
.I mode
is clear and either open files exist on the drive or another process
is searching a directory on the drive, or if bit 1 of
.I mode
is set and the drive is in use by the operating system.
.PP
ELOCKED if bit 1 of
.I mode
is clear and another process has locked the drive,
or if bit 1 of
.I mode
is set and the operating system has locked the drive.
.PP
For an unlock operation the following error codes may
be returned:
.PP
ENSLOCK if
.I mode
is 0 and the drive was not locked by this process
.SH SEE ALSO
.IR Fxattr (2)

.SH NOTES
Note that
.I Dlock
operates on BIOS devices, which may not always be in 1-1 correspondence
with GEMDOS drive letters. For this reason, to lock GEMDOS drive A:
(for example), the programmer should call
.I Fxattr
on the root directory of A: ("A:\e") and then use the
.I dev
field of the structure returned in order to determine the BIOS
device corresponding to the GEMDOS drive.

.SH BUGS
All GEMDOS operations are forbidden on a locked drive, even by the process
that created the lock. It would be useful to have a mode which allowed the
locking process (and only the locking process) to still make GEMDOS calls
on the drive, but unfortunately the structure of the OS makes this difficult;
there is also the above-mentioned difference between BIOS and GEMDOS devices
to consider.

