#include "proc_threads.h"

#ifndef PROC_THREADS_SLEEP_H
#define PROC_THREADS_SLEEP_H

long proc_thread_yield(void);
long proc_thread_sleep(long ms);

void check_and_wake_sleeping_threads(struct proc *p);
int wake_threads_by_time(struct proc *p, unsigned long current_time);
void proc_thread_sleep_wakeup_handler(PROC *p, long arg);

#endif /* PROC_THREADS_SLEEP_H */