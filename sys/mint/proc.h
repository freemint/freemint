/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/*
 * Description: defines for various process related things
 */

# ifndef _mint_proc_h
# define _mint_proc_h

# include "basepage.h"
# include "config.h"
# include "ktypes.h"
# include "mem.h"
# include "file.h"
# include "signal.h"


/*
 * a process context consists, for now, of its registers
 */

struct context
{
	long	regs[15];	/* registers d0-d7, a0-a6 */
	long	usp;		/* user stack pointer (a7) */
	short	sr;		/* status register */
	long	pc;		/* program counter */
	long	ssp;		/* supervisor stack pointer */
	long	term_vec;	/* GEMDOS terminate vector (0x102) */
/*
 * AGK: if running on a TT and the user is playing with the FPU then we
 * must save and restore the context. We should also consider this for
 * I/O based co-processors, although this may be difficult due to
 * possibility of a context switch in the middle of an I/O handshaking
 * exchange.
 */
	uchar	fstate[216];	/* FPU internal state */
	long	fregs[3*8];	/* registers fp0-fp7 */
	long	fctrl[3];	/* FPCR/FPSR/FPIAR */
	char	ptrace;		/* trace exception is pending */
	char	pad1;		/* junk */
	long	iar;		/* unused */
	long	res[2];		/* unused, reserved */
/*
 * Saved CRP and TC values. These are necessary for memory protection.
 */
	crp_reg	crp;		/* 64 bits */
	tc_reg	tc;		/* 32 bits */
/*
 * AGK: for long (time-wise) co-processor instructions (FMUL etc.), the
 * FPU returns NULL, come-again with interrupts allowed primitives. It
 * is highly likely that a context switch will occur in one of these if
 * running a mathematically intensive application, hence we must handle
 * the mid-instruction interrupt stack. We do this by saving the extra
 * 3 long words and the stack format word here.
 */
	ushort	sfmt;		/* stack frame format identifier */
	short	internal[42];	/* internal state -- see framesizes[] for size */
/*
 * jw: The following 3 longs are used to save the content of a register in
 * syscall handler.This is needed to get a reentrant workaround for bugs 
 * (relying on unmodified registers a0 or a1) in MiNT lib < 0.51 :-<
 * The affected functions are Pvfork(), Psigblock() and Psigsetmask()
 */
	long	wa_vfork;	/* space to save a1 over Pvfork() */
	long	wa_sigblock;	/* space to save a0 over Psigblock() */
	long	wa_sigsetmask;	/* space to save a0 over Psigsetmask() */
};

# define PROC_CTXTS	2
# define SYSCALL	0	/* saved context from system call */
# define CURRENT	1	/* current saved context */


/*
 * Timeout events are stored in a list; the "when" field in the event
 * specifies the number of milliseconds *after* the last entry in the
 * list that the timeout should occur, so routines that manipulate
 * the list only need to check the first entry.
 */

typedef void _cdecl to_func (PROC *);

struct timeout
{
	TIMEOUT	*next;
	PROC	*proc;
	long	when;
	to_func	*func;	/* function to call at timeout */
	short	flags;
	long	arg;
};

struct itimervalue
{
	TIMEOUT	*timeout;
	long	interval;
	long	reqtime;
	long	startsystime;
	long	startusrtime;
};


# define NUM_REGIONS	64	/* number of memory regions alloced at a time */
# define MIN_HANDLE	(-5)	/* minimum handle number		*/
# define MIN_OPEN	6	/* 0..MIN_OPEN-1 are reserved for system */
# define MAX_OPEN	32	/* max. number of open files for a proc	*/
# define SSTKSIZE	8192	/* size of supervisor stack (in bytes) 	*/
# define ISTKSIZE	4096	/* size of interrupt stack (in bytes)	*/
# define STKSIZE	(ISTKSIZE + SSTKSIZE)

# define STACK_MAGIC	0xfedcba98UL
				/* magic for the stack barrier */
# define FRAME_MAGIC	0xf4a3e000UL
				/* magic for signal call stack */
# define CTXT_MAGIC	0xabcdef98UL
# define CTXT2_MAGIC	0x87654321UL
				/* magic #'s for contexts */

