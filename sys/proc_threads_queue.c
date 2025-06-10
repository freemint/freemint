#include "proc_threads_queue.h"

#include "proc_threads_helper.h"
#include "proc_threads_sync.h"

/**
 * Add a thread to the ready queue with proper POSIX scheduling behavior
 * 
 * This function handles different insertion strategies based on:
 * - Thread priority
 * - Scheduling policy (SCHED_FIFO, SCHED_RR, SCHED_OTHER)
 * - Priority changes
 * - Priority boosting
 */
void add_to_ready_queue(struct thread *t) {
    struct proc *p;
    unsigned short sr;
    
    if (!t || !t->proc || (t->state & THREAD_STATE_RUNNING) || (t->state & THREAD_STATE_EXITED))
        return;
        
    if (t->tid == 0 && (t->state & THREAD_STATE_EXITED))
        return;
        
    p = t->proc;
    sr = splhigh();

    // Check if already in queue
    struct thread *curr = p->ready_queue;
    while (curr) {
        if (curr == t) {
            TRACE_THREAD("Ready_Q: Thread %d already in ready queue", t->tid);
            spl(sr);
            return;
        }
        curr = curr->next_ready;
    }

    atomic_thread_state_change(t, THREAD_STATE_READY);
    
    TRACE_THREAD("READY_Q: Adding Thread %d (pri %d, policy %d, state %d, boost=%d) to ready queue",
                t->tid, t->priority, t->policy, t->state, t->priority_boost);
    
    // Dump current ready queue for debugging (only at highest verbosity level)
    TRACE_THREAD_QUEUE_BEFORE(t);
#if THREAD_DEBUG_LEVEL >= THREAD_DEBUG_ALL
    curr = p->ready_queue;
    while (curr) {
        TRACE_THREAD("  Thread %d (pri %d, policy %d, boost=%d)", 
                    curr->tid, curr->priority, curr->policy, curr->priority_boost);
        curr = curr->next_ready;
    }
#endif
    
    // If ready queue is empty, just add the thread
    if (!p->ready_queue) {
        TRACE_THREAD("READY_Q: Empty queue, adding thread %d as head", t->tid);
        p->ready_queue = t;
        t->next_ready = NULL;
        spl(sr);
        return;
    }
    
    // Priority boosted threads: add in strict priority order
    if (t->priority_boost) {
        TRACE_THREAD("READY_Q: Thread %d has boosted priority %d, adding in priority order", 
                    t->tid, t->priority);
                    
        // Find position based on priority
        struct thread **pp = &p->ready_queue;
        while (*pp && (*pp)->priority > t->priority) {
            TRACE_THREAD("  Skipping thread %d (pri %d > %d)", 
                        (*pp)->tid, (*pp)->priority, t->priority);
            pp = &(*pp)->next_ready;
        }
        
        // For equal priority, ensure FIFO order by TID
        if (*pp && (*pp)->priority == t->priority) {
            TRACE_THREAD("  Equal priority threads found, checking TID for FIFO order");
            // If this thread has a lower TID, it should go first (FIFO)
            if (t->tid < (*pp)->tid) {
                TRACE_THREAD("  Thread %d has lower TID than %d, inserting here", 
                            t->tid, (*pp)->tid);
            } else {
                // Skip all threads with same priority but lower TID
                while (*pp && (*pp)->priority == t->priority && (*pp)->tid < t->tid) {
                    TRACE_THREAD("  Skipping thread %d (pri %d, TID %d < %d)", 
                                (*pp)->tid, (*pp)->priority, (*pp)->tid, t->tid);
                    pp = &(*pp)->next_ready;
                }
            }
        }
        
        // Insert at the right position
        TRACE_THREAD("  Inserting thread %d before thread %d", 
                    t->tid, *pp ? (*pp)->tid : -1);
        t->next_ready = *pp;
        *pp = t;
    }
    // Other scheduling policies
    else {
        // Default: add to end of queue
        TRACE_THREAD("READY_Q: Thread %d using default queue insertion", t->tid);
        struct thread *last = p->ready_queue;
        while (last->next_ready) {
            last = last->next_ready;
        }
        last->next_ready = t;
        t->next_ready = NULL;
    }
    
    // Dump final ready queue for debugging (only at highest verbosity level)
    TRACE_THREAD_QUEUE_AFTER(t);
#if THREAD_DEBUG_LEVEL >= THREAD_DEBUG_ALL
    curr = p->ready_queue;
    while (curr) {
        TRACE_THREAD("  Thread %d (pri %d, policy %d, boost=%d)", 
                    curr->tid, curr->priority, curr->policy, curr->priority_boost);
        curr = curr->next_ready;
    }
#endif
    
    spl(sr);
}

void remove_from_ready_queue(struct thread *t) {
    if (!t || !t->proc) return;
    struct proc *p = t->proc;
    unsigned short sr = splhigh();

    if (!p->ready_queue) {
        spl(sr);
        return;
    }

    struct thread **pp = &p->ready_queue;
    struct thread *curr = p->ready_queue;
    int found = 0;

    while (curr) {
        if (curr == t) {
            *pp = curr->next_ready;
            curr->next_ready = NULL;
            found = 1;
            TRACE_THREAD("READY_Q: Removed thread %d from ready queue", t->tid);
            break;
        }
        pp = &(curr->next_ready);
        curr = curr->next_ready;
    }

    if (!found) {
        TRACE_THREAD("READY_Q: Thread %d not found in ready queue", t->tid);
    }

    spl(sr);
}

