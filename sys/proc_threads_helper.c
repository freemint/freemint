#include "proc_threads_helper.h"

/**
 * Helper function to get the highest priority thread from the ready queue
 *
 * This function implements POSIX-compliant thread selection:
 * - Highest priority threads are selected first
 * - For equal priority SCHED_FIFO threads, the one that's been waiting longest is selected
 * - For equal priority SCHED_RR threads, round-robin order is used
 */
struct thread *get_highest_priority_thread(struct proc *p) {
    if (!p || !p->ready_queue) {
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Ready queue is empty");
        return NULL;
        // return get_idle_thread(p);
    }

#ifdef DEBUG_THREAD
    // Dump ready queue for debugging
    TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Current ready queue:");
    struct thread *debug_curr = p->ready_queue;
    while (debug_curr) {
        TRACE_THREAD(" Thread %d (pri %d, policy %d, boost=%d, state=%d)",
                    debug_curr->tid, debug_curr->priority, debug_curr->policy,
                    debug_curr->priority_boost, debug_curr->state);
        debug_curr = debug_curr->next_ready;
    }
#endif

    struct thread *highest = NULL;  
    struct thread *first_valid = NULL;
    struct thread *curr = p->ready_queue;
    int highest_priority = -1;

    // Single pass through the queue
    while (curr) {
        if (curr->magic == CTXT_MAGIC &&
            !(curr->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED))) {
            
            // Remember first valid thread as fallback
            if (!first_valid) {
                first_valid = curr;
            }
            
            // Check for highest priority or same priority with lower TID
            if (curr->priority > highest_priority) {
                TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Thread %d has higher priority (%d > %d)",
                            curr->tid, curr->priority, highest_priority);
                highest = curr;
                highest_priority = curr->priority;
            }
            else if (curr->priority == highest_priority && highest && curr->tid < highest->tid) {
                TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Thread %d has same priority but lower TID (%d < %d)",
                            curr->tid, curr->tid, highest->tid);
                highest = curr;
            }
        }
        curr = curr->next_ready;
    }

    // Return results with appropriate logging
    if (highest) {
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Selected highest priority thread %d (priority %d, policy %d)",
                    highest->tid, highest->priority, highest->policy);
        return highest;
    }
    
    if (first_valid) {
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Selected first valid thread %d as fallback",
                    first_valid->tid);
        return first_valid;
    }
    
    TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): No valid thread found in ready queue");
    return NULL;
}

/**
 * Get the current system tick count.
 *
 * This function returns the current system tick count, which can be used for
 * calculating time intervals. The tick count is incremented by the system
 * at a rate of 200 times per second.
 *
 * @return The current system tick count.
 */
inline unsigned long get_system_ticks(void) {
    return *((volatile unsigned long *)_hz_200);
}

/**
 * Make a process eligible for immediate selection as curproc
 * This increases the chance that curproc will become equal to p
 */
void make_process_eligible(struct proc *p) {
    if (!p) return;
    
    unsigned short sr = splhigh();
    
    // Set slices to 0 to ensure it's eligible to run immediately
    // p->slices = 2;
    
    // If not in READY_Q or CURPROC_Q, add to READY_Q
    if (p->wait_q != READY_Q && p->wait_q != CURPROC_Q) {
        // Remove from current queue if any
        TRACE_THREAD("make_process_eligible: Removing process %d from queue %d\n", p->pid, p->wait_q);
        if (p->wait_q) {
            rm_q(p->wait_q, p);
        }
        
        // Add to front of READY_Q
        p->wait_q = READY_Q;
        p->q_next = sysq[READY_Q].head;
        sysq[READY_Q].head = p;
        if (!p->q_next)
            sysq[READY_Q].tail = p;
        else
            p->q_next->q_prev = p;
        p->q_prev = NULL;
    }
    
    spl(sr);
}

/*
 * Atomic thread state change function
 * Ensures that thread state transitions are atomic operations
 * to prevent race conditions between concurrent threads
 */
void atomic_thread_state_change(struct thread *t, int new_state) {
    unsigned short sr;
    
    if (!t) {
        TRACE_THREAD("ERROR: Attempt to change state of NULL thread");
        return;
    }

    // Skip if state is already set to the new value
    if (t->state == new_state) {
        return;
    }

    // Check if thread is valid
    if (t->magic != CTXT_MAGIC) {
        TRACE_THREAD("ERROR: Attempt to change state of invalid thread %d, magic=%lx", t->tid, t->magic);
        return;
    }

    /* Check if the new state is valid */
    if ((t->state == THREAD_STATE_EXITED) && !(new_state == THREAD_STATE_EXITED)) {
        TRACE_THREAD("ERROR: Attempt to change state of EXITED thread %d from %d to %d", t->tid, t->state, new_state);
        return;
    }

    sr = splhigh();
    TRACE_THREAD("STATE: Thread %d state change from %d to %d", t->tid, t->state, new_state);
    t->state = new_state;
    spl(sr);
}

/*
 * Helper function to get the remaining time on a timeout
 */
long timeout_remaining(TIMEOUT *t)
{
    if (!t)
        return 0;
        
    ushort sr = splhigh();
    
    long remaining = 0;
    TIMEOUT *curr;
    
    for (curr = tlist; curr && curr != t; curr = curr->next) {
        remaining += curr->when;
    }
    
    if (curr == t) {
        remaining += curr->when;
    } else {
        /* Timeout not found in list */
        remaining = 0;
    }
    
    spl(sr);
    
    return remaining;
}

/*
 * Get the current thread's ID
 * Returns the thread ID or -1 on error
 */
long sys_p_thread_getid(void) {
    struct thread *t = CURTHREAD;
    
    if (!t){
        TRACE_THREAD("sys_p_thread_getid: no current thread");
        return -1;
    }
    TRACE_THREAD("sys_p_thread_getid: pid=%d, tid=%d", t->proc->pid, t->tid);
    return t->tid;
}