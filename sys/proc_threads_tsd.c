#include "proc_threads_tsd.h"
#include "proc_threads_debug.h"

/**
 * Thread-specific data implementation
 * 
 * This file implements POSIX thread-specific data functionality:
 * - pthread_key_create
 * - pthread_getspecific
 * - pthread_setspecific
 * - pthread_key_delete
 */

/* Process-wide key management is stored in the proc structure */

/**
 * Initialize process-wide thread-specific data
 * 
 * @param p The process to initialize
 * @return 0 on success, error code on failure
 */
int init_proc_tsd(struct proc *p) {
    if (!p || p->magic != CTXT_MAGIC) {
        return EINVAL;
    }

    /* Check if already initialized */
    if (p->thread_keys) {
        return 0;
    }

    p->thread_keys = kmalloc(MAX_THREAD_KEYS * sizeof(thread_key_t));
    if (!p->thread_keys) {
        return ENOMEM;
    }
    
    memset(p->thread_keys, 0, MAX_THREAD_KEYS * sizeof(thread_key_t));
    p->next_key = 0;

    /* Allocate space for thread-specific data pointers */
    p->proc_tsd_data = kmalloc(MAX_THREAD_KEYS * sizeof(void*));
    if (!p->proc_tsd_data) {
        kfree(p->thread_keys);
        p->thread_keys = NULL;
        return ENOMEM;
    }
    
    /* Initialize all values to NULL */
    memset(p->proc_tsd_data, 0, MAX_THREAD_KEYS * sizeof(void*));
    
    TRACE_THREAD("init_proc_tsd: initialized TSD for process %d\n", p->pid);
    return 0;
}

/**
 * Initialize thread-specific data for a new thread
 * 
 * @param t The thread to initialize
 * @return 0 on success, error code on failure
 */
int init_thread_tsd(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC) {
        TRACE_THREAD("init_thread_tsd: invalid thread\n");
        return EINVAL;
    }

    /* Check if already initialized */
    if (t->tsd_data) {
        return 0;
    }

    /* Make sure process TSD is initialized */
    if (t->proc && !t->proc->thread_keys) {
        if (init_proc_tsd(t->proc) != 0) {
            TRACE_THREAD("init_thread_tsd: failed to initialize process TSD\n");
            return ENOMEM;
        }
    }

    /* If this is thread0, use the process TSD data */
    if (t->tid == 0 && t->proc && t->proc->proc_tsd_data) {
        TRACE_THREAD("init_thread_tsd: using process TSD data for thread0\n");
        t->tsd_data = t->proc->proc_tsd_data;
        return 0;
    }

    TRACE_THREAD("init_thread_tsd: initializing thread=%p\n", t);
    /* Allocate space for thread-specific data pointers */
    t->tsd_data = kmalloc(MAX_THREAD_KEYS * sizeof(void*));
    if (!t->tsd_data) {
        TRACE_THREAD("init_thread_tsd: kmalloc failed\n");
        return ENOMEM;
    }
    
    /* Initialize all values to NULL */
    memset(t->tsd_data, 0, MAX_THREAD_KEYS * sizeof(void*));
    
    TRACE_THREAD("init_thread_tsd: initialized thread=%p (tid=%d)\n", t, t->tid);
    return 0;
}

/**
 * Clean up thread-specific data when a thread exits
 * 
 * @param t The thread being cleaned up
 */
void cleanup_thread_tsd(struct thread *t) {
    int i;
    struct proc *p;
    
    if (!t || t->magic != CTXT_MAGIC || !t->tsd_data) {
        return;
    }
    
    p = t->proc;
    if (!p || !p->thread_keys) {
        return;
    }
    
    TRACE_THREAD("cleanup_thread_tsd: cleaning up thread=%p\n", t);

    /* Call destructors for all non-NULL values */
    for (i = 0; i < MAX_THREAD_KEYS; i++) {
        if (p->thread_keys[i].in_use && t->tsd_data[i] && p->thread_keys[i].destructor) {
            p->thread_keys[i].destructor(t->tsd_data[i]);
        }
    }
    
    /* Don't free thread0's TSD data as it's owned by the process */
    if (t->tid != 0) {
        /* Free the TSD data array */
        kfree(t->tsd_data);
        t->tsd_data = NULL;
    }
}

/**
 * Create a thread-specific data key
 * 
 * @param destructor Function to call when thread exits with non-NULL value
 * @return Key index on success, negative error code on failure
 */
long thread_key_create(void (*destructor)(void*)) {
    int i;
    unsigned short sr = splhigh();
    struct proc *p = curproc;
    struct thread *t;

    if (!p) {
        spl(sr);
        return EINVAL;
    }
    
    /* Initialize process TSD if not already done */
    if (!p->thread_keys) {
        TRACE_THREAD("thread_key_create: initializing process TSD\n");
        if (init_proc_tsd(p) != 0) {
            spl(sr);
            return ENOMEM;
        }
    }

    TRACE_THREAD("thread_key_create: destructor=%p\n", destructor);

    /* Initialize TSD for current thread if needed */
    if (p->current_thread) {
        t = p->current_thread;
        if (!t->tsd_data) {
            TRACE_THREAD("thread_key_create: initializing TSD for current thread\n");
            init_thread_tsd(t);
        }
    }

    /* First try to reuse a deleted key */
    for (i = 0; i < p->next_key; i++) {
        if (!p->thread_keys[i].in_use) {
            p->thread_keys[i].in_use = 1;
            p->thread_keys[i].destructor = destructor;
            TRACE_THREAD("thread_key_create: reused key=%d\n", i);
            spl(sr);
            return i;
        }
    }
    
    /* If no deleted keys, create a new one */
    if (p->next_key < MAX_THREAD_KEYS) {
        i = p->next_key++;
        p->thread_keys[i].in_use = 1;
        p->thread_keys[i].destructor = destructor;
        TRACE_THREAD("thread_key_create: created key=%d\n", i);
        spl(sr);
        return i;
    }
    TRACE_THREAD("thread_key_create: no more keys available\n");
    spl(sr);
    return EAGAIN; /* No keys available */
}

