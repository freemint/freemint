/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* routines for handling processes */

# include "proc.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/signal.h"

# include "arch/context.h"	/* save_context, change_context */
# include "arch/kernel.h"
# include "arch/mprot.h"

# include "bios.h"
# include "dosfile.h"
# include "dosmem.h"
# include "fasttext.h"
# include "filesys.h"
# include "kmemory.h"
# include "memory.h"
# include "random.h"
# include "signal.h"
# include "time.h"
# include "timeout.h"
# include "random.h"
# include "util.h"
# include "xbios.h"

# include <osbind.h>


static void swap_in_curproc	(void);
static void do_wakeup_things	(short sr, int newslice, long cond);
INLINE void do_wake		(int que, long cond);
INLINE ulong gen_average	(ulong *sum, uchar *load_ptr, ulong max_size);


/*
 * We initialize proc_clock to a very large value so that we don't have
 * to worry about unexpected process switches while starting up
 */
ushort proc_clock = 0x7fff;


/* global process variables */
PROC *proclist = NULL;		/* list of all active processes */
PROC *curproc  = NULL;		/* current process		*/
PROC *rootproc = NULL;		/* pid 0 -- MiNT itself		*/
PROC *sys_q[NUM_QUEUES] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* default; actual value comes from mint.cnf */
short time_slice = 2;

# define TIME_SLICE	time_slice

/* macro for calculating number of missed time slices, based on a
 * process' priority
 */
# define SLICES(pri)	(((pri) >= 0) ? 0 : -(pri))

/*
 * get a new process struct
 */

PROC *
new_proc (void)
{
	PROC *p;
	void *pt;
	
# ifdef MMU040
	extern int page_ram_type;	/* in mprot040.c */
	
	pt = 0L;
	if (page_ram_type & 2)
		pt = get_region (alt, page_table_size + 512, PROT_S);
	if (!pt && (page_ram_type & 1))
		pt = get_region (core, page_table_size + 512, PROT_S);
# else
	pt = kmalloc (page_table_size + 16);
# endif
	if (!pt)
		return NULL;
	
	p = kmalloc (sizeof (*p));
	if (!p)
	{
# ifndef MMU040
		kfree (pt);
# else
		((MEMREGION *) pt)->links--;
		if (!((MEMREGION *) pt)->links)
			free_region (pt);
# endif
		return 0;
	}
	
	/* zero out the new allocated memory */
	bzero (p, sizeof (*p));
	
	/* set the stack barrier */
	p->stack_magic = STACK_MAGIC;
	
# ifndef MMU040
	/* page tables must be on 16 byte boundaries, so we
	 * round off by 16 for that; however, we will want to
	 * kfree that memory at some point, so we squirrel
	 * away the original address for later use
	 */
	p->page_table = ROUND16 (pt);
# else
	/* For the 040, the page tables must be on 512 byte boundaries */
	p->page_table = ROUND512 (((MEMREGION *)pt)->loc);
# endif
	p->pt_mem = pt;
	
	DEBUG (("Successfully generated proc %lx", (long)p));
	return p;
}

/*
 * dispose of an old proc
 */

void
dispose_proc (PROC *p)
{
	TRACELOW (("dispose_proc"));
	
# ifndef MMU040
	kfree (p->pt_mem);
# else
	((MEMREGION *) p->pt_mem)->links--;
	if (!((MEMREGION *) p->pt_mem)->links)
		free_region (p->pt_mem);
# endif
	
	kfree (p);
}

/*
 * create a new process that is (practically) a duplicate of the
 * current one
 */

