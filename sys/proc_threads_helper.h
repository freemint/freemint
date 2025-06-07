#include "proc_threads.h"

#ifndef PROC_THREADS_HELPER_H
#define PROC_THREADS_HELPER_H

long sys_p_thread_getid(void);

void atomic_thread_state_change(struct thread *t, int new_state);
struct thread *get_highest_priority_thread(struct proc *p);
unsigned long get_system_ticks(void);
void make_process_eligible(struct proc *p);

long timeout_remaining(TIMEOUT *t);

#endif //PROC_THREADS_HELPER_H