/**
 * Delete a thread-specific data key
 * 
 * @param key The key to delete
 * @return 0 on success, negative error code on failure
 */
long thread_key_delete(long key) {
    unsigned short sr;
    struct proc *p = curproc;
    
    if (!p) {
        return EINVAL;
    }
    
    /* Initialize process TSD if not already done */
    if (!p->thread_keys) {
        if (init_proc_tsd(p) != 0) {
            return ENOMEM;
        }
    }
    
    if (key < 0 || key >= p->next_key || !p->thread_keys[key].in_use) {
        return EINVAL;
    }
    
    TRACE_THREAD("thread_key_delete: key=%ld\n", key);
    sr = splhigh();
    p->thread_keys[key].in_use = 0;
    p->thread_keys[key].destructor = NULL;
    spl(sr);
    
    return 0;
}

/**
 * Get thread-specific data for the current thread
 * 
 * @param key The key to get data for
 * @return The data value, or NULL if not set or error
 */
void* thread_getspecific(long key) {
    struct proc *p = curproc;
    struct thread *t;
    TRACE_THREAD("thread_getspecific: key=%ld\n", key);
    
    if (!p || !p->current_thread) {
        return NULL;
    }
    
    /* Initialize process TSD if not already done */
    if (!p->thread_keys) {
        if (init_proc_tsd(p) != 0) {
            return NULL;
        }
    }
    
    t = p->current_thread;
    
    /* Initialize thread TSD if not already done */
    if (!t->tsd_data) {
        if (init_thread_tsd(t) != 0) {
            return NULL;
        }
    }
    
    if (key < 0 || key >= p->next_key || !p->thread_keys[key].in_use) {
        TRACE_THREAD("thread_getspecific: invalid key=%ld\n", key);
        return NULL;
    }
    
    TRACE_THREAD("thread_getspecific: key=%ld, value=%p\n", key, t->tsd_data[key]);
    return t->tsd_data[key];
}

/**
 * Set thread-specific data for the current thread
 * 
 * @param key The key to set data for
 * @param value The value to set
 * @return 0 on success, negative error code on failure
 */
long thread_setspecific(long key, void *value) {
    struct proc *p = curproc;
    struct thread *t;
    TRACE_THREAD("thread_setspecific: key=%ld, value=%p\n", key, value);
    
    if (!p || !p->current_thread) {
        TRACE_THREAD("thread_setspecific: no current thread\n");
        return EINVAL;
    }
    
    /* Initialize process TSD if not already done */
    if (!p->thread_keys) {
        if (init_proc_tsd(p) != 0) {
            return ENOMEM;
        }
    }
    
    t = p->current_thread;
    
    /* Initialize thread TSD if not already done */
    if (!t->tsd_data) {
        if (init_thread_tsd(t) != 0) {
            return ENOMEM;
        }
    }
    
    if (key < 0 || key >= p->next_key || !p->thread_keys[key].in_use) {
        TRACE_THREAD("thread_setspecific: invalid key=%ld\n", key);
        return EINVAL;
    }
    
    TRACE_THREAD("thread_setspecific: key=%ld, value=%p\n", key, value);
    t->tsd_data[key] = value;
    return 0;
}

void cleanup_proc_tsd(struct proc *p) {
    int i;
    
    if (!p || !p->proc_tsd_data || !p->thread_keys) {
        return;
    }
    
    /* Call destructors for all non-NULL values */
    for (i = 0; i < MAX_THREAD_KEYS; i++) {
        if (p->thread_keys[i].in_use && p->proc_tsd_data[i] && p->thread_keys[i].destructor) {
            p->thread_keys[i].destructor(p->proc_tsd_data[i]);
        }
    }
    
    /* Free the TSD data array */
    kfree(p->proc_tsd_data);
    p->proc_tsd_data = NULL;
    
    /* Free the thread keys array */
    if (p->thread_keys) {
        kfree(p->thread_keys);
        p->thread_keys = NULL;
    }
}

// /**
//  * System call handler for thread-specific data operations
//  * 
//  * @param op Operation code (THREAD_TSD_*)
//  * @param arg1 First argument (key or destructor)
//  * @param arg2 Second argument (value)
//  * @return Operation-specific return value
//  */
// long _cdecl sys_p_thread_tsd(long op, long arg1, long arg2) {
//     switch (op) {
//         case THREAD_TSD_CREATE_KEY:
//             return thread_key_create((void (*)(void*))arg1);
            
//         case THREAD_TSD_DELETE_KEY:
//             return thread_key_delete(arg1);
            
//         case THREAD_TSD_GET_SPECIFIC:
//             return (long)thread_getspecific(arg1);
            
//         case THREAD_TSD_SET_SPECIFIC:
//             return thread_setspecific(arg1, (void*)arg2);
            
//         default:
//             return -EINVAL;
//     }
// }