PROC *
fork_proc (long *err)
{
	PROC *p;
	char *root_dir = NULL;
	long i;
	
	p = new_proc ();
	if (!p)
	{
nomem:
		DEBUG (("fork_proc: insufficient memory"));
		
		if (err) *err = ENOMEM;
		return NULL;
	}
	
	if (curproc->root_dir)
	{
		root_dir = kmalloc (strlen (curproc->root_dir) + 1);
		if (!root_dir)
		{
			kfree (p);
			goto nomem;
		}
	}
	
	/* child shares most things with parent,
	 * but hold on to page table ptr
	 */
	 {
# ifndef MMU040
		long_desc *pthold;
# else
		ulong *pthold;
# endif
		void *ptmemhold;
		
		pthold = p->page_table;
		ptmemhold = p->pt_mem;
		
		*p = *curproc;
		
		p->page_table = pthold;
		p->pt_mem = ptmemhold;
	}
	
	/* these things are not inherited
	 */
	p->ppid = curproc->pid;
	p->pid = newpid ();
	p->sigpending = 0;
	p->nsigs = 0;
	p->sysstack = (long) (p->stack + STKSIZE - 12);
	p->ctxt[CURRENT].ssp = p->sysstack;
	p->ctxt[SYSCALL].ssp = (long) (p->stack + ISTKSIZE);
	p->stack_magic = STACK_MAGIC;
	p->alarmtim = 0;
	p->curpri = p->pri;
	p->slices = SLICES (p->pri);
	
	p->itimer[0].interval = 0;
	p->itimer[0].reqtime = 0;
	p->itimer[0].timeout = 0;
	p->itimer[1].interval = 0;
	p->itimer[1].reqtime = 0;
	p->itimer[1].timeout = 0;
	p->itimer[2].interval = 0;
	p->itimer[2].reqtime = 0;
	p->itimer[2].timeout = 0;
	
	((long *) p->sysstack)[1] = FRAME_MAGIC;
	((long *) p->sysstack)[2] = 0;
	((long *) p->sysstack)[3] = 0;
	
	p->usrtime = p->systime = p->chldstime = p->chldutime = 0;
	
	/* allocate space for memory regions: do it here so that we can fail
	 * before we duplicate anything else. The memory regions are
	 * actually copied later
	 */
	p->mem = kmalloc (p->num_reg * sizeof (MEMREGION *));
	if (!p->mem)
	{
		dispose_proc (p);
		goto nomem;
	}
	
	p->addr = kmalloc (p->num_reg * sizeof (virtaddr));
	if (!p->addr)
	{
		kfree (p->mem);
		dispose_proc (p);
		goto nomem;
	}
	
	/* Duplicate command line */
	if (p->real_cmdline != NULL
		&& (p->real_cmdline [0] != 0 || p->real_cmdline [1] != 0
			|| p->real_cmdline [2] != 0 || p->real_cmdline [3] != 0))
	{
		ulong *parent_cmdline = (ulong *) p->real_cmdline;
		
		p->real_cmdline = kmalloc ((*parent_cmdline) + 4);
		if (p->real_cmdline) 
		{
			memcpy (p->real_cmdline, parent_cmdline, (*parent_cmdline) + 4);
		}
		else
		{
			kfree (p->mem);
			kfree (p->addr);
			dispose_proc (p);
			
			goto nomem;
		}
	}
	else if (p->ppid != 0)
	{
		if (p->fname != NULL)
		{
			ALERT ("Oops: no command line for %s (pid %d)", p->fname, p->pid);
		}
		else if (p->name != NULL)
		{
			ALERT ("Oops: no command line for %s (pid %d)", p->name, p->pid);
		}
		else
		{
			ALERT ("Oops: no command line for pid %d (ppid %d)", p->pid, p->ppid);
		}
		
		p->real_cmdline = NULL;
	}
	
	/* copy open handles */
	for (i = MIN_HANDLE; i < MAX_OPEN; i++)
	{
		FILEPTR *f;
		
		if ((f = p->handle[i]) != 0)
		{
			if ((f == (FILEPTR *) 1L) || (f->flags & O_NOINHERIT))
			{
				/* oops, we didn't really want to copy this
				 * handle
				 */
				p->handle[i] = 0;
			}
			else
				f->links++;
		}
	}
	
	/* copy root and current directories */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		dup_cookie (&p->root[i], &curproc->root[i]);
		dup_cookie (&p->curdir[i], &curproc->curdir[i]);
	}
	
	if (root_dir)
	{
		p->root_dir = root_dir;
		strcpy (p->root_dir, curproc->root_dir);
		dup_cookie (&p->root_fc, &curproc->root_fc);
	}
	
	/* Duplicate cookie for the executable file */
	dup_cookie (&p->exe, &curproc->exe);
	
	/* clear directory search info */
	bzero (p->srchdta, NUM_SEARCH * sizeof (DTABUF *));
	bzero (p->srchdir, sizeof (p->srchdir));
	p->searches = 0;
	
	/* copy memory */
	for (i = 0; i < curproc->num_reg; i++)
	{
		p->mem[i] = curproc->mem[i];
		if (p->mem[i] != 0)
			p->mem[i]->links++;
		
		p->addr[i] = curproc->addr[i];
	}
	
	/* now that memory ownership is copied, fill in page table */
	init_page_table (p);
	
	/* child isn't traced */
	p->ptracer = 0;
	p->ptraceflags = 0;
	
	p->started = xtime;
	
	p->q_next = 0;
	p->wait_q = 0;
	
	/* hook into the process list */
	p->gl_next = proclist;
	proclist = p;
	
	return p;
}