# define PNAMSIZ	8	/* no. of characters in a process name */

# define DOM_TOS	0	/* TOS process domain */
# define DOM_MINT	1	/* MiNT process domain */

struct proc
{
	long	sysstack;		/* must be first		*/
	CONTEXT	ctxt[PROC_CTXTS];	/* must be second		*/
	
	/*
	 * this is stuff that the public can know about
	 */
	long	magic;			/* validation for proc struct	*/
	
	BASEPAGE *base;			/* process base page		*/
	short	pid, ppid, pgrp;
	short	ruid;			/* process real user id 	*/
	short	rgid;			/* process real group id 	*/
	short	euid;			/* effective user id		*/
	short	egid;			/* effective group id		*/
	
	ushort	memflags;		/* e.g. malloc from alt ram	*/
	short	pri;			/* base process priority 	*/
	short	wait_q;			/* current process queue	*/

	/* note: wait_cond should be unique for each kind of condition
	 * we might want to wait for. Put a define below, or use an
	 * address in the kernel as the wait condition to ensure
	 * uniqueness.
	 */
	long	wait_cond;		/* condition we're waiting on	*/
					/* (also return code from wait) */
	
# define WAIT_MB	0x3a140001L	/* wait_cond for p_msg call	*/
# define WAIT_SEMA	0x3a140003L	/* wait_cond for p_semaphore	*/
	
	/* all times are in milliseconds
	 * usrtime must always follow systime
	 */
	ulong	systime;		/* time spent in kernel		*/
	ulong	usrtime;		/* time spent out of kernel	*/
	ulong	chldstime;		/* children's kernel time 	*/
	ulong	chldutime;		/* children's user time		*/
	
	ulong	maxmem;			/* max. amount of memory to use */
	ulong	maxdata;		/* max. data region for process */
	ulong	maxcore;		/* max. core memory for process */
	ulong	maxcpu;			/* max. cpu time to use 	*/
	
	short	domain;			/* process domain (TOS or UNIX)	*/
	
	short	curpri;			/* current process priority	*/
	short	suid, sgid;		/* saved user and group ids	*/
# define MIN_NICE -20
# define MAX_NICE  20
	
/* EVERYTHING BELOW THIS LINE IS SUBJECT TO CHANGE:
 * programs should *not* try to read this stuff via the U:\PROC dir.
 */
	/* jr: two fields to hold information passed to Pexec */
	char	fname[PATH_MAX];	/* name of binary		*/
	char	cmdlin[128];		/* original command line	*/
	
	char	name[PNAMSIZ+1];	/* process name			*/
	TIMEOUT	*alarmtim;		/* alarm() event		*/
	short	slices;			/* number of time slices before	*
					 * this process gets to run	*
					 * again			*/
	
	short	bconmap;		/* Bconmap mapping		*/
	FILEPTR *midiout;		/* MIDI output			*/
	FILEPTR *midiin;		/* MIDI input			*/
	FILEPTR	*prn;			/* printer			*/
	FILEPTR *aux;			/* auxiliary tty		*/
	FILEPTR	*control;		/* control tty			*/
	FILEPTR	*handle[MAX_OPEN];	/* file handles			*/
	
	uchar	fdflags[MAX_OPEN];	/* file descriptor flags	*/
	
	ushort	num_reg;		/* number of allocated memory regions */
	MEMREGION **mem;		/* allocated memory regions	*/
	virtaddr *addr;			/* addresses of regions		*/
	
	ulong	sigpending;		/* pending signals		*/
	ulong	sigmask;		/* signals that are masked	*/
	ulong	sighandle[NSIG];	/* signal handlers		*/
	ushort	sigflags[NSIG];		/* signal flags			*/
	ulong	sigextra[NSIG];		/* additional signals to be	*
					 * masked on delivery		*/
	ulong	nsigs;			/* number of signals delivered 	*/
	char	*mb_ptr;		/* p_msg buffer ptr		*/
	long	mb_long1, mb_long2;	/* p_msg storage		*/
	long	mb_mbid;		/* p_msg id being waited for	*/
	short	mb_mode;		/* p_msg mode being waiting in	*/
	short	mb_writer;		/* p_msg pid of writer of msg	*/
	
