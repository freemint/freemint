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

# ifndef __KERNEL__
# error __KERNEL__ not defined!
# endif

# include "mint.h"

# include "basepage.h"
# include "config.h"
# include "mem.h"
# include "lists.h"
# include "file.h"
# include "time.h"

# include "arch/context.h"


# define LOGIN_NAME_MAX	32
# define MAXLOGNAME	LOGIN_NAME_MAX

/* Threads stuff */

enum sched_policy {
    SCHED_FIFO,
    SCHED_RR,
    SCHED_OTHER
};

struct thread {
    /* Thread identification */
    short tid;                      /* Thread ID */
    struct proc *proc;              /* Parent process */
    
    /* Stack information */
    void *stack;                    /* Stack base address */
    void *stack_top;                /* Top of stack area */
    unsigned long stack_magic;      /* Stack integrity check */
    
    /* Thread context */
    CONTEXT ctxt[PROC_CTXTS];       /* Thread context (reuse FreeMiNT's context) */
    
    /* Thread state and scheduling */
    short state;                    /* Thread state (RUNNING/READY/BLOCKED) */
	short is_idle;                    /* Flag indicating if this is an idle thread */
    short priority;                 /* Thread priority */
    short original_priority;        /* Original priority (for restoration after boost) */
    short priority_boost;           /* Flag indicating if priority is currently boosted */
    short timeslice;                /* Timeslice for this thread */
    short remaining_timeslice;      /* Remaining timeslice */
    short total_timeslice;          /* Total timeslice allocated to this thread */
    enum sched_policy policy;       /* Scheduling policy (SCHED_FIFO, SCHED_RR, SCHED_OTHER) */
    
    /* Thread linking */
    struct thread *next;            /* Next thread in process list */
    struct thread *next_ready;      /* For ready queue */
    struct thread *next_sleeping;   /* For sleep queue */
	struct thread *next_sigwait;    /* For signal wait queue */
    struct thread *next_wait;       /* For wait queue */
    
    /* Validation and debugging */
    unsigned long magic;            /* Magic number for validation */
    
    /* Thread execution */
    void* (*func)(void*);            /* Function to execute */
    void *arg;                      /* Argument to pass to function */
    
    /* Sleep and wait information */
    short sleep_reason;             /* Reason for sleep (0 = woken by signal/other, 1 = timeout) */
	TIMEOUT *sleep_timeout;  		// Timeout for sleeping
    TIMEOUT *alarm_timeout;         /* Per-thread alarm timeout */
    short wait_type;                /* Type of wait (WAIT_SIGNAL, WAIT_MUTEX, WAIT_CONDVAR, WAIT_IO, etc.) */
	void *mutex_wait_obj;
	void *sem_wait_obj;
	void *sig_wait_obj;
	void *cond_wait_obj;
	void *join_wait_obj;
    
    /* Scheduling and timing */
    unsigned long wakeup_time;      /* Wakeup time in ticks */
    unsigned long last_scheduled;   /* Last time this thread was scheduled (in ticks) */
    
    /* Thread join fields */
    void *retval;                /* Return value from proc_thread_exit */
    struct thread *joiner;       /* Thread that is joining this thread */
    int detached;                /* Whether thread is detached */
    int joined;                  /* Whether thread has been joined */
    void **join_retval;          /* Where to store return value for joiner */
    
    /* Signal handling */
    unsigned long t_sigpending;     /* Signals pending for this thread */
    unsigned long t_sigmask;        /* Thread-specific signal mask */
    short t_sig_in_progress;        /* Signal currently being processed */
    CONTEXT sig_ctx;           /* Signal handler context */
    void *sig_stack;           /* Signal handler stack */
    ulong old_sigmask;         /* Saved signal mask during handler execution */	
    /* Thread-specific signal handlers */
    struct {
        void (*handler)(int, void*);
        void *arg;
    } sig_handlers[32];	
};

/**
 * Thread join structure to track join relationships
 */
struct thread_join {
    struct thread *target;     /* Thread being joined */
    struct thread *joiner;     /* Thread doing the joining */
    void *retval;              /* Return value from target thread */
    int joined;                /* Flag indicating join completed */
};

long _cdecl sys_p_thread_ctrl(long mode, long arg1, long arg2);
long _cdecl sys_p_thread_signal(long func, long arg1, long arg2);
long _cdecl sys_p_thread_sync(long operator, long arg1, long arg2);
long _cdecl sys_p_thread_sched_policy(long func, long arg1, long arg2, long arg3);

long _cdecl proc_thread_create(void *(*func)(void*), void *arg, void *stack);
void proc_thread_cleanup_process(struct proc *pcurproc); /** Called in terminate function - k_exit.c */
int proc_thread_signal_aware_raise(struct proc *p, int sig);
void dispatch_thread_signals(struct thread *t);

