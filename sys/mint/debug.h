/*
 * $Id$
 *
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

# define DBG FORCE

# ifndef DEBUG_INFO

# define TRACELOW(x)
# define TRACE(x)
# define DEBUG(x)

# define DBG_FORCE(x)

# define FUNCTION __FUNCTION__

# else /* DEBUG_INFO */

# define TRACELOW(s)	Tracelow s

# define TRACE(s)	Trace s

# define DEBUG(s)	Debug s

/* can be removed by sed */
# define DBG_FORCE(x) FORCE x
# define DBG_IF(a,x) if(a)FORCE x

# ifndef str
# define str(x)		_stringify(x)
# define _stringify(x)	#x
# endif

# define FUNCTION __FILE__","str(__LINE__)

# endif /* DEBUG_INFO */


void		debug_ws	(const char *s);
int		_ALERT		(char *);

void	_cdecl	Tracelow	(const char *s, ...);
void	_cdecl	Trace		(const char *s, ...);
void		display		(const char *s, ...);
void	_cdecl	Debug		(const char *s, ...);
void	_cdecl	ALERT		(const char *s, ...);
void	_cdecl	FORCE		(const char *s, ...);

EXITING	_cdecl	FATAL		(const char *s, ...)	NORETURN;

void		DUMPLOG		(void);
void		do_func_key	(int);


# endif /* _mint_debug_h */
