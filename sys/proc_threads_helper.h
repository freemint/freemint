#include "proc_threads.h"

#ifndef PROC_THREADS_HELPER_H
#define PROC_THREADS_HELPER_H

long sys_p_thread_getid(void);

void atomic_thread_state_change(struct thread *t, int new_state);
struct thread *get_highest_priority_thread(struct proc *p);
unsigned long get_system_ticks(void);
void make_process_eligible(struct proc *p);
void boost_thread_priority(struct thread *t, int boost_amount);
void reset_thread_priority(struct thread *t);

long timeout_remaining(TIMEOUT *t);

extern const unsigned char bit_table[256];

int find_highest_priority_bit(unsigned char bitmap);

#endif //PROC_THREADS_HELPER_H