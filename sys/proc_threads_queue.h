/**
 * @file proc_threads_queue.h
 * @brief Thread Queue Management Interface
 * 
 * Declares queue operations for thread scheduling and synchronization.
 * 
 * Manages:
 *  - Ready queues (priority-based scheduling)
 *  - Sleep queues (timed waits)
 *  - Wait queues (synchronization objects)
 * 
 * Provides atomic queue operations critical for scheduler integrity
 * and POSIX-compliant thread ordering.
 * 
 * Author: Medour Mehdi
 * Date: June 2025
 * Version: 1.0
 */

#include "proc_threads.h"

#ifndef PROC_THREADS_QUEUE_H
#define PROC_THREADS_QUEUE_H

/* Ready queue management */
void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);
void remove_from_sleep_queue(struct proc *p, struct thread *t);
void remove_thread_from_wait_queues(struct thread *t);
void remove_thread_from_specific_wait_queue(struct thread *t, int wait_type_mask);
int is_in_ready_queue(struct thread *t);

struct thread *find_highest_priority_thread_in_queue(struct thread *queue, struct thread **prev_highest);

#endif //PROC_THREADS_QUEUE_H