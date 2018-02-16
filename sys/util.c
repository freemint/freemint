/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993 Atari Corporation.
 * All rights reserved.
 * 
 * 
 * misc. utility routines
 * 
 */

# include "util.h"
# include "mint/proc.h"


/*
 * given a pid, return the corresponding process
 */

struct proc *
pid2proc(int pid)
{
	struct proc *p;
	
	for (p = proclist; p; p = p->gl_next)
		if (p->pid == pid)
			return p;
	
	return NULL;
}

/*
 * return a new pid
 */

static int _maxpid = 2;
	
int
newpid(void)
{
	register int i;
	register int j = 2;
	
	do {
		i = _maxpid++;
		
		/* dont use PID 1 */
		if (_maxpid >= MAXPID)
			_maxpid = 2;
		
		if (j++ > MAXPID)
		{
			/* XXX better sleep until a PID is available */
			FATAL("no free PID's");
		}
	}
	while (pid2proc(i));
	
	return i;
}

/*
 * try to run the next process with PID 1
 */
void
set_pid_1(void)
{
	_maxpid = 1;
}
