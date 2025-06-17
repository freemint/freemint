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