/*
 * initialize the process table
 */

void
init_proc (void)
{
	static DTABUF dta;
	
	
	rootproc = curproc = new_proc ();
	assert (curproc);
	
	curproc->ppid = -1;		/* no parent */
	curproc->domain = DOM_TOS;	/* TOS domain */
	curproc->sysstack = (long) (curproc->stack + STKSIZE - 12);
	curproc->magic = CTXT_MAGIC;
	curproc->memflags = F_PROT_S;	/* default prot mode: super-only */
	
	((long *) curproc->sysstack)[1] = FRAME_MAGIC;
	((long *) curproc->sysstack)[2] = 0;
	((long *) curproc->sysstack)[3] = 0;
	
	curproc->dta = &dta;		/* looks ugly */
	curproc->base = _base;
	strcpy (curproc->name, "MiNT");
	
	/* get some memory */
	curproc->num_reg = NUM_REGIONS;
	curproc->mem = kmalloc (curproc->num_reg * sizeof (MEMREGION *));
	curproc->addr = kmalloc (curproc->num_reg * sizeof (virtaddr));
	
	/* make sure kmalloc was successful */
	assert (curproc->mem && curproc->addr);
	
	/* make sure it's filled with zeros */
	bzero (curproc->mem, curproc->num_reg * sizeof (MEMREGION *));
	bzero (curproc->addr, curproc->num_reg * sizeof (virtaddr));
	
	/* get root and current directories for all drives */
	{
		FILESYS *fs;
		int i;
		
		for (i = 0; i < NUM_DRIVES; i++)
		{
			fcookie dir;
			
			fs = drives [i];
			if (fs && xfs_root (fs, i, &dir) == E_OK)
			{
				curproc->root[i] = dir;
				dup_cookie (&curproc->curdir[i], &dir);
			}
			else
			{
				curproc->root[i].fs = curproc->curdir[i].fs = 0;
				curproc->root[i].dev = curproc->curdir[i].dev = i;
			}
		}
	}
	
	init_page_table (curproc);
	
	/* Set the correct drive. The current directory we
	 * set later, after all file systems have been loaded.
	 */
	curproc->curdrv = Dgetdrv ();
	proclist = curproc;
	
	curproc->umask = 0;
	
	/* some more protection against job control; unless these signals are
	 * re-activated by a shell that knows about job control, they'll have
	 * no effect
	 */
	curproc->sighandle[SIGTTIN] =
	curproc->sighandle[SIGTTOU] =
	curproc->sighandle[SIGTSTP] = SIG_IGN;
	
	/* set up some more per-process variables */
	curproc->started = xtime;
	
	if (has_bconmap)
		/* init_xbios not happened yet */
		curproc->bconmap = (int) Bconmap (-1);
	else
		curproc->bconmap = 1;
	
	curproc->logbase = (void *) Logbase();
	curproc->criticerr = *((long _cdecl (**)(long)) 0x404L);
}

/* reset_priorities():
 * 
 * reset all process priorities to their base level
 * called once per second, so that cpu hogs can get _some_ time
 * slices :-).
 */
void
reset_priorities (void)
{
	PROC *p;
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->slices >= 0)
		{
			p->curpri = p->pri;
			p->slices = SLICES (p->curpri);
		}
	}
}

/* run_next(p, slices):
 * 
 * schedule process "p" to run next, with "slices" initial time slices;
 * "p" does not actually start running until the next context switch
 */
