/*
 * $Id$
 * 
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
pid2proc (int pid)
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
newpid (void)
{
	register int i;
	register int j = 0;
	
	do {
		i = _maxpid++;
		if (_maxpid >= MAXPID)
			_maxpid = 2;
		
		assert (j++ < MAXPID);
	}
	while (pid2proc (i));
	
	return i;
}

/*
 * set up to run the init program as PID 1
 */
int
set_pid_1 (void)
{
	if (pid2proc (1))
		/* should never happen, only called once */
		return -1;
	
	_maxpid = 1;
	return 0;
}
