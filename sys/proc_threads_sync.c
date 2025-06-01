#include "proc_threads.h"

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

// Function to initialize a semaphore
int thread_semaphore_init(struct semaphore *sem, short count) {
    if (!sem || count < 0) {
        return THREAD_EINVAL;
    }
    
    sem->count = count;
    sem->wait_queue = NULL;
    TRACE_THREAD("SEMAPHORE INIT: Count=%d", sem->count);
    return THREAD_SUCCESS;
}

// Function to initialize a mutex
int thread_mutex_init(struct mutex *mutex) {
    if (!mutex) {
        return THREAD_EINVAL;
    }
    
    mutex->locked = 0;
    // mutex->fast_lock = 0;
    mutex->owner = NULL;
    mutex->wait_queue = NULL;
    return THREAD_SUCCESS;
}

// Improved mutex implementation for uniprocessor
int thread_mutex_lock(struct mutex *mutex) {
    if (!mutex) {
        return THREAD_EINVAL;
    }

    struct thread *t = CURTHREAD;
    if (!t) {
        return THREAD_EINVAL;
    }

    // Prevent nested blocking
    if (t->wait_type != WAIT_NONE) {
        TRACE_THREAD("MUTEX LOCK: Thread %d already blocked", t->tid);
        return EDEADLK;
    }

    unsigned short sr = splhigh();
    
    // Try quick acquisition
    if (mutex->locked == 0) {
        TRACE_THREAD("THREAD_MUTEX_LOCK: Fast lock");
        mutex->locked = 1;
        mutex->owner = t;
        spl(sr);
        return THREAD_SUCCESS;
    }
    
    // Check for deadlock (thread trying to lock mutex it already owns)
    if (mutex->owner == t) {
        spl(sr);
        return THREAD_EDEADLK;
    }
    
    TRACE_THREAD("THREAD_MUTEX_LOCK: Slow lock");
    // Add to wait queue with priority
    t->wait_type = WAIT_MUTEX;
    t->wait_obj = mutex;
    
    // Insert in priority order (higher priority first)
    struct thread **pp = &mutex->wait_queue;
    while (*pp && (*pp)->priority > t->priority) {
        pp = &(*pp)->next_wait;
    }
    t->next_wait = *pp;
    *pp = t;
    
    TRACE_THREAD("THREAD_MUTEX_LOCK: Block thread %d", t->tid);
    atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
    spl(sr);
    
    // Yield CPU - will resume here when woken
    schedule();
    
    // When we resume, we should own the lock
    // Double-check that we actually own the lock now
    sr = splhigh();
    if (mutex->owner != t) {
        TRACE_THREAD("THREAD_MUTEX_LOCK: Thread %d woke up but doesn't own mutex!", t->tid);
        mutex->owner = t;  // Force ownership
        mutex->locked = 1;
    }
    spl(sr);
    
    return THREAD_SUCCESS;
}

int thread_mutex_unlock(struct mutex *mutex) {
    if (!mutex) {
        return THREAD_EINVAL;
    }
    
    struct thread *current = CURTHREAD;
    if (!current) {
        return THREAD_EINVAL;
    }
    
    unsigned short sr = splhigh();
    
    // Check if current thread owns the mutex
    if (mutex->owner != current) {
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: Thread %d is not the owner (owner=%p)", 
                    current->tid, mutex->owner ? mutex->owner->tid : -1);
        spl(sr);
        return THREAD_EINVAL; // Not owner
    }
    
    // Clean invalid threads from wait queue first
    struct thread **pp = &mutex->wait_queue;
    while (*pp) {
        if ((*pp)->magic != CTXT_MAGIC || ((*pp)->state & THREAD_STATE_EXITED)) {
            TRACE_THREAD("THREAD_MUTEX_UNLOCK: Removing invalid thread %d from wait queue", (*pp)->tid);
            *pp = (*pp)->next_wait;
        } else {
            pp = &(*pp)->next_wait;
        }
    }
    
    // Wake highest priority waiter
    if (mutex->wait_queue) {
        struct thread *t = mutex->wait_queue;
        mutex->wait_queue = t->next_wait;
        t->next_wait = NULL;
        
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: Waking thread %d", t->tid);
        
        // Transfer lock ownership
        mutex->owner = t;
        t->wait_type = WAIT_NONE;
        t->wait_obj = NULL;
        
        // Mark thread as ready and add to ready queue
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        // Force immediate scheduling if possible
        if (t->priority > current->priority) {
            TRACE_THREAD("THREAD_MUTEX_UNLOCK: Forcing immediate schedule due to priority");
            spl(sr);
            schedule();
            return THREAD_SUCCESS;
        }
    } else {
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: No waiters, unlocking mutex");
        mutex->locked = 0;
        mutex->owner = NULL;
    }
    
    // Restore original priority if boosted
    if (current->priority_boost) {
        TRACE_THREAD("PRI-RESTORE: Thread %d pri %d -> %d",
            current->tid,
            current->priority,
            current->original_priority);
        current->priority = current->original_priority;
        current->priority_boost = 0;
    }
    
    spl(sr);
    return THREAD_SUCCESS;
}