void
run_next (PROC *p, int slices)
{
	register ushort sr;
	
	sr = spl7 ();
	
	p->slices = -slices;
	p->curpri = MAX_NICE;
	p->wait_q = READY_Q;
	p->q_next = sys_q[READY_Q];
	sys_q[READY_Q] = p;
	
	spl (sr);
}

/* fresh_slices(slices):
 * 
 * give the current process "slices" more slices in which to run
 */
void
fresh_slices (int slices)
{
	reset_priorities ();
	
	curproc->slices = 0;
	curproc->curpri = MAX_NICE + 1;
	proc_clock = TIME_SLICE + slices;
}

/*
 * add a process to a wait (or ready) queue.
 *
 * processes go onto a queue in first in-first out order
 */

void
add_q (int que, PROC *proc)
{
	PROC *q, **lastq;
	
	/* "proc" should not already be on a list */
	assert (proc->wait_q == 0);
	assert (proc->q_next == 0);
	
	lastq = &sys_q[que];
	q = *lastq;
	while (q)
	{
		lastq = &q->q_next;
		q = *lastq;
	}
	*lastq = proc;
	
	proc->wait_q = que;
	if (que != READY_Q && proc->slices >= 0)
	{
		proc->curpri = proc->pri;	/* reward the process */
		proc->slices = SLICES (proc->curpri);
	}
}

/*
 * remove a process from a queue
 */

void
rm_q (int que, PROC *proc)
{
	PROC *q;
	PROC *old = 0;
	
	assert (proc->wait_q == que);
	
	q = sys_q[que];
	while (q && q != proc)
	{
		old = q;
		q = q->q_next;
	}
	
	if (q == 0)
		FATAL ("rm_q: unable to remove process from queue");
	
	if (old)
		old->q_next = proc->q_next;
	else
		sys_q[que] = proc->q_next;
	
	proc->wait_q = 0;
	proc->q_next = 0;
}

/*
 * preempt(): called by the vbl routine and/or the trap handlers when
 * they detect that a process has exceeded its time slice and hasn't
 * yielded gracefully. For now, it just does sleep(READY_Q); later,
 * we might want to keep track of statistics or something.
 */

void _cdecl
preempt (void)
{
	if (bconbsiz)
	{
		(void) bflush ();
	}
	else
	{
		/* punish the pre-empted process */
		if (curproc->curpri >= MIN_NICE)
			curproc->curpri -= 1;
	}
	
	sleep (READY_Q, curproc->wait_cond);
}

/*
 * swap_in_curproc(): for all memory regions of the current process swaps
 * in the contents of those regions that have been saved in a shadow region
 */

static void
swap_in_curproc (void)
{
	long txtsize = curproc->txtsize;
	MEMREGION *m, *shdw, *save;
	int i;
	
	for (i = 0; i < curproc->num_reg; i++)
	{
		m = curproc->mem[i];
		if (m && m->save)
		{
			save = m->save;
			for (shdw = m->shadow; shdw->save; shdw = shdw->shadow)
				assert (shdw != m);
			
			assert (m->loc == shdw->loc);
			
			shdw->save = save;
			m->save = 0;
			if (i != 1 || txtsize == 0)
			{
				quickswap ((char *) m->loc, (char *) save->loc, m->len);
			}
			else
			{
				quickswap ((char *) m->loc, (char *) save->loc, 256);
				quickswap ((char *) m->loc + (txtsize+256), (char *) save->loc + 256, m->len - (txtsize+256));
			}
		}
	}
}

/*
 * sleep(que, cond): put the current process on the given queue, then switch
 * contexts. Before a new process runs, give it a fresh time slice. "cond"
 * is the condition for which the process is waiting, and is placed in
 * curproc->wait_cond
 */

INLINE void
do_wakeup_things (short sr, int newslice, long cond)
{
	/*
	 * check for stack underflow, just in case
	 */
	auto int foo;
	PROC *p;
	
	p = curproc;
	
	if ((sr & 0x700) < 0x500)
	{
		/* skip all this if int level is too high */
		if (p->pid && ((long) &foo) < (long) p->stack + ISTKSIZE + 512)
		{
			ALERT ("stack underflow");
			handle_sig (SIGBUS);
		}
		
		/* see if process' time limit has been exceeded */
		if (p->maxcpu)
		{
			if (p->maxcpu <= p->systime + p->usrtime)
			{
				DEBUG (("cpu limit exceeded"));
				raise (SIGXCPU);
			}
		}
		
		/* check for alarms and similar time out stuff */
		checkalarms ();
		
		if (p->sigpending && cond != (long) p_waitpid)
			/* check for signals */
			check_sigs ();
	}
	
	if (newslice)
	{
		if (p->slices >= 0)
		{
			/* get a fresh time slice */
			proc_clock = TIME_SLICE;
		}
		else
		{
			/* slices set by run_next */
			proc_clock = TIME_SLICE - p->slices;
			p->curpri = p->pri;
		}
		
		p->slices = SLICES (p->curpri);
	}
}

