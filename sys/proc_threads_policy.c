#include "proc_threads_policy.h"
#include "proc_threads_queue.h"
#include "proc_threads_helper.h"
#include "proc_threads_scheduler.h"

/* Forward declarations for internal functions */
static int set_thread_timeslice(struct thread *t, long timeslice);
static int set_thread_policy(struct thread *t, enum sched_policy policy, int priority);
static int get_rr_interval(struct thread *t, long *interval);

/**
 * Get the time quantum for SCHED_RR threads
 * 
 * @param t The thread to query
 * @param interval Pointer to store the time quantum in milliseconds
 * @return 0 on success, negative error code on failure
 */
static int get_rr_interval(struct thread *t, long *interval)
{
    if (!t || !interval)
        return EINVAL;
        
    *interval = t->timeslice * 5; // Convert ticks to milliseconds (5ms per tick)
    return 0;
}

/**
 * Set scheduling policy and priority for a thread
 * 
 * @param t The thread to modify
 * @param policy The new scheduling policy (SCHED_FIFO, SCHED_RR, SCHED_OTHER)
 * @param priority The new priority (0-99, with 0 being lowest)
 * @return 0 on success, negative error code on failure
 */
static int set_thread_policy(struct thread *t, enum sched_policy policy, int priority)
{
    if (!t || t->magic != CTXT_MAGIC)
        return EINVAL;

    // Validate process pointer
    if (!t->proc)
        return EINVAL;
        
    // Log the request
    TRACE_THREAD("POLICY: Setting thread %d policy to %d, priority to %d (current: policy=%d, pri=%d)",
                t->tid, policy, priority, t->policy, t->priority);

    // Acquire lock        
    unsigned short sr = splhigh();

    // Validate policy and priority
    if (policy != SCHED_FIFO && policy != SCHED_RR && policy != SCHED_OTHER)
        return EINVAL;

    // SCHED_OTHER can only use priority 0
    if (policy == SCHED_OTHER && priority != 0)
        priority = 0;

    // Clamp priority to valid range
    if (priority < 0)
        priority = 0;
    else if (priority > MAX_THREAD_PRIORITY)
        priority = MAX_THREAD_PRIORITY;
        
    // Save old values
    #ifdef DEBUG_THREAD
    int old_policy = t->policy;
    #endif
    int old_priority = t->priority;
    
    // Update policy
    t->policy = policy;
    
    // Update timeslice based on new policy
    if (policy == SCHED_RR) {
        t->timeslice = t->proc->thread_rr_timeslice;
    } else if (policy == SCHED_FIFO) {
        t->timeslice = 0;  // FIFO threads don't use timeslicing
    } else {
        t->timeslice = t->proc->thread_default_timeslice;
    }
    
    // Reset remaining timeslice
    t->remaining_timeslice = t->timeslice;
    
    // Update priority based on POSIX rules for SCHED_FIFO
    if (policy == SCHED_FIFO && t->state == THREAD_STATE_RUNNING) {
        if (priority > old_priority) {
            // If raising priority, move to end of list for new priority
            if (is_in_ready_queue(t)) {
                remove_from_ready_queue(t);
                t->priority = priority;
                add_to_ready_queue(t);
            } else {
                t->priority = priority;
            }
        } else if (priority < old_priority) {
            // If lowering priority, move to front of list for new priority
            if (is_in_ready_queue(t)) {
                remove_from_ready_queue(t);
                t->priority = priority;
                // Add to front of ready queue
                add_to_ready_queue(t);
            } else {
                t->priority = priority;
            }
        } else {
            // No change in priority, position unchanged
        }
    } else {
        // For other policies or non-running threads, just update priority
        t->priority = priority;
    }
    
    // If not using priority boost, update original priority too
    if (!t->priority_boost) {
        t->original_priority = priority;
    }
    
    #ifdef DEBUG_THREAD
    TRACE_THREAD("THREAD_SCHED: Thread %d policy changed from %d to %d, priority from %d to %d, timeslice=%d",
                t->tid, old_policy, policy, old_priority, priority, t->timeslice);
    #endif
    // If this is the current thread and we lowered priority, trigger a reschedule
    if (t == t->proc->current_thread && priority < old_priority) {
        spl(sr);
        proc_thread_schedule();
        return 0;
    }
    
    spl(sr);
    return 0;
}

/**
 * System call to set thread scheduling policy and priority
 */
long proc_thread_set_schedparam(long tid, long policy, long priority)
{
    struct proc *p = curproc;
    struct thread *t = NULL;
    
    if (!p)
        return EINVAL;
        
    // Find thread by ID
    if (tid < 0) {
        // Use current thread
        t = p->current_thread;
    } else {
        // Find thread with specified ID
        for (t = p->threads; t != NULL; t = t->next) {
            if (t->tid == tid)
                break;
        }
        
        if (!t)
            return ESRCH; // No such thread
    }
    
    return set_thread_policy(t, (enum sched_policy)policy, (int)priority);
}

/**
 * System call to get thread scheduling policy and priority
 */
