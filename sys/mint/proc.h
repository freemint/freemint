/*
 * $Id$
 * 
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

# include "ktypes.h"

# include "basepage.h"
# include "config.h"
# include "mem.h"
# include "lists.h"
# include "file.h"
# include "time.h"

# include "arch/context.h"


# define LOGIN_NAME_MAX	32
# define MAXLOGNAME	LOGIN_NAME_MAX

/*
 * One structure allocated per session
 */
struct session
{
	long	links;
	struct proc *leader;		/* session leader */
	fcookie *ttyvp;			/* vnode of controlling terminal */
	struct tty *ttyp;		/* controlling terminal */
	char	login[MAXLOGNAME];	/* Setlogin() name */
	short	sid;			/* session ID (pid of leader) */
};

/*
 * One structure allocated per process group
 */
struct pgrp
{
	LIST_ENTRY(pgrp) pg_hash;	/* Hash chain. */
	LIST_HEAD(, proc) pg_members;	/* Pointer to pgrp members. */
	struct session *pg_session;	/* Pointer to session. */
	short	pg_id;			/* Pgrp id. */
};

struct memspace
{
	long	links;
	
	ushort	num_reg;		/* number of allocated memory	*
					 * regions			*/
	MEMREGION **mem;		/* allocated memory regions	*/
	long	*addr;			/* addresses of regions		*/
	
	void	*page_table;		/* rounded page table pointer	*/
	void	*pt_mem;		/* original kmalloc'd block for	*
					 * above			*/
	
	long	txtsize;		/* size of text region		*
					 * (for fork())			*/
};


# define NUM_REGIONS	64	/* number of memory regions alloced at a time */
# define MIN_HANDLE	(-5)	/* minimum handle number		*/
# define MIN_OPEN	6	/* 0..MIN_OPEN-1 are reserved for system */
# define SSTKSIZE	8192	/* size of supervisor stack (in bytes) 	*/
# define ISTKSIZE	4096	/* size of interrupt stack (in bytes)	*/
# define STKSIZE	(ISTKSIZE + SSTKSIZE)

# define STACK_MAGIC	0xfedcba98UL
				/* magic for the stack barrier */
# define PNAMSIZ	8	/* no. of characters in a process name */

# define DOM_TOS	0	/* TOS process domain */
# define DOM_MINT	1	/* MiNT process domain */

/* forward declarations */
struct pcred;
struct filedesc;
struct cwd;
struct sigacts;
struct limits;

/*
 * from now it's private; at all, forever
 */
struct proc
{
	long	sysstack;		/* must be first		*/
	CONTEXT	ctxt[PROC_CTXTS];	/* must be second		*/
	
	long	magic;			/* validation for proc struct	*/
	
	BASEPAGE *base;			/* process base page		*/
	short	pid, ppid, pgrp;
	short	_ruid;			/* unused */
	short	_rgid;			/* unused */
	short	_euid;			/* unused */
	short	_egid;			/* unused */
	
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
	
	ulong	systime;		/* XXX unused */
	ulong	usrtime;		/* XXX unused */
	ulong	chldstime;		/* XXX unused */
	ulong	chldutime;		/* XXX unused */
	
	ulong	maxmem;			/* XXX max. amount of memory to use */
	ulong	maxdata;		/* XXX max. data region for process */
	ulong	maxcore;		/* XXX max. core memory for process */
	ulong	maxcpu;			/* XXX max. cpu time to use 	*/
	
	short	domain;			/* process domain (TOS or UNIX)	*/
	
	short	curpri;			/* current process priority	*/
	short	_suid, _sgid;		/* XXX unused */
# define MIN_NICE -20
# define MAX_NICE  20

/* 
 * magic line:
 * ---------------------------------------------------------------------------
 */
	ushort	p_flag;			/* */
	ushort	p_stat;			/* */
	
	
	/* sharable substructures */
	struct memspace	*p_mem;		/* address space */
	struct pcred	*p_cred;	/* owner identity */
	struct filedesc	*p_fd;		/* open files */
	struct cwd	*p_cwd;		/* path stuff */
	struct sigacts	*p_sigacts;	/* signal stuff */
	struct plimit	*p_limits;	/* process limits */
	
	
	/* statistics */
	struct timeval started;		/* start time in UTC		*/
	
