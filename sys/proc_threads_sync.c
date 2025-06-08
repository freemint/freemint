#include "proc_threads_sync.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"

/**
 * Detach a thread - mark it as not joinable
 * 
 * @param tid Thread ID to detach
 * @return 0 on success, error code on failure
 */
long proc_thread_detach(long tid)
{
    struct proc *p = curproc;
    struct thread *target = NULL;
    unsigned short sr;
    
    if (!p)
        return -EINVAL;
    
    // Find target thread
    for (target = p->threads; target; target = target->next) {
        TRACE_THREAD("DETACH: Checking thread %d", target->tid);
        if (target->tid == tid){
            break;
        }
    }
    
    if (!target){
        TRACE_THREAD("DETACH: No such thread %d", tid);
        return -ESRCH;  // No such thread
    }

    sr = splhigh();
    
    // Check if thread is already joined
    if (target->joined) {
        TRACE_THREAD("DETACH: Thread %d already joined", target->tid);
        spl(sr);
        return -EINVAL;  // Thread already joined
    }
    
    // Check if thread is already detached
    if (target->detached) {
        TRACE_THREAD("DETACH: Thread %d already detached", target->tid);
        spl(sr);
        return 0;  // Already detached, not an error
    }
    
    // Mark thread as detached
    target->detached = 1;
    
    // If thread already exited, free its resources
    if (target->state & THREAD_STATE_EXITED) {
        TRACE_THREAD("DETACH: Thread %d already exited, freeing resources", target->tid);
        
        // Handle any thread waiting to join this one
        handle_thread_joining(target, NULL);
        
        // Free resources
        cleanup_thread_resources(p, target, target->tid);
    }
    
    spl(sr);
    return 0;
}

/**
 * Join a thread - wait for it to terminate
 * 
 * @param tid Thread ID to join
 * @param retval Pointer to store the thread's return value
 * @return 0 on success, error code on failure
 */