static long sleepcond, iwakecond;

/*
 * sleep: returns 1 if no signals have happened since our last sleep, 0
 * if some have
 */

int _cdecl 
sleep (int _que, long cond)
{
	PROC *p;
	ushort sr;
	short que = _que & 0xff;
	ulong onsigs = curproc->nsigs;
	int newslice = 1;
	
	/* save condition, checkbttys may just wake() it right away ...
	 * note this assumes the condition will never be waked from interrupts
	 * or other than thru wake() before we really went to sleep, otherwise
	 * use the 0x100 bit like select
	 */
	sleepcond = cond;
	
	/* if there have been keyboard interrupts since our last sleep,
	 * check for special keys like CTRL-ALT-Fx
	 */
	sr = spl7 ();
	if ((sr & 0x700) < 0x500)
	{
		/* can't call checkkeys if sleep was called
		 * with interrupts off  -nox
		 */
		spl (sr);
		(void) checkbttys ();
		if (kintr)
		{
			(void) checkkeys ();
			kintr = 0;
		}
		
# ifdef DEV_RANDOM		
		/* Wake processes waiting for random bytes */
		checkrandom ();
# endif
		
		sr = spl7 ();
		if ((curproc->sigpending & ~(curproc->sigmask))
			&& curproc->pid && que != ZOMBIE_Q && que != TSR_Q)
		{
			spl (sr);
			check_sigs ();
			sleepcond = 0;	/* possibly handled a signal, return */
			sr = spl7 ();
		}
	}
	
	/* kay: If _que & 0x100 != 0 then take curproc->wait_cond != cond as
	 * an indicatation that the wakeup has already happend before we
	 * actually go to sleep and return immediatly.
	 */
	if ((que == READY_Q && !sys_q[READY_Q])
		|| ((sleepcond != cond || (iwakecond == cond && cond) || (_que & 0x100 && curproc->wait_cond != cond))
			&& (!sys_q[READY_Q] || (newslice = 0, proc_clock))))
	{
		/* we're just going to wake up again right away! */
		iwakecond = 0;
		spl (sr);
		do_wakeup_things (sr, newslice, cond);
		
		return (onsigs != curproc->nsigs);
	}
	
	/* unless our time slice has expired (proc_clock == 0) and other
	 * processes are ready...
	 */
	iwakecond = 0;
	if (!newslice)
		que = READY_Q;
	else
		curproc->wait_cond = cond;
	
	add_q (que, curproc);
	
	/* alright curproc is on que now... maybe there's an
	 * interrupt pending that will wakeselect or signal someone
	 */
	spl (sr);
	
	if (!sys_q[READY_Q])
	{
		/* hmm, no-one is ready to run. might be a deadlock, might not.
		 * first, try waking up any napping processes;
		 * if that doesn't work, run the root process,
		 * just so we have someone to charge time to.
		 */
		wake (SELECT_Q, (long) nap);
		
		sr = spl7 ();
		if (!sys_q[READY_Q])
		{
			p = rootproc;		/* pid 0 */
			rm_q (p->wait_q, p);
			add_q (READY_Q, p);
		}
		spl (sr);
	}
	
	/*
	 * Walk through the ready list, to find what process should run next.
	 * Lower priority processes don't get to run every time through this
	 * loop; if "p->slices" is positive, it's the number of times that
	 * they will have to miss a turn before getting to run again
	 *
	 * Loop structure:
	 *	while (we haven't picked anybody)
	 *	{
	 *		for (each process)
	 *		{
	 *			if (sleeping off a penalty)
	 *			{
	 *				decrement penalty counter
	 *			}
	 *			else
	 *			{
	 *				pick this one and break out of
	 *				both loops
	 *			}
	 *		}
	 *	}
	 */
	sr = spl7 ();
	p = 0;
	while (!p)
	{
		for (p = sys_q[READY_Q]; p; p = p->q_next)
		{
			if (p->slices > 0)
				p->slices--;
			else
				break;
		}
	}
	/* p is our victim */
	rm_q (READY_Q, p);
	spl (sr);
	
	if (save_context(&(curproc->ctxt[CURRENT])))
	{
		/*
		 * restore per-process variables here
		 */
# ifndef MULTITOS
# ifdef FASTTEXT
		if (!hardscroll)
			*((void **) 0x44eL) = curproc->logbase;
# endif
# endif
		swap_in_curproc ();
		do_wakeup_things (sr, 1, cond);
		
		return (onsigs != curproc->nsigs);
	}
	
	/*
	 * save per-process variables here
	 */
# ifndef MULTITOS
# ifdef FASTTEXT
	if (!hardscroll)
		curproc->logbase = *((void **) 0x44eL);
# endif
# endif
	
	curproc->ctxt[CURRENT].regs[0] = 1;
	curproc = p;
	
	proc_clock = TIME_SLICE;			/* fresh time */
	
	if ((p->ctxt[CURRENT].sr & 0x2000) == 0)	/* user mode? */
		leave_kernel ();
	
	assert (p->magic == CTXT_MAGIC);
	change_context (&(p->ctxt[CURRENT]));
	
	/* not reached */
	return 0;
}

