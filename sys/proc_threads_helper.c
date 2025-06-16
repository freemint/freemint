#include "proc_threads_helper.h"

/* Priority bit lookup table for fast highest bit finding */
const unsigned char bit_table[256] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

/* Fast function to find highest priority bit in a bitmap */
inline int find_highest_priority_bit(unsigned char bitmap) {
    return bit_table[bitmap] - 1;  // Adjust to 0-7 range
}

/* Fast function to find highest priority bit in a word bitmap */
inline int find_highest_priority_bit_word(unsigned short bitmap) {
    if (bitmap >> 8) {
        return find_highest_priority_bit(bitmap >> 8) + 8;
    } else {
        return find_highest_priority_bit(bitmap);
    }
}

/**
 * Scale thread priority from POSIX range (0-99) to internal bitmap range (0-16)
 *
 * This function is used when setting thread priorities via syscalls to convert
 * from the standard POSIX priority range to our internal optimized range.
 * The scaling maintains proportional relationships between priorities.
 *
 * @param priority The priority value in POSIX range (0-99)
 * @return The scaled priority value in internal range (0-16)
 */
inline int scale_thread_priority(int priority) {
    /* Fast multiply and shift approach using 32-bit intermediate value */
    long temp = (long)priority * 10923L;
    return (int)(temp >> 16);
}

/**
 * Boost a thread's priority by a specified amount
 * 
 * @param t Thread to boost
 * @param boost_amount Amount to boost the priority by
 */
void boost_thread_priority(struct thread *t, int boost_amount) {
    if (!t || t->magic != CTXT_MAGIC) {
        return;
    }
    
    // Save original priority if not already boosted
    if (!t->priority_boost) {
        t->original_priority = t->priority;
    }
    
    // Apply boost
    t->priority = t->priority + boost_amount;
    t->priority_boost = 1;
    
    TRACE_THREAD_PRIORITY(t, t->original_priority, t->priority);
}

/**
 * Reset a thread's priority to its original value
 * 
 * @param t Thread to reset priority for
 */
void reset_thread_priority(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC || !t->priority_boost) {
        return;
    }
    
    TRACE_THREAD_PRIORITY(t, t->priority, t->original_priority);
    
    t->priority = t->original_priority;
    t->priority_boost = 0;
}

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
        return NULL;
    }

    // Create priority bitmaps
    unsigned short rt_bitmap = 0;
    unsigned short normal_bitmap = 0;
    
    // Build bitmaps from ready queue
    struct thread *t = p->ready_queue;
    while (t) {
        if (t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED)) {
            // Priority is already scaled when set
            unsigned short bit = 1 << t->priority;
            
            if (t->policy == SCHED_FIFO || t->policy == SCHED_RR) {
                rt_bitmap |= bit;
            } else {
                normal_bitmap |= bit;
            }
        }
        t = t->next_ready;
    }
    
    // Find highest priority thread
    unsigned char highest_pri = 0;
    
    // Check RT bitmap first
    if (rt_bitmap) {
        highest_pri = find_highest_priority_bit_word(rt_bitmap);
        
        // Find first thread with this priority
        t = p->ready_queue;
        while (t) {
            if (t->magic == CTXT_MAGIC && 
                !(t->state & THREAD_STATE_EXITED) &&
                (t->policy == SCHED_FIFO || t->policy == SCHED_RR) &&
                t->priority == highest_pri) {
                return t;
            }
            t = t->next_ready;
        }
    }
    
    // Check normal bitmap
    if (normal_bitmap) {
        highest_pri = find_highest_priority_bit_word(normal_bitmap);
        
        // Find first thread with this priority
        t = p->ready_queue;
        while (t) {
            if (t->magic == CTXT_MAGIC && 
                !(t->state & THREAD_STATE_EXITED) &&
                t->policy == SCHED_OTHER &&
                t->priority == highest_pri) {
                return t;
            }
            t = t->next_ready;
        }
    }
    
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
    
    register unsigned short sr = splhigh();
    
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
    
    if (!t) {
        TRACE_THREAD_ERROR("Attempt to change state of NULL thread");
        return;
    }

    // Skip if state is already set to the new value
    if (t->state == new_state) {
        return;
    }

    // Check if thread is valid
    if (t->magic != CTXT_MAGIC) {
        TRACE_THREAD_ERROR("Attempt to change state of invalid thread %d, magic=%lx", t->tid, t->magic);
        return;
    }

    /* Check if the new state is valid */
    if ((t->state == THREAD_STATE_EXITED) && !(new_state == THREAD_STATE_EXITED)) {
        TRACE_THREAD_ERROR("Attempt to change state of EXITED thread %d from %d to %d", t->tid, t->state, new_state);
        return;
    }

    register unsigned short sr = splhigh();
    TRACE_THREAD_STATE(t, t->state, new_state);
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
        
    register unsigned short sr = splhigh();
    
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