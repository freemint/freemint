/* @(#) thread.c
 * tfork(function, argument): starts a new thread of execution running
 * in the same address space. The new thread gets its own 4K stack,
 * and starts at the address in "function" with "argument" on the stack,
 * i.e. as though the main program had a call like "function(argument)".
 * The main program continues executing, with tfork returning the process
 * i.d. of the child.
 * (if MiNT is not active, then the child runs to completion
 *  and the return value is the child's exit status; vfork() relies on
 *  this behavior)
 *
 * Note that parent and child share the same memory; this could cause
 * problems with some library calls, notably malloc().
 *
 * modified by Ulrich KÅhn for use under MiNT only, 27-Jul-92
 */

#include <mint/osbind.h>
#include <mint/basepage.h>

#define SIZE 8192L

static void
startup(b)
	register BASEPAGE *b;
{
	register int (*func)();
	register long arg;
	extern void _setstack();	/* in crt0.s */

	_setstack( ((long)b) + SIZE );
	func = (int (*)())b->p_dbase;
	arg = b->p_dlen;
	Pterm((*func)(arg));
}

/* use long instead of int so vfork works OK with -mshort */

long
thread(char *newname, int (*func)(), long arg)
{
	register BASEPAGE *b;
	register long pid;

	b = (BASEPAGE *)Pexec(PE_CBASEPAGE, 0L, "", 0L);
	(void)Mshrink(b, SIZE);
	b->p_tbase = (char *)startup;
	b->p_dbase = (char *)func;
	b->p_dlen = arg;
	b->p_hitpa = ((char *)b) + SIZE;

	pid = Pexec(104, newname, b, 0L);
	if (pid<0)  pid = -1L;
	return pid;
}