/*
 * wake(que, cond): wake up all processes on the given queue that are waiting
 * for the indicated condition
 */

INLINE void
do_wake (int que, long cond)
{
	PROC *p;
top:
	for (p = sys_q[que]; p; )
	{
		PROC *q;
		register short s;
		
		s = spl7 ();
		
		/* check p is still on the right queue,
		 * maybe an interrupt just woke it...
		 */
		if (p->wait_q != que)
		{
			spl (s);
			goto top;
		}
		
		q = p;
		p = p->q_next;
		if (q->wait_cond == cond)
		{
			rm_q (que, q);
			add_q (READY_Q, q);
		}
		
		spl (s);
	}
}

void _cdecl 
wake (int que, long cond)
{
	if (que == READY_Q)
	{
		ALERT ("wake: why wake up ready processes??");
		return;
	}
	
	if (sleepcond == cond)
		sleepcond = 0;
	
	do_wake (que, cond);
}

/*
 * iwake(que, cond, pid): special version of wake() for IO interrupt
 * handlers and such.  the normal wake() would lose when its
 * interrupt goes off just before a process is calling sleep() on the
 * same condition (similar problem like with wakeselect...)
 *
 * use like this:
 *	static ipid = -1;
 *	static volatile sleepers = 0;	(optional, to save useless calls)
 *	...
 *	device_read(...)
 *	{
 *		ipid = curproc->pid;	(p_getpid() for device drivers...)
 *		while (++sleepers, (not ready for IO...)) {
 *			sleep(IO_Q, cond);
 *			if (--sleepers < 0)
 *				sleepers = 0;
 *		}
 *		if (--sleepers < 0)
 *			sleepers = 0;
 *		ipid = -1;
 *		...
 *	}
 *
 * and in the interrupt handler:
 *	if (sleepers > 0)
 *	{
 *		sleepers = 0;
 *		iwake (IO_Q, cond, ipid);
 *	}
 *
 * caller is responsible for not trying to wake READY_Q or other nonsense :)
 * and making sure the passed pid is always -1 when curproc is calling
 * sleep() for another than the waked que/condition.
 */

void _cdecl 
iwake (int que, long cond, short pid)
{
	if (pid >= 0)
	{
		register ushort s;
		
		s = spl7 ();
		
		if (iwakecond == cond)
		{
			spl (s);
			return;
		}
		
		if (curproc->pid == pid && !curproc->wait_q)
			iwakecond = cond;
		
		spl (s);
	}
	
	do_wake (que, cond);
}

/*
 * wakeselect(p): wake process p from a select() system call
 * may be called by an interrupt handler or whatever
 */