/**
 * Remove a thread from its process's sleep queue
 * 
 * @param p Process containing the sleep queue
 * @param t Thread to remove from sleep queue
 */
void remove_from_sleep_queue(struct proc *p, struct thread *t) {
    if (!p || !t) return;
    
    unsigned short sr = splhigh();
    
    struct thread **pp = &p->sleep_queue;
    
    while (*pp) {
        if (*pp == t) {
            *pp = t->next_sleeping;
            t->next_sleeping = NULL;
            TRACE_THREAD("SLEEP_Q: Removed thread %d from sleep queue", t->tid);
            break;
        }
        pp = &(*pp)->next_sleeping;
    }
    
    spl(sr);
}

void remove_thread_from_wait_queues(struct thread *t) {
    if (!t) return;
    
    unsigned short sr = splhigh();
    
    // Remove from mutex wait queues
    if ((t->wait_type & WAIT_MUTEX) && t->mutex_wait_obj) {
        struct mutex *m = (struct mutex *)t->mutex_wait_obj;
        struct thread **pp = &m->wait_queue;
        while (*pp) {
            if (*pp == t) {
                TRACE_THREAD("Removing thread %d from mutex wait queue", t->tid);
                *pp = t->next_wait;
                break;
            }
            pp = &(*pp)->next_wait;
        }
        t->wait_type &= ~WAIT_MUTEX;
        t->next_wait = NULL;
        t->mutex_wait_obj = NULL;
    }
    
    // Remove from semaphore wait queues
    if ((t->wait_type & WAIT_SEMAPHORE) && t->sem_wait_obj) {
        struct semaphore *sem = (struct semaphore *)t->sem_wait_obj;
        struct thread **pp = &sem->wait_queue;
        while (*pp) {
            if (*pp == t) {
                TRACE_THREAD("Removing thread %d from semaphore wait queue", t->tid);
                *pp = t->next_wait;
                break;
            }
            pp = &(*pp)->next_wait;
        }
        t->wait_type &= ~WAIT_SEMAPHORE;
        t->next_wait = NULL;
        t->sem_wait_obj = NULL;
    } 
    
    // Remove from signal wait queue
    if ((t->wait_type & WAIT_SIGNAL) && t->sig_wait_obj) {
        struct thread **pp = &t->proc->signal_wait_queue;
        while (*pp) {
            if (*pp == t) {
                TRACE_THREAD("Removing thread %d from signal wait queue", t->tid);
                *pp = t->next_sigwait;
                break;
            }
            pp = &(*pp)->next_sigwait;
        }
        t->wait_type &= ~WAIT_SIGNAL;
        t->next_sigwait = NULL;
        t->sig_wait_obj = NULL;
    }
    
    if ((t->wait_type & WAIT_JOIN) && t->join_wait_obj) {
        struct thread *target = (struct thread *)t->join_wait_obj;
        // No need to remove from a queue, just clear the joiner field if it points to this thread
        if (target && target->joiner == t) {
            TRACE_THREAD("Clearing joiner reference for thread %d", target->tid);
            target->joiner = NULL;
        }
        t->wait_type &= ~WAIT_JOIN;
        t->join_wait_obj = NULL;
    }
    
    spl(sr);
}

int is_in_ready_queue(struct thread *t) {
    if (!t || !t->proc)
        return 0;

    struct thread *cur = t->proc->ready_queue;
    while (cur) {
        if (cur == t)
            return 1;
        cur = cur->next_ready;
    }
    return 0;
}

/**
 * Find the highest priority thread in a wait queue using bitmap optimization
 * 
 * @param queue The wait queue to search
 * @param prev_highest Pointer to store the previous thread of the highest priority thread
 * @return The highest priority thread, or NULL if none found
 */
struct thread *find_highest_priority_thread_in_queue(struct thread *queue, struct thread **prev_highest) {
    if (!queue) {
        return NULL;
    }
    
    // Create priority bitmap
    unsigned short wait_bitmap = 0;
    struct thread *t = queue;
    
    // Build bitmap from wait queue
    while (t) {
        if (t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED)) {
            // Priority is already scaled when set
            unsigned short bit = 1 << t->priority;
            wait_bitmap |= bit;
            TRACE_THREAD("QUEUE: Thread %d priority %d, bit 0x%04x, bitmap now 0x%04x", 
                        t->tid, t->priority, bit, wait_bitmap);
        }
        t = t->next_wait;
    }
    
    // Find highest priority thread
    struct thread *highest = NULL;
    *prev_highest = NULL;
    struct thread *prev = NULL;
    
    if (wait_bitmap) {
        int highest_pri = find_highest_priority_bit_word(wait_bitmap);
        TRACE_THREAD("QUEUE: Found highest priority bit %d from bitmap 0x%04x", 
                    highest_pri, wait_bitmap);
        
        // Find first thread with this priority
        t = queue;
        prev = NULL;
        
        while (t) {
            if (t->magic == CTXT_MAGIC && 
                !(t->state & THREAD_STATE_EXITED) &&
                t->priority == highest_pri) {
                highest = t;
                *prev_highest = prev;
                break;
            }
            prev = t;
            t = t->next_wait;
        }
    }
    
    return highest;
}
