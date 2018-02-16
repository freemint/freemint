/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_signal_h
# define _mint_signal_h

# include "ktypes.h"


typedef unsigned long int sigset_t;

# define NSIG		32		/* number of signals recognized */

# define SIGNULL	 0		/* not really a signal */
# define SIGHUP		 1		/* hangup signal */
# define SIGINT		 2		/* sent by ^C */
# define SIGQUIT	 3		/* quit signal */
# define SIGILL		 4		/* illegal instruction */
# define SIGTRAP	 5		/* trace trap */
# define SIGABRT	 6		/* abort signal */
# define SIGPRIV	 7		/* privilege violation */
# define SIGFPE		 8		/* divide by zero */
# define SIGKILL	 9		/* cannot be ignored */
# define SIGBUS		10		/* bus error */
# define SIGSEGV	11		/* illegal memory reference */
# define SIGSYS		12		/* bad argument to a system call */
# define SIGPIPE	13		/* broken pipe */
# define SIGALRM	14		/* alarm clock */
# define SIGTERM	15		/* software termination signal */
# define SIGURG		16		/* urgent condition on I/O channel */
# define SIGSTOP	17		/* stop signal not from terminal */
# define SIGTSTP	18		/* stop signal from terminal */
# define SIGCONT	19		/* continue stopped process */
# define SIGCHLD	20		/* child stopped or exited */
# define SIGTTIN	21		/* read by background process */
# define SIGTTOU	22		/* write by background process */
# define SIGIO		23		/* I/O possible on a descriptor */
# define SIGXCPU	24		/* CPU time exhausted */
# define SIGXFSZ	25		/* file size limited exceeded */
# define SIGVTALRM	26		/* virtual timer alarm */
# define SIGPROF	27		/* profiling timer expired */
# define SIGWINCH	28		/* window size changed */
# define SIGUSR1	29		/* user signal 1 */
# define SIGUSR2	30		/* user signal 2 */
# define SIGPWR		31		/* power failure (restart) */

/* XXX which signals are not maskable? */
# define UNMASKABLE (1L | (1L << SIGKILL) | (1L << SIGCONT) | (1L << SIGSTOP))

/* XXX which signals are stop signals? */
# define STOPSIGS ((1L << SIGTTIN)|(1L << SIGTTOU)|(1L << SIGTSTP)|(1L << SIGSTOP))

/* sigaction: extended POSIX signal handling facility
 */
struct sigaction
{
	unsigned long  sa_handler;	/* pointer to signal handler */
	unsigned long  sa_mask;		/* additional signals masked during delivery */
	unsigned short sa_flags;	/* signal specific flags */
};

/* signal handler values */
# define SIG_ERR	-1
# define SIG_DFL	 0
# define SIG_IGN	 1

/* signal flags */
# define SA_ONSTACK	0x2000		/* take signal on signal stack */
# define SA_RESTART	0x4000		/* restart system on signal return */
# define SA_RESETHAND	0x8000		/* reset to SIG_DFL just before delivery */
# define SA_NODEFER	0x0010		/* don't mask the signal we're delivering */
# define SA_SIGINFO	0x0040
/* only valid for SIGCHLD */
# define SA_NOCLDSTOP	0x0001		/* don't send SIGCHLD when child stops */
# define SA_NOCLDWAIT	0x0002		/* do not generate zombies on unwaited child */
# define SA_ALLBITS	0x007f

# define SAUSER		(SA_NODEFER | SA_NOCLDSTOP | SA_RESETHAND)	/* XXX signal flags which the process may set */
# define SAPARENT	(0)		/* XXX signal flags which the parent (of ptraced) processes may set */
# define SAKERNEL	(0)	/* XXX kernel only flags */


/* values for ss_flags */
# define SS_ONSTACK	1
# define SS_DISABLE	2


# define MINSIGSTKSZ	2048		/* minimum stack size */
# define SIGSTKSZ	8192		/* default stack size */

typedef struct sigaltstack
{
	void *ss_sp;
	long ss_flags;
	unsigned long ss_size;
}
stack_t;

struct sigacts
{
	struct sigaction sigact[NSIG];	/* disposition of signals */
	stack_t sigstk;			/* sp & on stack state variable */
	sigset_t oldmask;		/* saved mask from before sigpause */
	long flags;			/* signal flags, below */
	long links;			/* reference count */
};

/* signal flags */
# define SAS_OLDMASK	0x01		/* need to restore mask before pause */

/* helper macro */
# define SIGACTION(p, sig)		((p)->p_sigacts->sigact[(sig)])

# endif /* _mint_signal_h */