void _cdecl 
wakeselect (PROC *p)
{
	short s;
	
	s = spl7 ();
	
	if (p->wait_cond == (long) wakeselect
		|| p->wait_cond == (long) &select_coll)
	{
		p->wait_cond = 0;
	}
	
	if (p->wait_q == SELECT_Q)
	{
		rm_q (SELECT_Q, p);
		add_q (READY_Q, p);
	}
	
	spl (s);
}

/*
 * dump out information about processes
 */

/*
 * kludge alert! In order to get the right pid printed by FORCE, we use
 * curproc as the loop variable.
 *
 * I have changed this function so it is more useful to a user, less to
 * somebody debugging MiNT.  I haven't had any stack problems in MiNT
 * at all, so I consider all that stack info wasted space.  -- AKP
 */

# ifdef DEBUG_INFO
static const char *qstring[] =
{
	"run", "ready", "wait", "iowait", "zombie", "tsr", "stop", "select"
};

/* UNSAFE macro for qname, evaluates x 1, 2, or 3 times */
# define qname(x) ((x >= 0 && x < NUM_QUEUES) ? qstring[x] : "unkn")
# endif

ulong uptime = 0;
ulong avenrun[3] = { 0, 0, 0 };
ushort uptimetick = 200;	

static ushort number_running;

void
DUMPPROC (void)
{
# ifdef DEBUG_INFO
	PROC *p = curproc;

	FORCE ("Uptime: %ld seconds Loads: %ld %ld %ld Processes running: %d",
		uptime,
		(avenrun[0] * 100) / 2048 , (avenrun[1] * 100) / 2048, (avenrun[2] * 100 / 2048),
 		number_running);

	for (curproc = proclist; curproc; curproc = curproc->gl_next)
	{
		FORCE ("state %s PC: %lx BP: %lx",
			qname(curproc->wait_q),
			curproc->ctxt[SYSCALL].pc,
			curproc->base);
	}
	curproc = p;	/* restore the real curproc */
# endif
}

INLINE ulong
gen_average (ulong *sum, uchar *load_ptr, ulong max_size)
{
	register long old_load = (long) *load_ptr;
	register long new_load = number_running;
	
	*load_ptr = (uchar) new_load;
	
	*sum += (new_load - old_load) * LOAD_SCALE;
	
	return (*sum / max_size);
}

void
calc_load_average (void)
{
	static uchar one_min [SAMPS_PER_MIN];
	static uchar five_min [SAMPS_PER_5MIN];
	static uchar fifteen_min [SAMPS_PER_15MIN];
	
	static ushort one_min_ptr = 0;
	static ushort five_min_ptr = 0;
	static ushort fifteen_min_ptr = 0;
	
	static ulong sum1 = 0;
	static ulong sum5 = 0;
	static ulong sum15 = 0;
	
	register PROC *p;
	
# if 0	/* moved to intr.spp */
	uptime++;
	uptimetick += 200;
	
	if (uptime % 5) return;
# endif
	
	number_running = 0;
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p != rootproc)
		{
			if ((p->wait_q == CURPROC_Q) || (p->wait_q == READY_Q))
				number_running++;
		}
		
		/* Check the stack magic here, to ensure the system/interrupt
		 * stack hasn't grown too much. Most noticeably, NVDI 5's new
		 * bitmap conversion (vr_transfer_bits()) seems to eat _a lot_
		 * of supervisor stack, that's why the values in proc.h have
		 * been increased.
		 */
		if (p->stack_magic != STACK_MAGIC)
			FATAL ("proc %lx has invalid stack_magic %lx", (long) p, p->stack_magic);
	}
	
	if (one_min_ptr == SAMPS_PER_MIN)
		one_min_ptr = 0;
	
	avenrun [0] = gen_average (&sum1, &one_min [one_min_ptr++], SAMPS_PER_MIN);
	
	if (five_min_ptr == SAMPS_PER_5MIN)
		five_min_ptr = 0;
	
	avenrun [1] = gen_average (&sum5, &five_min [five_min_ptr++], SAMPS_PER_5MIN);
	
	if (fifteen_min_ptr == SAMPS_PER_15MIN)
		fifteen_min_ptr = 0;
	
	avenrun [2] = gen_average (&sum15, &fifteen_min [fifteen_min_ptr++], SAMPS_PER_15MIN);
}
