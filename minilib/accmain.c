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
#include <mint/mintbind.h>
#include "lib.h"

extern long _stksize;
extern short _app;

extern int main (int, char **, char **);

static void _main (int _argc, char **_argv, char **_envp)
{
	/* if stderr is not re-directed to a file, force 2 to console
	 * (UNLESS we've been run from a shell we trust, i.e. one that supports
	 *  the official ARGV scheme, in which case we leave stderr be).
	 */
#if 0
	if(!*_argv[0] && isatty (2))
		(void) Fforce(2, -1);
#endif

	exit(main(_argc, _argv, _envp));
}


void _acc_main(void) {
	void *_heapbase;
	
	if (_stksize == 0 || _stksize == -1L)
		_stksize = MINKEEP;

	if (_stksize < 0)
		_stksize = -_stksize;

#if 0
	if ((s = getenv("STACKSIZE")) != 0)
		_stksize = atoi(s);
#endif

	/* stack on word boundary */
	_stksize &= 0xfffffffeL;

	_heapbase = (void *)Malloc(_stksize);
	_setstack((char *) _heapbase + _stksize);

	/* this is an accessory */
	_app = 0;

	_main(__libc_argc, __libc_argv, NULL);
	/*NOTREACHED*/
}
