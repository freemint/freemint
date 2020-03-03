/*
 * main.c
 *
 *  Created on: 29.05.2013
 *      Author: mfro
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mintbind.h>
#include "lib.h"

extern int main (int, char **, char **);

static long parseargs(BASEPAGE *bp);

extern long _stksize;


#undef isspace
#define isspace(c) ((c) == ' '||(c) == '\t')
#undef isdigit
#define isdigit(c) ((c) >= '0' && (c) <= '9')

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)



static void _main (int _argc, char **_argv, char **_envp) {
	/* if stderr is not re-directed to a file, force 2 to console
	 * (UNLESS we've been run from a shell we trust, i.e. one that supports
	 *  the official ARGV scheme, in which case we leave stderr be).
	 */
	/* (void) Fforce(2, -1); */

	exit(main(_argc, _argv, _envp));
}

void _crtinit_noargs(void);
void _crtinit_noargs(void) {
	extern void etext(void);	/* a "function" to fake out pc-rel addressing */

	BASEPAGE *bp;
	long m;
	long freemem;

	/* its an application */
	_app = 1;

	bp = _base;

	/* m = # bytes used by environment + args */
	m = parseargs(bp);

	/* make m the total number of bytes required by program sans stack/heap */
	m += (bp->p_tlen + bp->p_dlen + bp->p_blen + sizeof(BASEPAGE));
	m = (m + 3L) & (~3L);

	/* freemem the amount of free mem accounting for MINFREE at top */
	if ((freemem = (long)bp->p_hitpa - (long)bp - MINFREE - m) <= 0L)
	    goto notenough;

	if (_stksize == -1L) {
		_stksize = freemem >> 1;
	} else if (_stksize == 0L) {	/* free all but MINKEEP */
		_stksize = MINKEEP;
	} else if (_stksize == 1L) { 	/* keep 1/4, free 3/4 */
		_stksize = freemem >> 2;
	} else if (_stksize ==  2L) {	/* keep 1/2, free 1/2 */
		_stksize = freemem >> 1;
	} else if (_stksize == 3L) {	/* keep 3/4, free 1/4 */
		_stksize = freemem - (freemem >> 2); 
	} else {
		if(_stksize < -1L) {	/* keep |_stksize|, use heap for mallocs */
			_stksize = -_stksize;
		}
	}
/*
	if ((s = getenv("STACKSIZE")) != 0)
		_stksize = atoi(s);
*/
	/* make m the total number of bytes including stack */
	_stksize = _stksize & (~3L);
	m += _stksize;

	/* make sure there's enough room for the stack */
	if (((long)bp + m) > ((long)bp->p_hitpa - MINFREE))
	    goto notenough;

	/* set up the new stack to bp + m  */
	_setstack((char *)bp + m);

	/* shrink the TPA */
	(void)Mshrink(bp, m);

	/* keep length of program area */
	_PgmSize = m;

	/* establish handlers,  call the main routine */
/*	_init_signal(); */

	/* start profiling, if we were linked with gcrt0.o */

	_main(__libc_argc, __libc_argv, environ);
	/* not reached normally */

notenough:
	Pterm(-1);
}

/*
 * parseargs(bp): parse the environment and arguments pointed to by the
 * basepage. Return the number of bytes of environment and arguments
 * that have been appended to the bss area (the environ and argv arrays
 * are put here, as is a temporary buffer for the command line, if
 * necessary).
 *
 * The MWC extended argument passing scheme is assumed.
 *
 */
static long parseargs(BASEPAGE *bp) {
	long count = 4;		/* compensate for aligning */
	char **envp, **arg;
	char *from;
	/* flag to indicate desktop-style arg. passing */
	long desktoparg;

	/* handle the environment first */

	environ = envp = (char **)(( (long)bp->p_bbase + bp->p_blen + 4) & (~3));
	from = bp->p_env;
	while (*from) {

		*envp++ = from;
		count += 4;
		desktoparg = 1;
		while (*from) {
			if (*from == '=') {
				desktoparg = 0;
			}
			from++;
		}
		from++;		/* skip 0 */

/* the desktop (and some shells) use the environment in the wrong
   way, putting in "PATH=\0C:\0" instead of "PATH=C:". so if we
   find an "environment variable" without an '=' in it, we
   see if the last environment variable ended with '=\0', and
   if so we append this one to the last one
 */
		if(desktoparg && envp > &environ[1]) 
		{
		/* launched from desktop -- fix up env */
		    char *p, *q;

		    q = envp[-2];	/* current one is envp[-1] */
		    while (*q) q++;
		    if (q[-1] == '=') {
			p = *--envp;
			while(*p)
			   *q++ = *p++;
		        *q = '\0';
		   }
		}
	}
	*envp++ = (char *)0;
	count += 4;

	__libc_argv = arg = envp;
	*arg++ = "";		/* __libc_argv[0] not available */
	count += 4;

	__libc_argc = 1;		/* at this point __libc_argv[0] is done */
	*arg = (char *) 0;

	return count+4;
}
