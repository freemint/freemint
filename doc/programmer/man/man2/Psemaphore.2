.TH Psemaphore 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Psemaphore \- create / use / destroy a semaphore
.SH SYNOPSIS
.nf
LONG Psemaphore( WORD mode, LONG id, LONG timeout );
.fi
.SH DESCRIPTION
.IR Psemaphore
is a call that implements uncounted semaphores. A semaphore is used for
mutual exclusion: only one process at a time may "own" a given semaphore.
.PP
For example, a semaphore may be used to protect access to data structures
which are in shared memory and which are used by multiple threads in a
process: before using the memory a thread must gain ownership of the
guarding semaphore, and when finished the thread must release the
semaphore. The semaphore would be created during initialization and
destroyed during shutdown.
.PP
Semaphores are identified by an \fIid\fR, which is an arbitrary longword.
This is the semaphore's "name." The \fIid\fR used to create the semaphore
is the "name" of that semaphore from then on.
When using
semaphores, you should strive to use a longword that is unique. Using four
ASCII characters which spell out something is common: 0x4b4f444f ("MODM") for
instance might be the \fIid\fR of a semaphore that controls access to a modem.
(Actually, this would be a poor
choice, since there can be more than one modem in a system and this 
semaphore ID isn't flexible enough to handle that.  "MDM1" might be better.)
.PP
Semaphore id's beginning with 0x5f
(the underscore character) are reserved for operating system use.
.PP
The \fItimeout\fR argument is only used in Mode 2. It is ignored in other
modes. A \fItimeout\fR of zero means "return immediately."  A value of -1
means "forever" \- that is, never time out. Other values are a number of
milliseconds to wait for the semaphore before timing out.
.PP
The \fImode\fR argument is used to tell what operation the caller desires:
.PP
.IP MODE
ACTION
.IP 0
Create and "get" a semaphore with the given ID.
.IP 1
Destroy the indicated semaphore. The caller must own the
semaphore prior to making this call.
.IP 2
Request ownership of the semaphore. Blocks until the
semaphore is free or until the timeout expires. See below.
.IP 3
Release the semaphore. The caller must own the semaphore
prior to making this call.
.SH RETURNS
.IP CODE
MEANING
.IP 0
Success.
.IP ERROR
A request for a semaphore which the caller already owns.
.IP ERANGE
The semaphore does not exist.
.IP EACCDN
Failure. The semaphore already exists (mode 0), you don't
own it (modes 1 and 3), or the request timed out (mode 2).

.SH NOTES
When you create a semaphore you also own it, so you must release it before
anybody else can get it. If you are blocked waiting for a semaphore (mode 2
before the timeout expires) and somebody destroys the semaphore, the
call will return ERANGE to you (because the requested semaphore does
not exist any more!).
.PP
When a
process terminates, any semaphores it owns are released.
.PP
At most one process may own a semaphore.  
Ownership is not inherited by children (fork()) or across exec().
.PP
Once created, semaphores are never destroyed
except upon request. Thus if a program creates a semaphore and then crashes
the semaphore will never go away.
.PP
Semaphores can be implemented using
named pipes and file locking. Technically, then, semaphores are redundant.
The facility is included as a kernel call because it was deemed necessary
to have this kind of exclusion available with little persistent state or
system-call overhead.
