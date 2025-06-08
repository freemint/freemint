#include "libkern/libkern.h" /* memset and memcpy */

#include "proc.h"
#include "mint/signal.h"
#include "timeout.h"
#include "arch/context.h"
#include "mint/arch/asm_spl.h"
#include "kmemory.h"
#include "kentry.h"
#include "proc_threads_debug.h"

#ifndef PROC_THREAD_H
#define PROC_THREAD_H

#define THREAD_SUCCESS  0 // Success

#define MS_PER_TICK 5 // 200Hz = 5ms/tick
#define MAX_SWITCH_RETRIES 3

#define MAX_THREAD_PRIORITY 99 /* Maximum thread priority (POSIX compliant) */
#define THREAD_CREATION_PRIORITY_BOOST 3  /* Priority boost for newly created threads */

/* Default scheduling policy */
#define DEFAULT_SCHED_POLICY SCHED_FIFO

/*
	RUNNING	0x0001	Currently executing
	READY	0x0002	On run queue, can be scheduled
	BLOCKED	0x0004	Sleeping, waiting for event
	STOPPED	0x0010	Stopped by signal
	ZOMBIE	0x0020	Exited, not yet reaped
	DEAD	0x0040	Fully dead, resources can be freed
*/
#define THREAD_STATE_RUNNING    0x0001
#define THREAD_STATE_READY      0x0002
#define THREAD_STATE_BLOCKED	0x0004
#define THREAD_STATE_STOPPED	0x0008
#define THREAD_STATE_ZOMBIE		0x0010
#define THREAD_STATE_DEAD		0x0020
// For checks only:
#define THREAD_STATE_EXITED     (THREAD_STATE_ZOMBIE | THREAD_STATE_DEAD)
#define THREAD_STATE_LIVE       (THREAD_STATE_RUNNING | THREAD_STATE_READY)

/* Thread operation modes for sys_p_thread_ctrl */
#define THREAD_CTRL_EXIT     0   /* Exit the current thread */
#define THREAD_CTRL_STATUS   4   /* Get thread status */
#define THREAD_CTRL_GETID    5	/* Get thread ID */

/* Thread wait types */
#define WAIT_NONE		0	// No wait type specified (not waiting on any resource)
#define WAIT_SIGNAL		1	// Waiting for a signal to be delivered (e.g. SIGINT, SIGTERM)
#define WAIT_MUTEX		2	// Waiting for a mutex (mutual exclusion) lock to be released
#define WAIT_CONDVAR	3	// Waiting for a condition variable to be signaled
#define WAIT_IO			4	// Waiting for I/O (input/output) operation to complete
#define WAIT_SEMAPHORE	5	// Waiting for a semaphore to be released
#define WAIT_SLEEP		6	// Waiting for sleep timeout
#define WAIT_JOIN       7  /* Waiting for thread to exit */

/* Thread scheduling system call constants */
#define PSCHED_SETPARAM       1
#define PSCHED_GETPARAM       2
#define PSCHED_GETRRINTERVAL  4
#define PSCHED_SET_TIMESLICE  5
#define PSCHED_GET_TIMESLICE  6

#define THREAD_SYNC_SEM_WAIT		1
#define THREAD_SYNC_SEM_POST		2
#define THREAD_SYNC_MUTEX_LOCK		3
#define THREAD_SYNC_MUTEX_UNLOCK	4
#define THREAD_SYNC_MUTEX_INIT		5
#define THREAD_SYNC_SEM_INIT		6
#define THREAD_SYNC_JOIN			7   /* Join a thread and wait for it to terminate */
#define THREAD_SYNC_DETACH			8   /* Detach a thread, making it unjoinable */
#define THREAD_SYNC_TRYJOIN			9   /* Non-blocking join (new) */
#define THREAD_SYNC_SLEEP			10  /* Sleep for a specified number of milliseconds */
#define THREAD_SYNC_YIELD			11  /* Yield the current thread */

 #define CURTHREAD \
 	((curproc && curproc->current_thread) ? curproc->current_thread : NULL)
 
/* Thread safety macros */
#define LOCK_THREAD_SIGNALS(proc)   /* Implement locking mechanism here */
#define UNLOCK_THREAD_SIGNALS(proc) /* Implement unlocking mechanism here */
#define ATOMIC_THREAD_SIG_OP(proc, op) /* Implement atomic operation here */

long proc_thread_status(long tid);
CONTEXT* get_thread_context(struct thread *t);
struct thread* get_idle_thread(struct proc *p);
struct thread* get_main_thread(struct proc *p);
#endif /* PROC_THREAD_H */
