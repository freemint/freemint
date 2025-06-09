#include "proc_threads.h"

#ifndef PROC_THREADS_QUEUE_H
#define PROC_THREADS_QUEUE_H

/* Ready queue management */
void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);
void remove_from_sleep_queue(struct proc *p, struct thread *t);
void remove_thread_from_wait_queues(struct thread *t);
int is_in_ready_queue(struct thread *t);

struct thread *find_highest_priority_thread_in_queue(struct thread *queue, struct thread **prev_highest);

#endif //PROC_THREADS_QUEUE_H