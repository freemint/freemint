.TH Pmsg 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Pmsg \- send and receive brief messages
.SH SYNOPSIS
.nf
struct msg { LONG msg1, msg2; WORD pid; };

LONG Pmsg( WORD mode, LONG mboxid, struct msg *msgptr );
.fi
.SH DESCRIPTION
.I Pmsg
sends or receives a message to a specified message box. What sort
of operation is performed depends on the bits in
.I mode
as follows:
.nf
   Mode        Operation
   0x0000      read
   0x0001      write
   0x0002      write, then read from mboxid 0xFFFFxxxx where
	       xxxx is the process id of the current process

   0x8000      OR with this bit to make the operation
	       non-blocking.
.fi
.PP
The messages are five words long: two longs and a short, in that order. 
The values  of the first two longs are totally up to the processes in
question.  The value of the short is the PID of the sender.  On return
from writes, the short is the PID of the process that read your message.
On return from reads, it's the PID of the writer.
.PP
If the 0x8000 bit is set in the mode, and there is not a reader/writer
for the mboxid already, this call returns -1. Otherwise,
a read operation waits until a message is written and a write operation
waits for a reader to receive the message.
.PP
In mode 2, the writer is declaring that it wants to wait for a reply to
the message.  What happens is that the reader gets put on the ready
queue, but the writer is atomically turned into a reader on a mailbox
whose mboxid is (0xFFFF0000 | pid). The idea is that this process
will sleep until awoken at a later time by the process that read the
message.  The process reading the original request is guaranteed not to
block when writing the reply.

.SH RETURNS
0 if successful
.PP
-1 if bit 0x8000 is set and the
.B Pmsg
call would have to block
.PP
EINVFN if
.I mode
is invalid
.SH USAGE
Used for short messages and as a way to do rendezvous and interprocess
communication with little overhead. For more complicated messages or
more general IPC, use FIFOs or pseudo-terminals.
.SH BUGS
There is no provision for a timeout.
.PP
No checking is done for deadlocks. A deadlock could arise in the following
way:
Say PID 33 writes in mode 2 to a message box and it blocks. Then somebody
writes to PD33 (
.I i.e.
message box (0xFFFF0000 | 33) )
and blocks because you're not waiting for it.  Then your
write is satisfied, so you become a reader on PD33.  We should check to
see if there are any writers on PD33 and satisfy that rendezvous! But
this could go on forever, and it's easier to stop here.  So you lose:
this situation is timing sensitive.  It shouldn't come up anyway.
.SH AUTHOR
Allan K. Pratt
