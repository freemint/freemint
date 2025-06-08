#include "proc_threads.h"

/*
 * Thread Scheduling Implementation (POSIX-compliant)
 *
 * Priority Range:
 * - Thread priorities range from 0 to 99 (MAX_THREAD_PRIORITY)
 * - Higher numerical values represent higher priorities
 * - This is opposite to process priorities where lower values are higher priority
 *
 * Priority Handling:
 * - SCHED_FIFO threads maintain their position in the ready queue until they
 *   yield, block, or are preempted by higher priority threads.
 * - When a SCHED_FIFO thread's priority is raised, it moves to the end of the
 *   list for its new priority.
 * - When a SCHED_FIFO thread's priority is lowered, it moves to the front of
 *   the list for its new priority.
 *
 * Time Slicing:
 * - SCHED_RR threads use time slicing (thread_rr_timeslice).
 * - SCHED_FIFO threads don't use time slicing.
 * - SCHED_OTHER threads use the default time slice (thread_default_timeslice).
 *
 * Priority Boosting:
 * - Threads waking from sleep get a temporary priority boost to prevent starvation.
 * - The boost is removed after the thread has run for a while.
 * - Priority inheritance is implemented for mutexes to prevent priority inversion.
 *
 * Process Integration:
 * - Thread scheduling parameters are derived from the parent process's time_slice
 * - Thread preemption occurs independently of process scheduling
 * - The make_process_eligible() function ensures the thread-handling process
 *   gets scheduled frequently
 */

#ifndef PROC_THREADS_SCHEDULER_H
#define PROC_THREADS_SCHEDULER_H

void thread_preempt_handler(PROC *p, long arg);
void proc_thread_schedule(void);
void proc_thread_exit(void *retval);

void thread_timer_start(struct proc *p, int thread_id);
void thread_timer_stop(PROC *p);

void reschedule_preemption_timer(PROC *p, long arg);

/* Thread exit helper functions */
struct thread *find_next_thread_to_run(struct proc *p);
void handle_thread_joining(struct thread *current, void *retval);
void cancel_thread_timeouts(struct proc *p, struct thread *t);
void cleanup_thread_resources(struct proc *p, struct thread *t, int tid);

#endif //PROC_THREADS_SCHEDULER_H
