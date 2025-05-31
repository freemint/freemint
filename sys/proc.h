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
extern struct proc_queue sysq[NUM_QUEUES];

/* macro for calculating number of missed time slices, based on a
 * process' priority
 */
# define SLICES(pri)	(((pri) >= 0) ? 0 : -(pri))

# define TICKS_PER_TOCK		200
# define TOCKS_PER_SECOND	1

# define SAMPS_PER_MIN		12
# define SAMPS_PER_5MIN		SAMPS_PER_MIN * 5
# define SAMPS_PER_15MIN	SAMPS_PER_MIN * 15

# define LOAD_SCALE		2048

extern ulong uptime;
extern ulong avenrun[3];
extern ushort uptimetick;


void		init_proc	(void);

void		reset_priorities(void);
void		run_next	(struct proc *p, int slices);
void		fresh_slices	(int slices);

void		add_q		(int que, struct proc *proc);
void		rm_q		(int que, struct proc *proc);

void	_cdecl	preempt		(void);
int	_cdecl	sleep		(int que, long cond);
void	_cdecl	wake		(int que, long cond);
void	_cdecl	iwake		(int que, long cond, short pid);
void	_cdecl	wakeselect	(struct proc *p);

void		DUMPPROC	(void);
void		calc_load_average (void);

ulong	_cdecl	remaining_proc_time (void);

struct proc *_cdecl get_curproc(void);

# endif /* _proc_h */
