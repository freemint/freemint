#include "proc_threads.h"

#ifndef _PROC_THREADS_TSD_H
#define _PROC_THREADS_TSD_H

/* Maximum number of thread-specific data keys per process */
#define MAX_THREAD_KEYS 32

/* Sparse TSD entry for memory efficiency */
struct tsd_entry {
    long key;
    void *value;
    struct tsd_entry *next;
};

/* Thread-specific data operations */
#define THREAD_TSD_CREATE_KEY    1   /* Create a new key */
#define THREAD_TSD_DELETE_KEY    2   /* Delete a key */
#define THREAD_TSD_GET_SPECIFIC  3   /* Get thread-specific data */
#define THREAD_TSD_SET_SPECIFIC  4   /* Set thread-specific data */

/* Function declarations */
int init_proc_tsd(struct proc *p);
int init_thread_tsd(struct thread *t);
void cleanup_thread_tsd(struct thread *t);
long thread_key_create(void (*destructor)(void*));
long thread_key_delete(long key);
void* thread_getspecific(long key);
long thread_setspecific(long key, void *value);
void cleanup_proc_tsd(struct proc *p);
void run_tsd_destructors(struct thread *t);

#endif /* _PROC_THREADS_TSD_H */