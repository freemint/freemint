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

/* Helper functions for sparse TSD management */

/**
 * Find a TSD entry for a given key in the sparse list
 * 
 * @param head Head of the TSD entry list
 * @param key The key to search for
 * @return Pointer to entry if found, NULL otherwise
 */
static struct tsd_entry* find_tsd_entry(struct tsd_entry *head, long key) {
    struct tsd_entry *entry = head;
    while (entry) {
        if (entry->key == key) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * Set a value in the sparse TSD list
 * 
 * @param head_ptr Pointer to head of the TSD entry list
 * @param key The key to set
 * @param value The value to set (NULL to remove entry)
 * @return 0 on success, error code on failure
 */
static int set_tsd_entry(struct tsd_entry **head_ptr, long key, void *value) {
    struct tsd_entry *entry = find_tsd_entry(*head_ptr, key);
    
    if (value == NULL) {
        /* Remove entry if value is NULL */
        if (entry) {
            if (entry == *head_ptr) {
                *head_ptr = entry->next;
            } else {
                struct tsd_entry *prev = *head_ptr;
                while (prev && prev->next != entry) {
                    prev = prev->next;
                }
                if (prev) {
                    prev->next = entry->next;
                }
            }
            kfree(entry);
        }
        return 0;
    }
    
    if (!entry) {
        /* Create new entry */
        entry = kmalloc(sizeof(struct tsd_entry));
        if (!entry) {
            return ENOMEM;
        }
        entry->key = key;
        entry->next = *head_ptr;
        *head_ptr = entry;
    }
    
    entry->value = value;
    return 0;
}

/**
 * Free all entries in a sparse TSD list and call destructors
 * 
 * @param head Head of the TSD entry list
 * 
 */
static void free_tsd_entries(struct tsd_entry *head) {
    struct tsd_entry *entry = head;
    while (entry) {
        struct tsd_entry *next = entry->next;
        kfree(entry);
        entry = next;
    }
}

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

    /* Initialize sparse TSD data to NULL */
    p->proc_tsd_data = NULL;
    
    return 0;
}

/**
 * Initialize thread-specific data for a new thread
 * 
 * @param t The thread to initialize
 * @return 0 on success, error code on failure
 */
int init_thread_tsd(struct thread *t) {
    if (!t) {
        TRACE_THREAD("init_thread_tsd: invalid thread\n");
        return EINVAL;
    }

    TRACE_THREAD("init_thread_tsd: initializing thread=%p\n", t);
    
    /* Initialize sparse TSD data to NULL */
    if (t->tid == 0 && t->proc) {
        /* Thread0 uses process TSD data */
        t->tsd_data = (void*)&t->proc->proc_tsd_data;
    } else {
        /* Other threads get their own sparse TSD */
        t->tsd_data = NULL;
    }
    
    TRACE_THREAD("init_thread_tsd: initialized thread=%p\n", t);
    
    return 0;
}

/**
 * Clean up thread-specific data when a thread exits
 * 
 * @param t The thread being cleaned up
 */
void cleanup_thread_tsd(struct thread *t) {
    struct proc *p;
    struct tsd_entry *tsd_head;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return;
    }
    
    p = t->proc;
    if (!p || !p->thread_keys) {
        return;
    }
    
    TRACE_THREAD("cleanup_thread_tsd: cleaning up thread=%p\n", t);

    if (t->tid == 0) {
        /* Thread0 uses process TSD data */
        tsd_head = (struct tsd_entry*)p->proc_tsd_data;
    } else {
        /* Other threads have their own TSD */
        tsd_head = (struct tsd_entry*)t->tsd_data;
    }
    