/* End of Threads stuff */

/*
 * One structure allocated per session
 */
struct session
{
	long links;
	struct proc *leader;		/* session leader */
	fcookie *ttyvp;			/* vnode of controlling terminal */
	struct tty *ttyp;		/* controlling terminal */
	char login[MAXLOGNAME];		/* Setlogin() name */
	short sid;			/* session ID (pid of leader) */
};

/*
 * One structure allocated per process group
 */
struct pgrp
{
	LIST_ENTRY(pgrp) pg_hash;	/* Hash chain. */
	LIST_HEAD(, proc) pg_members;	/* Pointer to pgrp members. */
	struct session *pg_session;	/* Pointer to session. */
	short pg_id;			/* Pgrp id. */
};

/** @struct memspace
 * Description of the memory used by a process.
 * There is a structure for each process. To walk throuh the memregions use:
 * 	mem[i], where i in [0, num_reg)   OR   0<= i < num_reg
 */
struct memspace
{
	long links;

	unsigned short	memflags;	///< malloc preferences
	int		num_reg;	///< Nnumber of allocated memoryregions.
	MEMREGION	**mem;		///< Array of ptrs to allocated regions.
	unsigned long	*addr;		///< Array of addresses of regions.
	void		*page_table;	///< rounded page table pointer.
	void		*pt_mem;	///< original kmalloc'd block for above.
	long		txtsize;	///< size of text region (for fork()).
	struct user_things *tp_ptr;	///< pointer to the trampoline things
	MEMREGION	*tp_reg;	///< memregion of the trampoline code
	BASEPAGE	*base;		///< process base page
};


# define NUM_REGIONS	64	/* number of memory regions alloced at a time */
# define MIN_HANDLE	(-5)	/* minimum handle number		*/
# define MIN_OPEN	6	/* 0..MIN_OPEN-1 are reserved for system */
/*# define SSTKSIZE	8192*/	/* size of supervisor stack (in bytes) 	*/
/*# define ISTKSIZE	4096*/	/* size of interrupt stack (in bytes)	*/
#define SSTKSIZE	16384
#define ISTKSIZE	8192
# define STKSIZE	(ISTKSIZE + SSTKSIZE)

# define STACK_MAGIC	0xfedcba98UL
				/* magic for the stack barrier */
# define PNAMSIZ	32	/* no. of characters in a process name */

# define DOM_TOS	0	/* TOS process domain */
# define DOM_MINT	1	/* MiNT process domain */


struct proc_queue {
	struct proc *head;
	struct proc *tail;
};

/* forward declarations */
struct pcred;
struct filedesc;
struct cwd;
struct sigacts;
struct limits;

/*
 * from now it's private; at all, forever
 */
/**
 * The process structure.
 */
struct proc
{
	unsigned long	sysstack;		/* must be first		*/
	CONTEXT		ctxt[PROC_CTXTS];	/* must be second		*/

	unsigned long	magic;			/* validation for proc struct	*/

	BASEPAGE * _base;		/* unused */
	short	pid, ppid, pgrp;
	short	_ruid;			/* unused */
	short	_rgid;			/* unused */
	short	_euid;			/* unused */
	short	_egid;			/* unused */

# define M_SINGLE_TASK    0x0001    /* XaAES: if set (in modeflags) it's "single-task-mode" */
# define M_DONT_STOP      0x0002    /* XaAES: if set do not stop when entering single-task-mode */
# define M_XA_CLIENT_EXIT 0x8000    /* XaAES: appl_exit called */
	ushort	modeflags;

	short	pri;			/**< base process priority 	*/
	short	wait_q;			/**< current process queue	*/

	/* note: wait_cond should be unique for each kind of condition
	 * we might want to wait for. Put a define below, or use an
	 * address in the kernel as the wait condition to ensure
	 * uniqueness.
	 */
	long	wait_cond;		/**< condition we're waiting on	*/
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

	short	domain;			/**< process domain (TOS or UNIX)	*/

	short	curpri;			/**< current process priority	*/
	short	_suid, _sgid;		/* XXX unused */
# define MIN_NICE -20
# define MAX_NICE  20

/*
 * magic line:
 * ---------------------------------------------------------------------------
 */

# define P_FLAG_SYS	0x0001		/* Unkillable system process */
# define P_FLAG_SLB	0x0002		/* Unkillable SLB */
# define P_FLAG_SLO	0x0004		/* Flag for exec_region() */
# define P_FLAG_SUPER	0x0008		/* Program called Super() or Supexec() */
# define P_FLAG_BER	0x0010		/* Program wanted to record the bus error vector */