long proc_thread_join(long tid, void **retval)
{
    struct proc *p = curproc;
    struct thread *current, *target = NULL;
    unsigned short sr;
    
    if (!p || !p->current_thread)
        return EINVAL;
    
    current = p->current_thread;

    TRACE_THREAD("JOIN: proc_thread_join called for tid=%ld, retval=%p", tid, retval);

    // Cannot join self - would deadlock
    if (current->tid == tid){
        TRACE_THREAD("JOIN: Cannot join self (would deadlock)");
        return EDEADLK;
    }

    for (struct thread *t = p->threads; t; t = t->next) {
        TRACE_THREAD("  Thread %d: state=%d, magic=%lx, detached=%d, joined=%d",
                    t->tid, t->state, t->magic, t->detached, t->joined);
    }

    // Find target thread
    for (target = p->threads; target; target = target->next) {
        if (target->tid == tid){
            break;
        }
    }
    
    if (!target){
        TRACE_THREAD("JOIN: Thread %ld not found", tid);
        return ESRCH;  // Return positive error code
    }
    
    // Check if thread is joinable
    if (target->detached){
        TRACE_THREAD("JOIN: Thread %ld is detached, cannot join", tid);
        return EINVAL;  // Return positive error code
    }
    
    // Check if thread is already joined
    if (target->joined){
        TRACE_THREAD("JOIN: Thread %ld is already joined", tid);
        return EINVAL;  // Thread already joined
    }
    
    // Check if another thread is already joining this thread
    if (target->joiner && target->joiner != current && target->joiner->magic == CTXT_MAGIC) {
        TRACE_THREAD("JOIN: Thread %ld is already being joined by thread %d", 
                    tid, target->joiner->tid);
        return EINVAL;  // Thread is already being joined by another thread
    }

    sr = splhigh();
    
    // Check if thread already exited
    if (target->state & THREAD_STATE_EXITED) {
        // Thread already exited, get return value and return immediately
        if (retval)
            *retval = target->retval;
        
        // Mark as joined so resources can be freed
        target->joined = 1;
        
        // Free resources now
        cleanup_thread_resources(p, target, tid);
        
        // Clear any wait state on the current thread
        current->wait_type = WAIT_NONE;
        current->wait_obj = NULL;
        current->join_retval = NULL;
        
        spl(sr);
        return 0;
    }
    
    // Mark target as being joined by current thread
    target->joiner = current;

    // Mark current thread as waiting for join
    current->wait_type = WAIT_JOIN;
    current->wait_obj = target;
    current->join_retval = retval;  // Store the pointer where to put the return value

    TRACE_THREAD("JOIN: Thread %d waiting for thread %d to exit",
                current->tid, target->tid);
    
    // Block the current thread
    atomic_thread_state_change(current, THREAD_STATE_BLOCKED);
    
    if (save_context(get_thread_context(current)) == 0) {
        // First time through - going to sleep
        spl(sr);
        
        // Schedule another thread
        proc_thread_schedule();
        
        // Should never reach here
        TRACE_THREAD("JOIN: ERROR - Returned from proc_thread_schedule() in join path!");
        return -1;
    }
    
    // Second time through - waking up after target thread exited
    sr = splhigh();
    
    // Ensure we're in the RUNNING state
    if (current->state != THREAD_STATE_RUNNING) {
        TRACE_THREAD("JOIN: Thread %d not in RUNNING state after wake, fixing", current->tid);
        atomic_thread_state_change(current, THREAD_STATE_RUNNING);
    }

    // Check if we're still waiting for the target thread
    if (current->wait_type == WAIT_JOIN && current->wait_obj == target) {
        // Check if target has exited
        if (!(target->state & THREAD_STATE_EXITED)) {
            TRACE_THREAD("JOIN: Thread %d woken up but target thread %ld hasn't exited yet", 
                        current->tid, tid);
            
            // Go back to waiting
            atomic_thread_state_change(current, THREAD_STATE_BLOCKED);
            
            // Schedule another thread and continue waiting
            spl(sr);
            proc_thread_schedule();
            return 0;  // Will never reach here
        }
    } else {
        // We're no longer waiting for this thread, which means it has been handled
        TRACE_THREAD("JOIN: Thread %d no longer waiting for thread %ld", current->tid, tid);
    }

    // CRITICAL FIX: Make sure target thread still exists and is valid
    int target_found = 0;
    for (struct thread *t = p->threads; t; t = t->next) {
        if (t == target) {
            target_found = 1;
            break;
        }
    }
    
    if (!target_found) {
        // Target thread no longer exists in the thread list
        // This means it has been properly joined and freed
        TRACE_THREAD("JOIN: Target thread %ld no longer exists, join successful", tid);
        
        // Clear wait state
        current->wait_type = WAIT_NONE;
        current->wait_obj = NULL;
        
        spl(sr);
        return 0;
    }

    // Get return value if requested
    if (retval && target && target->magic == CTXT_MAGIC)
        *retval = target->retval;
    
    // Clear wait state
    current->wait_type = WAIT_NONE;
    current->wait_obj = NULL;
    
    TRACE_THREAD("JOIN: Thread %d successfully joined thread %d", current->tid, tid);
    
    spl(sr);
    return 0;
}

/**
 * Try to join a thread - non-blocking version
 * 
 * @param tid Thread ID to join
 * @param retval Pointer to store the thread's return value
 * @return 0 on success (thread joined), -EAGAIN if thread still running, error code on failure
 */
