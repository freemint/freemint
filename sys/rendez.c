/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1991, 1992 Eric R. Smith
 * Copyright 1992 Atari Corporation.
 * All rights reserved.
 *
 *
 * The rendezvous:
 * a little bit of message passing with a lot of synchronization.
 *
 * Pmsg(mode,mboxid,msgptr);
 * short mode;
 * long mboxid;
 * struct { long msg1, msg2; short pid; } *msgptr;
 *
 * 'mode' bits:
 *   0000	read
 *   0001	write
 *   0002	write, then read from mboxid "PDxx" where xx is my pid.
 *
 *   8000	OR with this bit to make the operation non-blocking.
 *
 * The messages are five words long: two longs and a short, in that order.
 * The values  of the first two longs are totally up to the processes in
 * question.  The value of the short is the PID of the sender.  On return
 * from writes, the short is the PID of the process that read your message.
 * On return from reads, it's the PID of the writer.
 *
 * If the 0x8000 bit is set in the mode, and there is not a reader/writer
 * for the mboxid already on the WAIT_Q, this call returns -1.
 *
 * In mode 2, the writer is declaring that it wants to wait for a reply to
 * the message.  What happens is that the reader gets put on the ready
 * queue, but the writer is atomically turned into a reader on a mailbox
 * whose mboxid is (0xFFFF0000 .OR. pid). The idea is that this process
 * will sleep until awoken at a later time by the process that read the
 * message.  The process reading the original request is guaranteed not to
 * block when writing the reply.
 *
 * There is no provision for a timeout.