	/* XXX all times are in milliseconds
	 * XXX usrtime must always follow systime
	 */
	ulong	_systime;		/* time spent in kernel		*/
	ulong	_usrtime;		/* time spent out of kernel	*/
	ulong	_chldstime;		/* children's kernel time 	*/
	ulong	_chldutime;		/* children's user time		*/	
# if 0
	/* For a future improvement of getrusage() */
	long	ru_majflt;		/* Number of page faults	*/
	long	ru_inblock;		/* Number of block input operations	*/
	long	ru_oublock;		/* Number of block output operations	*/
	long	ru_msgsnd;		/* Number of messages sent	*/
	long	ru_msgrcv;		/* Number of messages received	*/
# endif
	
	
	/* jr: two fields to hold information passed to Pexec */
	char	fname[PATH_MAX];	/* name of binary		*/
	char	cmdlin[128];		/* original command line	*/
	
	char	*real_cmdline;		/* Saved command line		*/
	fcookie exe;			/* File cookie for binary	*/
	
	char	name[PNAMSIZ+1];	/* process name			*/
	short	slices;			/* number of time slices before	*
					 * this process gets to run	*
					 * again			*/
	
	
	TIMEOUT	*alarmtim;		/* alarm() event		*/
	struct	itimervalue itimer[3];	/* interval timers */
	
	struct pgrp *p_pgrp;		/* XXX process group		*/
	void	*p_ctxlink;		/* XXX uc_link {get,set}context */
	
	
	ulong	sigpending;		/* pending signals		*/
	ulong	nsigs;			/* number of signals delivered 	*/
	
	ulong	p_sigmask;		/* current signal mask		*/
//	ulong	p_sigignore;		/* signals being ignored	*/
//	ulong	p_sigcatch;		/* signals being caught by user	*/
	
	
	long	_cdecl (*criticerr)(long);
					/* critical error handler	*/
	void	*logbase;		/* XXX logical screen base	*/
	
	PROC	*ptracer;		/* process which is tracing this one */
	short	ptraceflags;		/* flags for process tracing	*/
	
	short	in_dos;			/* flag: 1 = process is executing a GEMDOS call */
	short	fork_flag;		/* flag: set to 1 if process has called Pfork() */
	short	auid;			/* XXX tesche: audit user id */
	
	short	last_sig;		/* Last signal received by the process	*/
	short	signaled;		/* Non-zero if process was killed by	*
					 * a fatal signal			*/
	
	
	long	links;			/* reference count		*/
	PROC	*q_prev;		/* prev process on queue	*/
	PROC	*q_next;		/* next process on queue	*/
	PROC	*gl_next;		/* next process in system	*/
	
	
	/* GEMDOS extension: Pmsg() */
	char	*mb_ptr;		/* p_msg buffer ptr		*/
	long	mb_long1, mb_long2;	/* p_msg storage		*/
	long	mb_mbid;		/* p_msg id being waited for	*/
	short	mb_mode;		/* p_msg mode being waiting in	*/
	short	mb_writer;		/* p_msg pid of writer of msg	*/
	
	/* GEMDOS extension: Pusrval() */
	long	usrdata;		/* p_usrval user-supplied data	*/
	
	
	ulong	exception_pc;		/* pc at time of bombs		*/
	ulong	exception_ssp;		/* ssp at time of bomb (e.g. bus error)	*/
	ulong	exception_tbl;		/* table in use at exception time */
	ulong	exception_addr;		/* access address from stack	*/
	ushort	exception_mmusr;	/* result from ptest insn	*/
	ushort	exception_access;	/* cause of the bus error (read, write, read/write) */
	
	
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
# if __KERNEL__ == 1

extern PROC *proclist;			/* list of all active processes */
extern PROC *curproc;			/* current process		*/
extern PROC *rootproc;			/* pid 0 -- MiNT itself		*/

# endif
# endif


# endif /* _mint_proc_h */
