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
    
    // Priority inheritance - boost the priority of the mutex owner
    if (mutex->owner && mutex->owner->priority < t->priority) {
        TRACE_THREAD("PRI-INHERIT: Thread %d (pri %d) -> owner %d (pri %d)",
                    t->tid, t->priority,
                    mutex->owner->tid, mutex->owner->priority);
        
        // Save original priority if not already boosted
        if (!mutex->owner->priority_boost) {
            mutex->owner->original_priority = mutex->owner->priority;
            mutex->owner->priority_boost = 1;
        }
        
        // Boost owner's priority
        mutex->owner->priority = t->priority;
        
        // Reinsert owner in ready queue if needed
        if (mutex->owner->state == THREAD_STATE_READY) {
            remove_from_ready_queue(mutex->owner);
            add_to_ready_queue(mutex->owner);
        }
    }
    
    spl(sr);
    
    // Yield CPU - will resume here when woken
    schedule();
    
    // When we resume, check if we were sleeping
    sr = splhigh();
    if (t->wakeup_time > 0) {
        TRACE_THREAD("THREAD_MUTEX_LOCK: Thread %d was sleeping, clearing sleep state", t->tid);
        t->wakeup_time = 0;
        remove_from_sleep_queue(t->proc, t);
    }
    
    // When we resume, we should own the lock
    // Double-check that we actually own the lock now
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
    
    // If there are waiters, wake the highest priority one
    if (mutex->wait_queue) {
        // Find highest priority valid thread
        struct thread *t = mutex->wait_queue;
        struct thread *highest = NULL;
        struct thread *prev_highest = NULL;
        struct thread *prev = NULL;
        int highest_priority = -1;
        
        // First pass: find highest priority valid thread
        while (t) {
            if (t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED) && 
                t->priority > highest_priority) {
                highest = t;
                prev_highest = prev;
                highest_priority = t->priority;
            }
            prev = t;
            t = t->next_wait;
        }
        
        if (highest) {
            // Remove from wait queue
            if (prev_highest) {
                prev_highest->next_wait = highest->next_wait;
            } else {
                mutex->wait_queue = highest->next_wait;
            }
            highest->next_wait = NULL;
            
            TRACE_THREAD("THREAD_MUTEX_UNLOCK: Waking thread %d (priority %d)", 
                        highest->tid, highest->priority);
            
            // Transfer lock ownership
            mutex->owner = highest;
            
            // Clear wait state
            highest->wait_type = WAIT_NONE;
            highest->wait_obj = NULL;
            
            // Remove from sleep queue if needed
            if (highest->wakeup_time > 0) {
                remove_from_sleep_queue(highest->proc, highest);
                highest->wakeup_time = 0;
            }
            
            // Mark as ready and add to ready queue
            atomic_thread_state_change(highest, THREAD_STATE_READY);
            add_to_ready_queue(highest);
            
            // Restore original priority if boosted
            if (current->priority_boost) {
                TRACE_THREAD("PRI-RESTORE: Thread %d pri %d -> %d",
                            current->tid, current->priority, current->original_priority);
                current->priority = current->original_priority;
                current->priority_boost = 0;
            }
            
            // Force immediate scheduling if higher priority
            if (highest->priority > current->priority) {
                TRACE_THREAD("THREAD_MUTEX_UNLOCK: Forcing immediate schedule due to priority");
                spl(sr);
                schedule();
                return THREAD_SUCCESS;
            }
        } else {
            // No valid waiters, release the mutex
            TRACE_THREAD("THREAD_MUTEX_UNLOCK: No valid waiters, releasing mutex");
            mutex->locked = 0;
            mutex->owner = NULL;
            
            // Restore original priority if boosted
            if (current->priority_boost) {
                TRACE_THREAD("PRI-RESTORE: Thread %d pri %d -> %d",
                            current->tid, current->priority, current->original_priority);
                current->priority = current->original_priority;
                current->priority_boost = 0;
            }
        }
    } else {
        // No waiters, release the mutex
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: No waiters, releasing mutex");
        mutex->locked = 0;
        mutex->owner = NULL;
        
        // Restore original priority if boosted
        if (current->priority_boost) {
            TRACE_THREAD("PRI-RESTORE: Thread %d pri %d -> %d",
                        current->tid, current->priority, current->original_priority);
            current->priority = current->original_priority;
            current->priority_boost = 0;
        }
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
    // When we resume, check if we were sleeping
    sr = splhigh();
    if (t->wait_type == WAIT_SLEEP) {
        TRACE_THREAD("SEMAPHORE DOWN: Thread %d was sleeping, clearing sleep state", t->tid);
        t->wait_type = WAIT_NONE;
        t->wakeup_time = 0;
    }
    
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
    
    // If no waiters, just increment count and return
    if (!sem->wait_queue) {
        sem->count++;
        TRACE_THREAD("SEMAPHORE UP: No waiters, incremented count to %d", sem->count);
        spl(sr);
        return THREAD_SUCCESS;
    }
    
    // Find first valid waiter
    struct thread *t = sem->wait_queue;
    struct thread *prev = NULL;
    
    while (t && (t->magic != CTXT_MAGIC || (t->state & THREAD_STATE_EXITED))) {
        prev = t;
        t = t->next_wait;
    }
    
    if (!t) {
        // No valid waiters, increment count
        sem->count++;
        TRACE_THREAD("SEMAPHORE UP: No valid waiters, incremented count to %d", sem->count);
        spl(sr);
        return THREAD_SUCCESS;
    }
    
    // Remove from wait queue
    if (prev) {
        prev->next_wait = t->next_wait;
    } else {
        sem->wait_queue = t->next_wait;
    }
    t->next_wait = NULL;
    
    // Wake up thread
    t->wait_type = WAIT_NONE;
    t->wait_obj = NULL;
    
    // Remove from sleep queue if needed
    if (t->wakeup_time > 0) {
        t->wakeup_time = 0;
        remove_from_sleep_queue(t->proc, t);
    }
    
    // Mark as ready and add to ready queue
    atomic_thread_state_change(t, THREAD_STATE_READY);
    add_to_ready_queue(t);
    
    // Force immediate scheduling if higher priority
    if (t->priority > current->priority) {
        TRACE_THREAD("SEMAPHORE UP: Forcing immediate schedule due to priority");
        spl(sr);
        schedule();
        return THREAD_SUCCESS;
    }
    
    spl(sr);
    return THREAD_SUCCESS;
}

