/*
 * FreeMiNT Kernel Threading Implementation
 * ----------------------------------------
 * 
 * POSIX-compliant threading subsystem for FreeMiNT kernel
 * 
 * Provides core threading infrastructure including:
 *  - Thread scheduling and context switching
 *  - Synchronization primitives (mutexes/condvars/semaphores)
 *  - Signal handling
 *  - Thread-specific data
 *  - Cleanup routines
 * 
 * Key components:
 *  proc_threads_sync.[ch]      - Synchronization objects
 *  proc_threads_scheduler.[ch] - Scheduler core
 *  proc_threads_policy.[ch]    - Scheduling policies
 *  proc_threads_signal.[ch]    - Signal handling
 *  proc_threads_tsd.[ch]       - Thread-specific data
 *  proc_threads_cleanup.[ch]   - Cleanup handlers
 *  proc_threads_sleep_yield.[ch] - Sleep/yield mechanisms
 *  proc_threads_syscall.c      - System call interface
 *  proc_threads_queue.[ch]     - Queue management (ready/sleep/wait)
 *  proc_threads.[ch]           - Core thread management
 *  proc_threads_helper.[ch]    - Utility functions
 *  proc_threads_debug.[ch]     - Debugging facilities
 * 
 * Optimized for Motorola 68000-series processors
 * with tight memory constraints (Atari ST/STE/TT/Falcon)
 * 
 * Author: Medour Mehdi
 * Date: June 2025
 * Version: 1.0
 * License: GPLv2
 */

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

#define MAX_THREAD_PRIORITY 16 /* Maximum thread priority (scaled from POSIX 0-99 range) */
/* 
 * Thread priority scaling:
 * - User-facing API accepts priorities in the standard POSIX range (0-99)
 * - Internally, priorities are scaled to 0-16 range when set via syscalls
 * - This allows efficient bitmap operations while maintaining POSIX compatibility
 * - Scaling uses fast multiply-shift: (priority * 10923) >> 16 ≈ (priority * 16) / 99
 */
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
#define THREAD_CTRL_CANCEL   1   /* Cancel a thread */
#define THREAD_CTRL_STATUS   4   /* Get thread status */
#define THREAD_CTRL_GETID    5	/* Get thread ID */
#define THREAD_CTRL_SETCANCELSTATE 6
#define THREAD_CTRL_SETCANCELTYPE  7
#define THREAD_CTRL_TESTCANCEL     8

/* Wait types as bitfields */
#define WAIT_NONE       0x0000  /* Not waiting */
#define WAIT_JOIN       0x0001  /* Waiting for thread to exit */
#define WAIT_MUTEX      0x0002  /* Waiting for mutex */
#define WAIT_CONDVAR    0x0004  /* Waiting for condition variable */
#define WAIT_SEMAPHORE  0x0008  /* Waiting for semaphore */
#define WAIT_SIGNAL     0x0010  /* Waiting for signal */
#define WAIT_IO         0x0020  /* Waiting for I/O */
#define WAIT_SLEEP      0x0040  /* Sleeping */
#define WAIT_OTHER      0x0080  /* Other wait reason */

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

/* Condition variable system call constants */
#define THREAD_SYNC_COND_INIT       12
#define THREAD_SYNC_COND_DESTROY    13
#define THREAD_SYNC_COND_WAIT       14
#define THREAD_SYNC_COND_TIMEDWAIT  15
#define THREAD_SYNC_COND_SIGNAL     16
#define THREAD_SYNC_COND_BROADCAST  17

/* Cleanup operation constants for syscalls */
#define THREAD_SYNC_CLEANUP_PUSH    18
#define THREAD_SYNC_CLEANUP_POP     19
#define THREAD_SYNC_CLEANUP_GET     20

/* Thread-specific data operations */
#define THREAD_TSD_CREATE_KEY    21   /* Create a new key */
#define THREAD_TSD_DELETE_KEY    22   /* Delete a key */
#define THREAD_TSD_GET_SPECIFIC  23   /* Get thread-specific data */
#define THREAD_TSD_SET_SPECIFIC  24   /* Set thread-specific data */

/* Thread cancellation constants */
#define PTHREAD_CANCEL_ENABLE       0   /* Enable cancellation */
#define PTHREAD_CANCEL_DISABLE      1   /* Disable cancellation */
#define PTHREAD_CANCEL_DEFERRED     0   /* Deferred cancellation (at cancellation points) */
#define PTHREAD_CANCEL_ASYNCHRONOUS 1   /* Asynchronous cancellation (immediate) */
#define PTHREAD_CANCELED           ((void *)-1)  /* Return value for canceled threads */

 #define CURTHREAD \
 	((curproc && curproc->current_thread) ? curproc->current_thread : NULL)

long proc_thread_status(long tid);
CONTEXT* get_thread_context(struct thread *t);
struct thread* get_idle_thread(struct proc *p);
struct thread* get_main_thread(struct proc *p);
#endif /* PROC_THREAD_H */
