/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_debug_h
# define _mint_debug_h


extern int debug_level;
extern int out_device;
extern int debug_logging;

/*
 * Extra terse settings - don't even output ALERTs unless asked to.
 *
 * Things that happen in on an idle Desktop are at LOW_LEVEL:
 * Psemaphore, Pmsg, Syield.
 */

# define FORCE_LEVEL	0
# define ALERT_LEVEL	1
# define DEBUG_LEVEL	2
# define TRACE_LEVEL	3
# define LOW_LEVEL	4


# ifndef DEBUG_INFO

# define TRACELOW(x)
# define TRACE(x)
# define DEBUG(x)

# define FUNCTION __FUNCTION__

# else

# define TRACELOW(s)	Tracelow s

# define TRACE(s)	Trace s

# define DEBUG(s)	Debug s 

# ifndef str
# define str(x)		_stringify(x)
# define _stringify(x)	#x
# endif

# define FUNCTION __FILE__","str(__LINE__)

# endif /* DEBUG_INFO */


void		debug_ws	(const char *s);
int		_ALERT		(const char *);

void	_cdecl	Tracelow	(const char *s, ...) __attribute__((format(printf, 1, 2)));
void	_cdecl	Trace		(const char *s, ...) __attribute__((format(printf, 1, 2)));
void		display		(const char *s, ...) __attribute__((format(printf, 1, 2)));
void	_cdecl	Debug		(const char *s, ...) __attribute__((format(printf, 1, 2)));
void	_cdecl	ALERT		(const char *s, ...) __attribute__((format(printf, 1, 2)));
void	_cdecl	FORCE		(const char *s, ...) __attribute__((format(printf, 1, 2)));

EXITING	_cdecl	FATAL		(const char *s, ...) __attribute__((format(printf, 1, 2))) NORETURN;

void		DUMPLOG		(void);
void		do_func_key	(int);


# endif /* _mint_debug_h */