    /* Free TSD entries */
    if (tsd_head) {
        free_tsd_entries(tsd_head);
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
    struct tsd_entry *tsd_head;
    struct tsd_entry *entry;
    
    TRACE_THREAD("thread_getspecific: key=%ld\n", key);
    if (!p || !p->current_thread || !p->thread_keys) {
        return NULL;
    }
    
    t = p->current_thread;
    
    if (key < 0 || key >= p->next_key || !p->thread_keys[key].in_use) {
        TRACE_THREAD("thread_getspecific: invalid key=%ld\n", key);
        return NULL;
    }
    
    tsd_head = (t->tid == 0) ? (struct tsd_entry*)p->proc_tsd_data : (struct tsd_entry*)t->tsd_data;
    entry = find_tsd_entry(tsd_head, key);
    
    TRACE_THREAD("thread_getspecific: key=%ld, value=%p\n", key, entry ? entry->value : NULL);
    return entry ? entry->value : NULL;
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
    struct tsd_entry **tsd_head_ptr;
    int result;
    
    TRACE_THREAD("thread_setspecific: key=%ld, value=%p\n", key, value);
    if (!p || !p->current_thread || !p->thread_keys) {
        TRACE_THREAD("thread_setspecific: no current thread\n");
        return EINVAL;
    }
    
    t = p->current_thread;
    
    if (key < 0 || key >= p->next_key || !p->thread_keys[key].in_use) {
        TRACE_THREAD("thread_setspecific: invalid key=%ld\n", key);
        return EINVAL;
    }
    
    tsd_head_ptr = (t->tid == 0) ? (struct tsd_entry**)&p->proc_tsd_data : (struct tsd_entry**)&t->tsd_data;
    result = set_tsd_entry(tsd_head_ptr, key, value);
    
    if (result != 0) {
        return result;
    }
    
    TRACE_THREAD("thread_setspecific: key=%ld, value=%p\n", key, value);
    return 0;
}

void cleanup_proc_tsd(struct proc *p) {
    if (!p || !p->thread_keys) {
        return;
    }
    
    /* Free process TSD entries and call destructors */
    if (p->proc_tsd_data) {
        free_tsd_entries((struct tsd_entry*)p->proc_tsd_data);
        p->proc_tsd_data = NULL;
    }
    
    /* Free the thread keys array */
    if (p->thread_keys) {
        kfree(p->thread_keys);
        p->thread_keys = NULL;
    }
}

void run_tsd_destructors(struct thread *t) {
    // This runs in user space!
    struct proc *p;
    struct tsd_entry *tsd_head;
    struct tsd_entry *entry;
    int iterations = 0;
    const int MAX_DESTRUCTOR_ITERATIONS = 4; // POSIX allows up to 4 iterations
    
    if (!t || t->magic != CTXT_MAGIC) {
        return;
    }
    
    p = t->proc;
    if (!p || !p->thread_keys) {
        return;
    }
    
    TRACE_THREAD("run_tsd_destructors: running destructors for thread=%p\n", t);
    
    // Get the appropriate TSD head for this thread
    if (t->tid == 0) {
        /* Thread0 uses process TSD data */
        tsd_head = (struct tsd_entry*)p->proc_tsd_data;
    } else {
        /* Other threads have their own TSD */
        tsd_head = (struct tsd_entry*)t->tsd_data;
    }
    
    // POSIX allows destructors to set new values, so we need to iterate
    // until no more destructors are called or we reach the maximum iterations
    while (iterations < MAX_DESTRUCTOR_ITERATIONS) {
        int destructors_called = 0;
        
        // Walk through all TSD entries for this thread
        entry = tsd_head;
        while (entry) {
            // Check if this key has a destructor and a non-NULL value
            if (entry->key >= 0 && entry->key < p->next_key && 
                p->thread_keys[entry->key].in_use &&
                p->thread_keys[entry->key].destructor &&
                entry->value != NULL) {
                
                void (*destructor)(void*) = p->thread_keys[entry->key].destructor;
                void *value = entry->value;
                
                TRACE_THREAD("run_tsd_destructors: calling destructor for key=%ld, value=%p\n", 
                           entry->key, value);
                
                // Clear the value before calling destructor to prevent infinite recursion
                entry->value = NULL;
                
                // Call the destructor - this runs in user space
                destructor(value);
                
                destructors_called++;
            }
            entry = entry->next;
        }
        
        // If no destructors were called this iteration, we're done
        if (destructors_called == 0) {
            break;
        }
        
        iterations++;
        TRACE_THREAD("run_tsd_destructors: iteration %d completed, %d destructors called\n", 
                   iterations, destructors_called);
    }
    
    // After all destructor iterations, clean up any remaining TSD entries
    // that still have non-NULL values (these are implementation-defined behavior)
    entry = tsd_head;
    while (entry) {
        if (entry->value != NULL) {
            TRACE_THREAD("run_tsd_destructors: warning - key=%ld still has non-NULL value after destructors\n", 
                       entry->key);
            // We don't call destructors again, just note the issue
        }
        entry = entry->next;
    }
    
    TRACE_THREAD("run_tsd_destructors: completed %d iterations for thread=%p\n", 
               iterations, t);
}