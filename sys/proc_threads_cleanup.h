/**
 * @file proc_threads_cleanup.h
 * @brief Thread Cleanup Handlers Interface
 * 
 * Declares cleanup stack management for pthread_cleanup_push/pop functionality.
 * Defines structures for handler registration and execution during thread exit.
 * 
 * @author Medour Mehdi
 * @date June 2025
 * @version 1.0
 */

#include "proc_threads.h"

#ifndef PROC_THREADS_CLEANUP_H
#define PROC_THREADS_CLEANUP_H

/* Thread cleanup handler functions */
void run_cleanup_handlers(struct thread *t);

long thread_cleanup_push(void (*routine)(void*), void *arg);
long thread_cleanup_pop(void (**routine_ptr)(void*), void **arg_ptr);

int init_thread_cleanup(struct thread *t);
void cleanup_thread_handlers(struct thread *t);

/**
 * Cleanup handler info structure (for userspace interface)
 */
struct cleanup_info {
    void (*routine)(void*);         /* Cleanup function */  
    void *arg;                      /* Argument to cleanup function */
};
long get_cleanup_handlers(struct thread *t, struct cleanup_info *handlers, int max_handlers);
#endif //PROC_THREADS_CLEANUP_H