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


#define	NSIG		32		/* number of signals recognized */

#define	SIGNULL		0		/* not really a signal */
#define SIGHUP		1		/* hangup signal */
#define SIGINT		2		/* sent by ^C */
#define SIGQUIT		3		/* quit signal */
#define SIGILL		4		/* illegal instruction */
#define SIGTRAP		5		/* trace trap */
#define SIGABRT		6		/* abort signal */
#define SIGPRIV		7		/* privilege violation */
#define SIGFPE		8		/* divide by zero */
#define SIGKILL		9		/* cannot be ignored */
#define SIGBUS		10		/* bus error */
#define SIGSEGV		11		/* illegal memory reference */
#define SIGSYS		12		/* bad argument to a system call */
#define SIGPIPE		13		/* broken pipe */
#define SIGALRM		14		/* alarm clock */
#define SIGTERM		15		/* software termination signal */

#define SIGURG		16		/* urgent condition on I/O channel */
#define SIGSTOP		17		/* stop signal not from terminal */
#define SIGTSTP		18		/* stop signal from terminal */
#define SIGCONT		19		/* continue stopped process */
#define SIGCHLD		20		/* child stopped or exited */
#define SIGTTIN		21		/* read by background process */
#define SIGTTOU		22		/* write by background process */
#define SIGIO		23		/* I/O possible on a descriptor */
#define SIGXCPU		24		/* CPU time exhausted */
#define SIGXFSZ		25		/* file size limited exceeded */
#define SIGVTALRM	26		/* virtual timer alarm */
#define SIGPROF		27		/* profiling timer expired */
#define SIGWINCH	28		/* window size changed */
#define SIGUSR1		29		/* user signal 1 */
#define SIGUSR2		30		/* user signal 2 */
#define SIGPWR		31		/* power failure (restart) */

#define SIG_DFL		0
#define SIG_IGN		1
#define SIG_ERR		-1

/* which signals are not maskable? */
#define UNMASKABLE (1L | (1L << SIGKILL) | (1L << SIGCONT) | (1L << SIGSTOP))

/* which signals are stop signals? */
#define STOPSIGS ((1L<< SIGTTIN)|(1L<< SIGTTOU)|(1L<< SIGTSTP)|(1L<<SIGSTOP))

/* sigaction: extended POSIX signal handling facility
 */
struct sigaction
{
	ulong	sa_handler;	/* pointer to signal handler */
	ulong	sa_mask;	/* additional signals masked during delivery */
	ushort	sa_flags;	/* signal specific flags */
/* signal flags */
#define SA_NOCLDSTOP	1	/* don't send SIGCHLD when child stops */
#define SA_RESET	0x8000	/* reset to SIG_DFL just before delivery */
};


#define SAUSER		(SA_NOCLDSTOP)	/* signal flags which the process may set */
#define SAPARENT	(0)		/* signal flags which the parent (of ptraced) processes may set */
#define SAKERNEL	(SA_RESET)	/* kernel only flags */


# endif /* _mint_signal_h */