The flow is as follows:

    if (you're writing)
	if (there's somebody waiting to read)
					    (SATISFY THE RENDEZVOUS RIGHT AWAY)
	    copy msg from your data space into his proc structure
	    wake him up
	    if (you're doing a write/read)
					(TURN AROUND AND BLOCK WAITING TO READ)
		write mode 0 to your proc struct and change your mbid
		goto dosleep
	    else
		fill in his PID in your data space
		return 0;
	    endif
	else
				       (YOU HAVE TO BLOCK WAITING FOR A WRITER)
	    copy from your data space to your proc structure
	    goto dosleep
	endif
    else (you're reading)
	if (there's somebody waiting to write)
					    (SATISFY THE RENDEZVOUS RIGHT AWAY)
	    copy msg from his proc to your data space
	    if (he's doing a write/read)
					 (TURN HIM AROUND WAITING FOR A WRITER)
		change his mode from write/read to read, and his mbid
		leave him asleep
	    else
		wake him up
	    endif
	    return 0;
	else
				       (YOU HAVE TO BLOCK WAITING FOR A READER)
	    write mode 0 to your proc struct
	    goto dosleep
	endif
    endif

dosleep:
    if (non-blocking mode) return EWOULDBLOCK;
    do {
        sleep(WAIT_Q, WAIT_MB);
    } until (a rendezvous happens);
    (when you wake up...)
    if (you're reading)
	copy from your proc struct to your data space
    else
	copy the writer's PID from your proc struct to your data space
    endif
    return 0;

 * The wait_cond we use is WAIT_MB, and the mbid waited for is another
 * field in the proc structure, along with the two longs and a short you
 * need to remember what somebody else wrote to you, and the mode field for
 * what kind of operation you're doing (read, write, write/read).
 *
 * The beauty of this is that mailboxes don't need to be created or
 * destroyed, and the blocking and turnaround are atomic, and it's a way to
 * do rendezvous and interprocess communication without lots of system
 * calls (Pgetpid, Fwrite, Pause, plus synchronization worries about having
 * the reader get the message and Pkill you before you've Paused).
 *
 * Note: Say PID 33 writes in mode 2 to MBXX and it blocks. Then somebody
 * writes to PD33, and blocks because you're not waiting for it.  Then your
 * write is satisfied, so you become a reader on PD33.  We should check to
 * see if there are any writers on PD33 and satisfy that rendezvous! But
 * this could go on forever, and it's easier to stop here.  So you lose:
 * this situation is timing sensitive.  It shouldn't come up anyway.
 */

# include "rendez.h"

# include "mint/asm.h"

# include "kmemory.h"
# include "proc.h"
# include "timeout.h"


long _cdecl
p_msg(int mode, long mbid, char *ptr)
{
    int noblock;
    PROC *p;

    TRACELOW(("Pmsg(%d,%lx,%lx)",mode,mbid,ptr));

    noblock = (mode & 0x8000);
    mode &= ~0x8000;

    if (mode == 0) {
	/* read */
	/* walk the whole process list looking for a writer */
	for (p = proclist; p; p = p->gl_next) {
	    if ((p->mb_mode == 1 || p->mb_mode == 2) && p->mb_mbid == mbid) {
		/* this process is trying to write this mbox */
		goto got_rendezvous;
	    }
	}
	/* nobody is writing just now */
	goto dosleep;
    }
    else if (mode == 1 || mode == 2) {
	/* write, or write/read */
	/* walk the whole process list looking for a reader */
	for (p = proclist; p; p = p->gl_next) {
	    if (p->mb_mode == 0 && p->mb_mbid == mbid) {
		/* this process is reading this mbox */
		goto got_rendezvous;
	    }
	}
	/* nobody is reading just now */
	curproc->mb_long1 = *(long *)ptr;	/* copy the message */
	curproc->mb_long2 = *(long *)(ptr+4);	/* into my proc struct */
	goto dosleep;
    }
    else return ENOSYS;			/* invalid mode */

/*
 * we get here if we actually have a rendezvous between p and curproc.
 */

got_rendezvous:
    if (!mode) {
	/* curproc is reading */
	*(long *)(ptr) = p->mb_long1;		/* copy the message */
	*(long *)(ptr+4) = p->mb_long2;		/* from his proc struct */
	*(short *)(ptr+8) = p->pid;		/* provide the PID */

	if (p->mb_mode == 2) {
	    /*
	     * The blocked write was in mode 2: writer becomes a reader and
	     * stays on WAIT_Q waiting on WAIT_MB.
	     */
	    p->mb_mbid = 0xFFFF0000L | p->pid;
	    p->mb_mode = 0;
	}
	else {
	    short sr = spl7();
	    /* The blocked write was in mode 1: writer wakes up */
	    p->mb_writer = curproc->pid;	/* tell writer reader's pid */
	    p->mb_mode = -1;			/* mark rendezvous */
	    p->wait_cond = 0;			/* not necessary? */
	    if (p->wait_q != READY_Q) {
		/* wake up the writer if it is sleeping */
		rm_q(p->wait_q,p);
		add_q(READY_Q,p);
	    }
	    spl(sr);
	}
	return 0;
    }
    else {
	short sr = spl7();
	/* curproc is writing */
	p->mb_writer = curproc->pid;		/* provide the PID */
	p->mb_long1 = *(long *)(ptr);		/* copy the message */
	p->mb_long2 = *(long *)(ptr+4);
	p->mb_mode = -1;			/* mark rendezvous */
	p->wait_cond = 0;			/* not necessary? */
	if (p->wait_q != READY_Q) {
	    /* wake up the reader if it is sleeping */
	    rm_q(p->wait_q,p);
	    add_q(READY_Q,p);
	}
	spl(sr);
	if (mode == 2) {
	    /* now curproc becomes a reader */
	    mbid = 0xFFFF0000L | curproc->pid;
	    mode = 0;
	    goto dosleep;
	}
	else {
	    *(short *)(ptr+8) = p->pid;		/* tell reader writer's pid */
	    return 0;
	}
    }

/*
 * we get here when a read or write should block because there's nobody
 * already waiting at the other end.
 */

dosleep:
	if (noblock) {
	    return -1L;
	}
	curproc->mb_mbid = mbid;	/* and ID waited for */
	curproc->mb_mode = mode;	/* save mode */

/*
 * OK: now we sleep until a rendezvous has occured. The loop is because we
 * may be woken up to deal with signals before the rendezvous.  The thing
 * that says the rendezvous actually happened is that mb_mode got changed
 * to -1.
 */
	do {
		sleep(WAIT_Q, WAIT_MB);			/* block */
	} while (curproc->mb_mode != -1);

	/*
	 * When we wake up, we transfer the message from our proc struct
	 * into our message pointer space if we were reading, and do
	 * nothing if we were writing.  The write/read case is taken care
	 * of without waking this process up at all.
	 *
	 * Check curproc->mode because it may not be the mode we started
	 * with any more.
	 */
	if (mode == 0) {
	    *(long *)(ptr) = curproc->mb_long1;
	    *(long *)(ptr+4) = curproc->mb_long2;
	}
	/* in any case, copy the 'writer' field because that's the PID */
	/* of the other side of the rendezvous */
	*(short *)(ptr+8) = curproc->mb_writer;
	return 0;
}

/*
 * more mutex: this time a semaphore.
 *
 * long Psemaphore(short mode, long id, long timeout)
 *
 *	MODE	ACTION
 *	  0	Create and get a semaphore with the given ID.
 *	  1	Destroy.
 *	  2	Get (blocks until it's available or destroyed, or timeout).
 *	  3	Release.
 *
 * RETURNS
 *
 *	CODE	MEANING
 *	  0	OK.  Created/obtained/released/destroyed, depending on mode.
 *	ERROR	You asked for a semaphore that you already own.
 *	EBADARG	That semaphore doesn't exist (modes 1, 2, 3, 4),
 *		or out of slots for new semaphores (0).
 *	EACCES	That semaphore exists already, so you can't create it (mode 0),
 *		or the semaphore is busy (returned from mode 3 if you lose),
 *		or you don't own it (modes 1 and 4).
 *
 * If you create a semaphore you will also own it, so you have to release
 * it with mode 2 before anybody else can get it.  Semaphore ID's are magic
 * numbers which the creator should make available to the users. You have
 * to own a semaphore to destroy it.  If you block waiting for a semaphore,
 * and then it gets destroyed, you get EBADARG.
 *
 * The timeout argument is ignored except in mode 2.  In that mode, 0 means
 * "return immediately" and, as a special case, -1 means "forever."
 * Since it is in fact an unsigned long number of milliseconds, -2 means
 * 49 hours, but here -1 truly means forever.
 *
 */

static void _cdecl
unsemame(struct proc *p)
{
	ushort sr;

	/* take process P off the WAIT_Q and put it on the READY_Q */
	TRACE(("semaphore sleep ends for pid %d",p->pid));

	sr = spl7();
	
	if (p->wait_q == WAIT_Q)
	{
		rm_q(WAIT_Q,p);
		add_q(READY_Q,p);
	}
	p->wait_cond = 0;
	
	spl(sr);
}

struct sema
{
	struct sema *next;
	long id;
	short owner; /* -1 means "available" */
};

static struct sema *semalist;

long _cdecl
p_semaphore(int mode, long id, long timeout)
{
	TRACELOW(("Psemaphore(%d,%lx)", mode, id));

	switch (mode)
	{
		case 0:	/* create */
		{
			struct sema *s;

			for (s = semalist; s; s = s->next)
			{
				if (s->id == id)
				{
					DEBUG(("Psemaphore(%d,%lx): already exists", mode, id));
					return EACCES;
				}
			}

			/* get a new one */
			s = kmalloc(sizeof(*s));
			if (!s)
				return ENOMEM;

			s->id = id;
			s->owner = curproc->pid;
			s->next = semalist;
			semalist = s;

			return E_OK;
		}
		case 2:	/* get */
		{
			TIMEOUT *timeout_ptr = NULL;
			struct sema *s;
loop:
			for (s = semalist; s; s = s->next)
			{
				if (s->id == id)
				{
					/* found your semaphore */
					if (s->owner == curproc->pid)
					{
						DEBUG(("Psemaphore(%d,%lx): curproc already owns it!",
							mode, id));
						return EERROR;
					}

					if (s->owner == -1)
					{
						/* it's free; you get it */
						s->owner = curproc->pid;
						if (timeout_ptr)
							canceltimeout(timeout_ptr);
						
						return 0;
					}
					else
					{
						/* not free */
						if (timeout == 0)
						{
							/* non-blocking mode */
							return EACCES;
						}
						else
						{
							if (timeout != -1 && !timeout_ptr)
							{
								/* schedule a timeout */
								timeout_ptr = addtimeout(curproc, timeout, unsemame);
							}

							/* block until it's released, then try again */
							sleep(WAIT_Q, WAIT_SEMA);
							if (curproc->wait_cond != WAIT_SEMA)
							{
								TRACE(("Psemaphore(%d,%lx) timed out",
									mode, id));
								return EACCES;
							}
							
							goto loop;
						}
					}
				}
			}

			/* no such semaphore (possibly deleted while we were waiting) */
			DEBUG(("Psemaphore(%d,%lx): no such semaphore", mode, id));
			
			if (timeout_ptr)
				canceltimeout(timeout_ptr);
			
			return EBADARG;
		}
		case 3:	/* release */
		case 1:	/* destroy */
		{
			struct sema *s, **q;

			/*
			 * Style note: this is a handy way to chain down a linked list.
			 * q follows along behind s pointing at the "next" field of the
			 * previous list element.  To start, q points to the semalist
			 * variable itself.  At all times, then, *q is the place to
			 * write s->next when you want to remove an element 's' from
			 * the list.
			 */
			for (s = *(q = &semalist); s; s = *(q = &s->next))
			{
				if (s->id == id)
				{
					/* found your semaphore */

					if (s->owner != curproc->pid
					    && s->owner != -1)
					{
						DEBUG(("Psemaphore(%d,%lx): access denied, locked by pid %i",
							mode, id, s->owner));
						return EACCES;
					}

					if (mode == 3)
					{
						s->owner = -1;	/* make it free */
					}
					else
					{
						*q = s->next;	/* delete from list */
						kfree(s);	/* and free it */
					}

					/* wake up anybody who's waiting for a semaphore */
					wake(WAIT_Q, WAIT_SEMA);

					return E_OK;
				}
			}

			/* no such semaphore */
			DEBUG(("Psemaphore(%d,%lx): no such semaphore",mode,id));
			return EBADARG;
		}
	}

	DEBUG(("Psemaphore(%d,%lx): invalid mode", mode, id));
	return ENOSYS;
}

/*
 * Release all semaphores owned by the process with process id "pid".
 * The semaphores are only released, not destroyed. This function
 * is called from terminate() in dos_mem.c during process termination.
 */
void
free_semaphores(int pid)
{
	int didsomething = 0;
	struct sema *s;

	for (s = semalist; s; s = s->next)
	{
		if (s->owner == pid)
		{
			s->owner = -1; /* mark the semaphore as free */
			didsomething = 1;
		}
	}

	if (didsomething)
	{
		/* wake up anybody waiting for a semaphore, in case it's one of
		 * the ones we just freed
		 */
		wake(WAIT_Q, WAIT_SEMA);
	}
}