// Semaphore implementation
int thread_semaphore_down(struct semaphore *sem) {
    if (!sem) {
        return THREAD_EINVAL;
    }

    struct thread *t = CURTHREAD;
    if (!t) {
        return THREAD_EINVAL;
    }

    // Prevent nested blocking
    if (t->wait_type != WAIT_NONE) {
        TRACE_THREAD("SEMAPHORE DOWN: Thread %d already blocked", t->tid);
        return EDEADLK;
    }

    unsigned short sr = splhigh();
    
    if (sem->count > 0) {
        sem->count--;
        TRACE_THREAD("SEMAPHORE DOWN: Decremented count to %d", sem->count);
        spl(sr);
        return THREAD_SUCCESS;
    }
    
    TRACE_THREAD("SEMAPHORE DOWN: Count=%d", sem->count);
    
    // Block thread
    t->wait_type = WAIT_SEMAPHORE;
    t->wait_obj = sem;
    
    TRACE_THREAD("SEMAPHORE DOWN: Block thread %d", t->tid);
    
    // Add to wait queue (FIFO for semaphores)
    struct thread **pp = &sem->wait_queue;
    while (*pp) pp = &(*pp)->next_wait;
    *pp = t;
    t->next_wait = NULL;
    
    atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
    spl(sr);
    
    // Schedule another thread
    schedule();

    sr = splhigh();
    if (t->wait_type == WAIT_SEMAPHORE) {
        TRACE_THREAD("SEMAPHORE DOWN: Thread %d woke up but still waiting!", t->tid);
        t->wait_type = WAIT_NONE;
        t->wait_obj = NULL;
    }
    spl(sr);

    // When we resume, the semaphore has been decremented for us
    return THREAD_SUCCESS;
}

int thread_semaphore_up(struct semaphore *sem) {
    if (!sem) {
        return THREAD_EINVAL;
    }
    
    struct thread *current = CURTHREAD;
    if (!current) {
        return THREAD_EINVAL;
    }
    
    unsigned short sr = splhigh();
    
    TRACE_THREAD("SEMAPHORE UP: Count=%d", sem->count);
    
    // Clean invalid threads from wait queue first
    struct thread **pp = &sem->wait_queue;
    while (*pp) {
        if ((*pp)->magic != CTXT_MAGIC || 
            ((*pp)->state & THREAD_STATE_EXITED)) 
        {
            TRACE_THREAD("SEMAPHORE UP: Removing invalid thread %d", (*pp)->tid);
            struct thread *to_remove = *pp;
            *pp = to_remove->next_wait;  // Remove from list
            to_remove->next_wait = NULL;
        } else {
            pp = &(*pp)->next_wait;  // Move to next pointer
        }
    }
// #ifdef DEBUG_THREADS
TRACE_THREAD("SEMAPHORE UP: Wait queue after cleanup:");
struct thread *debug_t = sem->wait_queue;
while (debug_t) {
    TRACE_THREAD("  Thread %d (state %d, magic %lx)", 
                debug_t->tid, debug_t->state, debug_t->magic);
    debug_t = debug_t->next_wait;
}
// #endif    
    if (sem->wait_queue) {
        // Wake first waiter
        struct thread *t = sem->wait_queue;
        sem->wait_queue = t->next_wait;
        t->next_wait = NULL;
        
        TRACE_THREAD("SEMAPHORE UP: Wake thread %d", t->tid);
        t->wait_type = WAIT_NONE;
        t->wait_obj = NULL;
        
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        // Force immediate scheduling if possible
        if (t->priority > current->priority) {
            TRACE_THREAD("SEMAPHORE UP: Forcing immediate schedule due to priority");
            spl(sr);
            schedule();
            return THREAD_SUCCESS;
        }
    } else {
        TRACE_THREAD("SEMAPHORE UP: No waiters, incrementing count");
        sem->count++;
    }
    
    spl(sr);
    return THREAD_SUCCESS;
}

long _cdecl sys_p_threadop(int operator, void *arg) {
    TRACE_THREAD("sys_p_threadop(%d, %p)", operator, arg);
    
    if (!arg) {
        TRACE_THREAD("THREAD_OP: NULL argument");
        return -EINVAL;
    }
    
    switch (operator) {
        case THREAD_OP_SEM_WAIT:
            TRACE_THREAD("THREAD_OP_SEM_WAIT");
            return thread_semaphore_down((struct semaphore *)arg);
            
        case THREAD_OP_SEM_POST:
            TRACE_THREAD("THREAD_OP_SEM_POST");
            return thread_semaphore_up((struct semaphore *)arg);
            
        case THREAD_OP_MUTEX_LOCK:
            TRACE_THREAD("THREAD_OP_MUTEX_LOCK");
            return thread_mutex_lock((struct mutex *)arg);
            
        case THREAD_OP_MUTEX_UNLOCK:
            TRACE_THREAD("THREAD_OP_MUTEX_UNLOCK");
            return thread_mutex_unlock((struct mutex *)arg);
            
        case THREAD_OP_MUTEX_INIT:
            TRACE_THREAD("THREAD_OP_MUTEX_INIT");
            return thread_mutex_init((struct mutex *)arg);
            
        case THREAD_OP_SEM_INIT: {
            struct semaphore *sem = (struct semaphore *)arg;
            TRACE_THREAD("THREAD_OP_SEM_INIT: count=%d", sem->count);
            return thread_semaphore_init(sem, sem->count);
        }            
        
        default:
            TRACE_THREAD("THREAD_OP_UNKNOWN: %d", operator);
            return -EINVAL;
    }
}