long proc_thread_tryjoin(long tid, void **retval)
{
    struct proc *p = curproc;
    struct thread *current, *target = NULL;
    unsigned short sr;
    
    if (!p || !p->current_thread)
        return EINVAL;
    
    current = p->current_thread;

    TRACE_THREAD("TRY_JOIN: proc_thread_tryjoin called for tid=%ld, retval=%p", tid, retval);

    // Cannot join self - would deadlock
    if (current->tid == tid) {
        TRACE_THREAD("TRY_JOIN: Cannot join self (would deadlock)");
        return EDEADLK;
    }

    // Find target thread
    for (target = p->threads; target; target = target->next) {
        if (target->tid == tid) {
            break;
        }
    }
    
    if (!target) {
        TRACE_THREAD("TRY_JOIN: Thread %ld not found", tid);
        return ESRCH;
    }
    
    // Check if thread is joinable
    if (target->detached) {
        TRACE_THREAD("TRY_JOIN: Thread %ld is detached, cannot join", tid);
        return EINVAL;
    }
    
    // Check if thread is already joined
    if (target->joined) {
        TRACE_THREAD("TRY_JOIN: Thread %ld is already joined", tid);
        return EINVAL;
    }
    
    // Check if another thread is already joining this thread
    if (target->joiner && target->joiner != current && target->joiner->magic == CTXT_MAGIC) {
        TRACE_THREAD("TRY_JOIN: Thread %ld is already being joined by thread %d", 
                    tid, target->joiner->tid);
        return EINVAL;  // Thread is already being joined by another thread
    }

    sr = splhigh();
    
    // KEY DIFFERENCE: Check if thread exited, but don't block if it hasn't
    if (target->state & THREAD_STATE_EXITED) {
        // Thread already exited, get return value and return immediately
        if (retval)
            *retval = target->retval;
        
        // Mark as joined so resources can be freed
        target->joined = 1;
        
        // Free resources now
        cleanup_thread_resources(p, target, tid);
        
        spl(sr);
        TRACE_THREAD("TRY_JOIN: Thread %ld joined successfully", tid);
        return 0;  // Success - thread was joined
    }
    
    // Thread is still running - return immediately with EAGAIN
    spl(sr);
    TRACE_THREAD("TRY_JOIN: Thread %ld still running", tid);
    return EAGAIN;  // Thread still running, try again later
}

// Function to initialize a semaphore
int thread_semaphore_init(struct semaphore *sem, short count) {
    if (!sem || count < 0) {
        return EINVAL;
    }
    
    sem->count = count;
    sem->wait_queue = NULL;
    TRACE_THREAD("SEMAPHORE INIT: Count=%d", sem->count);
    return THREAD_SUCCESS;
}

// Function to initialize a mutex
int thread_mutex_init(struct mutex *mutex) {
    if (!mutex) {
        return EINVAL;
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
        return EINVAL;
    }

    struct thread *t = CURTHREAD;
    if (!t) {
        return EINVAL;
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
        return EDEADLK;
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
        
        // Boost owner's priority to the waiting thread's priority
        boost_thread_priority(mutex->owner, t->priority - mutex->owner->priority);
        
        // Reinsert owner in ready queue if needed
        if (mutex->owner->state == THREAD_STATE_READY) {
            remove_from_ready_queue(mutex->owner);
            add_to_ready_queue(mutex->owner);
        }
    }
    
    spl(sr);
    
    // Yield CPU - will resume here when woken
    proc_thread_schedule();
    
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
        return EINVAL;
    }
    
    struct thread *current = CURTHREAD;
    if (!current) {
        return EINVAL;
    }
    
    unsigned short sr = splhigh();
    
    // Check if current thread owns the mutex
    if (mutex->owner != current) {
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: Thread %d is not the owner (owner=%p)", 
                    current->tid, mutex->owner ? mutex->owner->tid : -1);
        spl(sr);
        return EINVAL; // Not owner
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
            reset_thread_priority(current);
            
            // Force immediate scheduling if higher priority
            if (highest->priority > current->priority) {
                TRACE_THREAD("THREAD_MUTEX_UNLOCK: Forcing immediate schedule due to priority");
                spl(sr);
                proc_thread_schedule();
                return THREAD_SUCCESS;
            }
        } else {
            // No valid waiters, release the mutex
            TRACE_THREAD("THREAD_MUTEX_UNLOCK: No valid waiters, releasing mutex");
            mutex->locked = 0;
            mutex->owner = NULL;
            
            // Restore original priority if boosted
            reset_thread_priority(current);
        }
    } else {
        // No waiters, release the mutex
        TRACE_THREAD("THREAD_MUTEX_UNLOCK: No waiters, releasing mutex");
        mutex->locked = 0;
        mutex->owner = NULL;
        
        // Restore original priority if boosted
        reset_thread_priority(current);
    }
    
    spl(sr);
    return THREAD_SUCCESS;
}

// Semaphore implementation
int thread_semaphore_down(struct semaphore *sem) {
    if (!sem) {
        return EINVAL;
    }

    struct thread *t = CURTHREAD;
    if (!t) {
        return EINVAL;
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
    proc_thread_schedule();
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
        return EINVAL;
    }
    
    struct thread *current = CURTHREAD;
    if (!current) {
        return EINVAL;
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
        proc_thread_schedule();
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