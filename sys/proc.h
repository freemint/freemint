/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _proc_h
# define _proc_h

# include "mint/mint.h"
# include "mint/proc.h"


extern ushort proc_clock;		/* timeslices */
extern short time_slice;
extern PROC *sys_q[NUM_QUEUES];		/* process queues */


# define TICKS_PER_TOCK		200
# define TOCKS_PER_SECOND	1

# define SAMPS_PER_MIN		12
# define SAMPS_PER_5MIN		SAMPS_PER_MIN * 5
# define SAMPS_PER_15MIN	SAMPS_PER_MIN * 15

# define LOAD_SCALE		2048

extern ulong uptime;
extern ulong avenrun[3];
extern ushort uptimetick;


PROC *		new_proc	(void);
void		dispose_proc	(PROC *p);

PROC *		fork_proc	(long *err);
void		init_proc	(void);

void		reset_priorities(void);
void		run_next	(PROC *p, int slices);
void		fresh_slices	(int slices);

void		add_q		(int que, PROC *proc);
void		rm_q		(int que, PROC *proc);

void	_cdecl	preempt		(void);
int	_cdecl	sleep		(int que, long cond);
void	_cdecl	wake		(int que, long cond);
void	_cdecl	iwake		(int que, long cond, short pid);
void	_cdecl	wakeselect	(PROC *p);

void		DUMPPROC	(void);
void		calc_load_average (void);


# endif /* _proc_h */
