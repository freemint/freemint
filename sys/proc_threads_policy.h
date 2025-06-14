#include "proc_threads.h"

#ifndef PROC_THREADS_POLICY_H
#define PROC_THREADS_POLICY_H

long _cdecl proc_thread_set_policy(enum sched_policy policy, short priority, short timeslice);

/* Update thread timeslice accounting during context switches */
void update_thread_timeslice(struct thread *t);

/* Thread scheduling used by system calls */
long proc_thread_get_timeslice(long tid, long *timeslice, long *remaining);
long proc_thread_set_timeslice(long tid, long timeslice);
long proc_thread_get_rrinterval(long tid, long *interval);
long proc_thread_get_schedparam(long tid, long *policy, long *priority);
long proc_thread_set_schedparam(long tid, long policy, long priority);

#endif //PROC_THREADS_POLICY_H