	short	curdrv;			/* current drive		*/
	ushort	umask;			/* file creation mask		*/
	fcookie root[NUM_DRIVES];	/* root directories		*/
	fcookie	curdir[NUM_DRIVES];	/* current directory		*/
	
	char	*root_dir;		/* for chroot emulation		*/
	fcookie	root_fc;		/* same				*/
	
	long	usrdata;		/* user-supplied data		*/
	
	DTABUF	*dta;			/* current DTA			*/
# define NUM_SEARCH	10		/* max. number of searches	*/
	DTABUF *srchdta[NUM_SEARCH];	/* for Fsfirst/next		*/
	DIR	srchdir[NUM_SEARCH];	/* for Fsfirst/next		*/
	long	srchtim[NUM_SEARCH];	/* for Fsfirst/next		*/
	
	DIR	*searches;		/* open directory searches	*/
	
	long	txtsize;		/* size of text region		*
					 * (for fork())			*/
	
	long	_cdecl (*criticerr)(long);
					/* critical error handler	*/
	void	*logbase;		/* logical screen base		*/
	
	PROC	*ptracer;		/* process which is tracing this one */
	short	ptraceflags;		/* flags for process tracing	*/
	
	short	in_dos;			/* flag: 1 = process is executing a GEMDOS call */
	
	void	*page_table;		/* rounded page table pointer	*/
	void	*pt_mem;		/* original kmalloc'd block for	*
					 * above			*/
	
	ulong	exception_pc;		/* pc at time of bombs		*/
	ulong	exception_ssp;		/* ssp at time of bomb (e.g. bus error)	*/
	ulong	exception_tbl;		/* table in use at exception time */
	ulong	exception_addr;		/* access address from stack	*/
	ushort	exception_mmusr;	/* result from ptest insn	*/
	ushort	exception_access;	/* cause of the bus error (read, write, read/write) */
	
	short	fork_flag;		/* flag: set to 1 if process has called Pfork() */
	
	short	auid;			/* tesche: audit user id */
# define NGROUPS_MAX	8
	short	ngroups;		/* tesche: number of supplementary groups	*/
	short	ngroup[NGROUPS_MAX];	/* tesche: supplementary groups	*/
	struct	itimervalue itimer[3];	/* interval timers */
	
	/* For a future improvement of getrusage().  */
# if 0
	long	ru_majflt;		/* Number of page faults	*/
	long	ru_inblock;		/* Number of block input operations	*/
	long	ru_oublock;		/* Number of block output operations	*/
	long	ru_msgsnd;		/* Number of messages sent	*/
	long	ru_msgrcv;		/* Number of messages received	*/
# endif
	int	last_sig;		/* Last signal received by the process	*/
	int	signaled;		/* Non-zero if process was killed by	*
					 * a fatal signal			*/
	
	struct timeval started;		/* Time the process started in	*
					 * UTC. (Other members starttime*
					 * and startdate are kept for 	*
					 * compatibility reasons.	*/
	
	fcookie exe;			/* File cookie for binary	*/
	char	*real_cmdline;		/* Saved command line		*/
	
	PROC	*q_prev;		/* prev process on queue	*/
	PROC	*q_next;		/* next process on queue	*/
	PROC	*gl_next;		/* next process in system	*/
	
	ulong	stack_magic;		/* to detect stack overflows	*/
	char	stack[STKSIZE+4];	/* stack for system calls	*/
};


/*
 * our process queues
 */

# define CURPROC_Q	0
# define READY_Q	1
# define WAIT_Q		2
# define IO_Q		3
# define ZOMBIE_Q	4
# define TSR_Q		5
# define STOP_Q		6
# define SELECT_Q	7

# define NUM_QUEUES	8


# ifdef __KERNEL__
extern PROC *proclist;			/* list of all active processes */
extern PROC *curproc;			/* current process		*/
extern PROC *rootproc;			/* pid 0 -- MiNT itself		*/
# endif


# endif /* _mint_proc_h */
