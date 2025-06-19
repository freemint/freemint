/**
 * @file proc_threads_helper.h
 * @brief Threading Utility Functions
 * 
 * Declares helper functions for kernel threading subsystem including:
 *  - Priority bitmap operations
 *  - Thread state management
 *  - Scheduling utilities
 *  - System tick access
 * 
 * Provides fast lookup tables for priority management and
 * atomic thread state transitions critical for scheduler operation.
 * 
 * Author: Medour Mehdi
 * Date: June 2025
 * Version: 1.0
 */

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
int find_highest_priority_bit_word(unsigned short bitmap);
int scale_thread_priority(int priority);

struct thread *get_thread_by_id(struct proc *p, short tid);

#endif //PROC_THREADS_HELPER_H