	ushort	p_flag;
	ushort	p_stat;			/* */
	ushort	debug_level;		/* debug-level of the process	*/
	ushort	pad;

	/* sharable substructures */
	struct memspace	*p_mem;		/**< address space */
	struct pcred	*p_cred;	/**< owner identity */
	struct filedesc	*p_fd;		/**< open files */
	struct cwd	*p_cwd;		/**< path stuff */
	struct sigacts	*p_sigacts;	/**< signal stuff */
	struct plimit	*p_limits;	/**< process limits */
	struct p_ext	*p_ext;		/**< process extensions */

	/* statistics */
	struct timeval started;		/**< start time in UTC		*/

	/* XXX all times are in milliseconds
	 * XXX usrtime must always follow systime
	 */
	ulong	_systime;		/**< time spent in kernel		*/
	ulong	_usrtime;		/**< time spent out of kernel	*/
	ulong	_chldstime;		/**< children's kernel time 	*/
	ulong	_chldutime;		/**< children's user time		*/
# if 0
	/* For a future improvement of getrusage() */
	long	ru_majflt;		/* Number of page faults	*/
	long	ru_inblock;		/* Number of block input operations	*/
	long	ru_oublock;		/* Number of block output operations	*/
	long	ru_msgsnd;		/* Number of messages sent	*/
	long	ru_msgrcv;		/* Number of messages received	*/
# endif


	/* jr: two fields to hold information passed to Pexec */
	char	fname[PATH_MAX];	/**< name of binary		*/
	char	cmdlin[128];		/**< original command line	*/

	char	*real_cmdline;		/**< Saved command line		*/
	fcookie exe;			/**< File cookie for binary	*/

	char	name[PNAMSIZ+1];	/**< process name			*/
	short	slices;			/**< number of time slices before	*
					 * this process gets to run	*
					 * again			*/

	struct procwakeup *wakeupthings;/**< things todo on the next wake */

	TIMEOUT	*alarmtim;		/**< alarm() event		*/
	struct	itimervalue itimer[3];	/**< interval timers */

	struct pgrp *p_pgrp;		/* XXX process group		*/
	void	*p_ctxlink;		/* XXX uc_link {get,set}context */


	ulong	sigpending;		/**< pending signals		*/
	ulong	nsigs;			/**< number of signals delivered */

	ulong	p_sigmask;		/**< current signal mask		*/
//	ulong	p_sigignore;		/* signals being ignored	*/
//	ulong	p_sigcatch;		/* signals being caught by user	*/

	ulong	berr;			/* the value the program tried to write */
					/* to the bus error vector (see check_exc.c) */

	long	_cdecl (*criticerr)(long);
					/* critical error handler	*/
	void	*logbase;		/* XXX logical screen base	*/

	PROC	*ptracer;		/**< process which is tracing this one */
	short	ptraceflags;		/**< flags for process tracing	*/

	short	in_dos;			/**< flag: 1 = process is executing a GEMDOS call */
	short	unused;
	short	in_vdi;			/**< flag: 1 = process is executing a VDI call (only on 68020+) */
	short	fork_flag;		/**< flag: set to 1 if process has called Pfork() */
	short	auid;			/* XXX tesche: audit user id */
	short	last_sig;		/**< Last signal received by the process	*/
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

/* Threads stuff */
	struct {
		short thread_id;		// Thread actuellement préempté
		short enabled;			// Timer actif ?
		TIMEOUT *timeout;		// Handle du timeout
		unsigned short sr;		// Etat d'interruption sauvegardé
		short in_handler;		// Handler en cours ?
	} p_thread_timer;

    struct thread *threads;           /* List of all threads in this process */
    struct thread *current_thread;    /* Currently executing thread */
    struct thread *ready_queue;       /* Queue of threads ready to run */
    struct thread *signal_wait_queue; /* Queue of threads waiting for signals */
    struct thread *sleep_queue;       /* Queue of sleeping threads */

	short num_threads;             /* Thread count */
	short total_threads;           /* Total thread count */

    /* Thread scheduling parameters */
    unsigned short thread_preempt_interval;    /* Preemption interval in ticks */
    unsigned short thread_default_timeslice;   /* Default timeslice in ticks */
    unsigned short thread_min_timeslice;       /* Minimum timeslice in ticks */
    unsigned short thread_rr_timeslice;        /* Round-robin timeslice in ticks */

	short thread_signals_enabled;   /* Cache for quick access */
/* End of Threads stuff */

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


# if __KERNEL__ == 1

extern PROC *proclist;			/* list of all active processes */
extern PROC *curproc;			/* current process		*/
extern PROC *rootproc;			/* pid 0 -- MiNT itself		*/

# endif


# endif /* _mint_proc_h */
