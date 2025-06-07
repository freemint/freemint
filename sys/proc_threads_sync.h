#include "proc_threads.h"

#ifndef PROC_THREADS_SYNC_H
#define PROC_THREADS_SYNC_H

struct semaphore {
    volatile unsigned short count;
    struct thread *wait_queue;
};

struct mutex {
    volatile short locked;
    struct thread *owner;
    struct thread *wait_queue;
};

long proc_thread_join(long tid, void **retval);
long proc_thread_tryjoin(long tid, void **retval);
long proc_thread_detach(long tid);

/* Function to clean up thread synchronization states */
void cleanup_thread_sync_states(struct proc *p);

// Function to unlock a mutex
int thread_mutex_unlock(struct mutex *mutex);
// Function to lock a mutex
int thread_mutex_lock(struct mutex *mutex);
// Function to initialize a mutex
int thread_mutex_init(struct mutex *mutex);
// Function to up a semaphore
int thread_semaphore_up(struct semaphore *sem);
// Function to down a semaphore
int thread_semaphore_down(struct semaphore *sem);
// Function to initialize a semaphore
int thread_semaphore_init(struct semaphore *sem, short count);

#endif /* PROC_THREADS_SYNC_H */