long proc_thread_get_schedparam(long tid, long *policy, long *priority)
{
    struct proc *p = curproc;
    struct thread *t = NULL;
    
    if (!p || !policy || !priority)
        return EINVAL;
        
    // Find thread by ID
    if (tid < 0) {
        // Use current thread
        t = p->current_thread;
    } else {
        // Find thread with specified ID
        for (t = p->threads; t != NULL; t = t->next) {
            if (t->tid == tid)
                break;
        }
        
        if (!t)
            return ESRCH; // No such thread
    }
    
    *policy = t->policy;
    *priority = t->priority;
    
    return 0;
}

/**
 * System call to get the RR interval for a thread
 */
long proc_thread_get_rrinterval(long tid, long *interval)
{
    struct proc *p = curproc;
    struct thread *t = NULL;
    
    if (!p || !interval)
        return EINVAL;
        
    // Find thread by ID
    if (tid < 0) {
        // Use current thread
        t = p->current_thread;
    } else {
        // Find thread with specified ID
        for (t = p->threads; t != NULL; t = t->next) {
            if (t->tid == tid)
                break;
        }
        
        if (!t)
            return -ESRCH; // No such thread
    }
    
    return get_rr_interval(t, interval);
}

/**
 * System call to set thread timeslice
 */
long proc_thread_set_timeslice(long tid, long timeslice)
{
    struct proc *p = curproc;
    struct thread *t = NULL;
    
    if (!p)
        return EINVAL;
        
    // Find thread by ID
    if (tid < 0) {
        // Use current thread
        t = p->current_thread;
    } else {
        // Find thread with specified ID
        for (t = p->threads; t != NULL; t = t->next) {
            if (t->tid == tid)
                break;
        }
        
        if (!t)
            return ESRCH; // No such thread
    }
    
    return set_thread_timeslice(t, timeslice);
}

/**
 * System call to get thread timeslice
 */
long proc_thread_get_timeslice(long tid, long *timeslice, long *remaining)
{
    struct proc *p = curproc;
    struct thread *t = NULL;
    
    if (!p || !timeslice)
        return EINVAL;
        
    // Find thread by ID
    if (tid < 0) {
        // Use current thread
        t = p->current_thread;
    } else {
        // Find thread with specified ID
        for (t = p->threads; t != NULL; t = t->next) {
            if (t->tid == tid)
                break;
        }
        
        if (!t)
            return -ESRCH; // No such thread
    }
    
    *timeslice = t->timeslice;
    if (remaining)
        *remaining = t->remaining_timeslice;
    
    return 0;
}

/**
 * Set the timeslice for a thread
 * 
 * @param t The thread to modify
 * @param timeslice The new timeslice in ticks
 * @return 0 on success, negative error code on failure
 */
static int set_thread_timeslice(struct thread *t, long timeslice)
{
    if (!t || t->magic != CTXT_MAGIC)
        return EINVAL;

    // Validate process pointer
    if (!t->proc)
        return EINVAL;
        
    // Enforce minimum timeslice        
    if (timeslice < t->proc->thread_min_timeslice)
        timeslice = t->proc->thread_min_timeslice;

    TRACE_THREAD("TIMESLICE: Setting thread %d timeslice to %ld (min=%d)", t->tid, timeslice, t->proc->thread_min_timeslice);

    unsigned short sr = splhigh();
    
    // FIFO threads don't use timeslicing
    if (t->policy == SCHED_FIFO) {
        spl(sr);
        return 0;
    }
    
    t->timeslice = timeslice;
    t->remaining_timeslice = timeslice;

    TRACE_THREAD("THREAD_SCHED: Thread %d timeslice set to %ld ticks",
                t->tid, timeslice);
                
    spl(sr);
    return 0;
}

/**
 * Update thread timeslice accounting during context switches
 * 
 * This function should be called when a thread is preempted to update
 * its remaining timeslice.
 * 
 * @param t The thread being preempted
 */
void update_thread_timeslice(struct thread *t)
{
    if (!t || t->magic != CTXT_MAGIC)
        return;
        
    unsigned long elapsed = get_system_ticks() - t->last_scheduled;
    
    // Only update for non-FIFO threads
    if (t->policy != SCHED_FIFO) {
        if (t->remaining_timeslice <= elapsed) {
            // Reset timeslice when expired
            t->remaining_timeslice = t->timeslice;
        } else {
            // Decrement remaining timeslice
            t->remaining_timeslice -= elapsed;
        }
    }
    
    // Update last scheduled time
    t->last_scheduled = get_system_ticks();
}

// Function to set thread scheduling policy
long proc_thread_set_policy(enum sched_policy policy, short priority, short timeslice)
{
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("proc_thread_set_policy: invalid current process/thread");
        return EINVAL;
    }

    struct thread *current = p->current_thread;
    unsigned short sr = splhigh();
    
    // Set the policy and priority
    int result = set_thread_policy(current, policy, priority);
    
    // Set timeslice if valid
    if (timeslice > 0) {
        current->timeslice = timeslice;
        current->total_timeslice = timeslice;
    }
    
    spl(sr);
    // return 0;
    return result;
}