/**
 * Clean up thread synchronization states when a process terminates
 * 
 * This function should be called from the terminate() function in k_exit.c
 * to ensure that all threads waiting on synchronization objects are properly
 * unblocked when a process terminates.
 * 
 * @param p The process being terminated
 */
void cleanup_thread_sync_states(struct proc *p)
{
    if (!p) return;
    
    TRACE_THREAD("CLEANUP: Cleaning up thread sync states for process %d", p->pid);
    
    unsigned short sr = splhigh();
    
    // Clean up any threads waiting on mutexes or semaphores
    struct thread *t;
    for (t = p->threads; t; t = t->next) {
        if (t->magic == CTXT_MAGIC && t->wait_type != WAIT_NONE) {
            TRACE_THREAD("CLEANUP: Clearing wait state for thread %d (wait_type=%d)",
                        t->tid, t->wait_type);
            
            // Clear wait state
            t->wait_type = WAIT_NONE;
            t->wait_obj = NULL;
            t->next_wait = NULL;
        }
    }
    
    // For each process in the system
    struct proc *other_proc;
    for (other_proc = proclist; other_proc; other_proc = other_proc->gl_next) {
        // Skip the terminating process
        if (other_proc == p) continue;
        
        // Check each thread in the process
        for (t = other_proc->threads; t; t = t->next) {
            if (t->magic != CTXT_MAGIC) continue;
            
            // If thread is waiting on a mutex owned by a thread in the terminating process
            if (t->wait_type == WAIT_MUTEX && t->wait_obj) {
                struct mutex *mutex = (struct mutex *)t->wait_obj;
                
                // If mutex owner is from terminating process
                if (mutex->owner && mutex->owner->proc == p) {
                    TRACE_THREAD("CLEANUP: Thread %d in process %d waiting on mutex owned by terminating process",
                                t->tid, other_proc->pid);
                    
                    // Clear mutex owner
                    mutex->owner = NULL;
                    mutex->locked = 0;
                    
                    // Unblock thread
                    t->wait_type = WAIT_NONE;
                    t->wait_obj = NULL;
                    t->next_wait = NULL;
                    
                    // Wake up thread
                    atomic_thread_state_change(t, THREAD_STATE_READY);
                    add_to_ready_queue(t);
                }
            }
        }
    }
    
    spl(sr);
    
    TRACE_THREAD("CLEANUP: Finished cleaning up thread sync states for process %d", p->pid);
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