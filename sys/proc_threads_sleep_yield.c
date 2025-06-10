#include "proc_threads_sleep_yield.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"

/* Maximum value for an unsigned long */
#ifndef ULONG_MAX
#define ULONG_MAX 0xFFFFFFFFUL
#endif

/**
 * Wake threads that have reached their wakeup time
 * 
 * @param p Process containing threads to check
 * @param current_time Current system time in ticks
 * @return Number of threads woken
 */
int wake_threads_by_time(struct proc *p, unsigned long current_time) {
    if (!p || !p->sleep_queue) {
        return 0;
    }
    
    struct thread *sleep_t = p->sleep_queue;
    int woke_threads = 0;

    while (sleep_t) {
        struct thread *next_sleep = sleep_t->next_sleeping;

        // If thread should wake up
        if (sleep_t->magic == CTXT_MAGIC && 
            (sleep_t->state & THREAD_STATE_BLOCKED) &&
            !(sleep_t->state & THREAD_STATE_EXITED) &&
            sleep_t->wakeup_time > 0 && 
            sleep_t->wakeup_time <= current_time) {
            TRACE_THREAD("SLEEP_CHECK: Thread %d should wake up!", sleep_t->tid);
            
            // Remove from sleep queue
            remove_from_sleep_queue(p, sleep_t);

            // Boost priority and prepare for ready queue
            boost_thread_priority(sleep_t, 5); // Boost by 5 levels
            sleep_t->wait_type &= ~WAIT_SLEEP;
            sleep_t->wakeup_time = 0; // Clear wake-up time
            
            // Update thread state and add to ready queue
            atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
            add_to_ready_queue(sleep_t);
            
            woke_threads++;
        }
        
        sleep_t = next_sleep;
    }
    
    return woke_threads;
}

/**
 * Check and wake sleeping threads that have reached their wakeup time
 * 
 * @param p Process containing threads to check
 */
void check_and_wake_sleeping_threads(struct proc *p) {
    if (!p || !p->sleep_queue) {
        return;
    }
    unsigned short sr;
    int woke_threads;

    #ifdef DEBUG_THREAD
    unsigned long current_time = get_system_ticks();
    
    TRACE_THREAD("SLEEP_CHECK: Checking sleep queue for process %d at time %lu", 
                p->pid, current_time);
    
    // Debug: dump sleep queue
    struct thread *debug_t = p->sleep_queue;
    while (debug_t) {
        TRACE_THREAD("  Thread %d (wake at %lu, current %lu, state=%d)", 
                    debug_t->tid, 
                    debug_t->wakeup_time, 
                    current_time,
                    debug_t->state);
        debug_t = debug_t->next_sleeping;
    }
    #endif

    sr = splhigh();
    
    // Wake threads that have reached their wakeup time
    woke_threads = wake_threads_by_time(p, get_system_ticks());
    
    if (woke_threads > 0) {
        TRACE_THREAD("SLEEP_CHECK: Woke up %d threads from sleep queue", woke_threads);
        // Force a schedule if we woke up any threads and this is the current process
        if (p == curproc) {
            TRACE_THREAD("SLEEP_CHECK: Forcing schedule after waking threads");
            spl(sr);
            proc_thread_schedule();
            return;
        }        
    }
    
    spl(sr);
}

void proc_thread_sleep_wakeup_handler(PROC *p, long arg) {
    struct thread *t = (struct thread *)arg;
    
    if (!t || !p || t->proc != p) {
        return;
    }
    
    unsigned short sr = splhigh();
    
    // Only wake up if still sleeping
    if ((t->state & THREAD_STATE_BLOCKED) && (t->wait_type & WAIT_SLEEP)) {
        TRACE_THREAD("SLEEP_WAKEUP: Direct wakeup for thread %d", t->tid);

        // Boost priority
        boost_thread_priority(t, 5);  // Boost by 5 levels
        
        // Wake up thread
        t->wait_type &= ~WAIT_SLEEP;
        t->wakeup_time = 0;  // Clear wake-up time
        remove_from_sleep_queue(p, t);
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        // Force a schedule to run this thread immediately if possible
        spl(sr);

        reschedule_preemption_timer(p, t->tid);

        proc_thread_schedule();
        
        return;
    }
    
    spl(sr);
}

long proc_thread_sleep(long ms) {
    struct proc *p = curproc;
    struct thread *t = p ? p->current_thread : NULL;
    unsigned short sr;
    
    if (!p || !t) return EINVAL;
    if (t->tid == 0) return EINVAL; // thread0 can't sleep
    if (ms <= 0) return 0; // No need to sleep

    // Prevent nested blocking
    if (t->wait_type != WAIT_NONE) {
        TRACE_THREAD("SLEEP: Thread %d already blocked", t->tid);
        return EDEADLK;
    }

    unsigned long current_time = get_system_ticks();
    unsigned long ticks;

    // Handle special cases
    if (ms <= 0) {
        // No sleep or invalid sleep time
        ticks = 0;
    } else if (ms >= ULONG_MAX / 2) {
        // Very long sleep time, cap it to avoid overflow
        ticks = ULONG_MAX / 2;
        TRACE_THREAD("SLEEP: Sleep time too large, capping to %lu ticks", ticks);
    } else {
        // Normal case: convert ms to ticks with rounding up
        ticks = (ms + MS_PER_TICK - 1) / MS_PER_TICK;
    }

    // Set wakeup time
    t->wakeup_time = current_time + ticks;

    TRACE_THREAD_SLEEP(t, ms, ticks, t->wakeup_time);
    
    // Add to sleep queue
    sr = splhigh();
    
    // Remove from sleep queue if already there
    remove_from_sleep_queue(p, t);
    
    // Add to sleep queue
    t->next_sleeping = p->sleep_queue;
    p->sleep_queue = t;
    TRACE_THREAD("SLEEP: Added thread %d to sleep queue (wake at %lu)", 
                t->tid, t->wakeup_time);
    
        
    // Set up a direct timeout for this thread
    t->sleep_timeout = addtimeout(p, ms, proc_thread_sleep_wakeup_handler);
    if (t->sleep_timeout) {
        t->sleep_timeout->arg = (long)t;
    } else {
        TRACE_THREAD("SLEEP: Failed to set up sleep timeout");
        spl(sr);
        return -1;
    }
    
    if (save_context(&t->ctxt[CURRENT]) == 0) {
        // First time through - this is the "going to sleep" path
        TRACE_THREAD_SLEEP(t, ms, ticks, t->wakeup_time);
        
        atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
        t->wait_type |= WAIT_SLEEP;  // Set wait type
        remove_from_ready_queue(t);
        
        // Schedule another thread
        spl(sr);
        proc_thread_schedule();
        
        // We should never reach here - proc_thread_schedule() will switch to another thread
        TRACE_THREAD("SLEEP: ERROR - Returned from proc_thread_schedule() in sleep path!");
        return -1;
    } else {
        // Second time through - this is the "waking up" path
        // This code runs when the thread is awakened and context is restored
        
        // When we return, check if we woke up on time
        current_time = get_system_ticks();
        if (t->wakeup_time > 0 && current_time > t->wakeup_time) {
            TRACE_THREAD("SLEEP: Thread %d woke up late by %lu ms",
                        t->tid, (current_time - t->wakeup_time) * MS_PER_TICK);
        }
        t->wait_type &= ~WAIT_SLEEP;
        t->wakeup_time = 0; // Clear wake-up time
        
        // Ensure we're in the RUNNING state
        if (t->state != THREAD_STATE_RUNNING) {
            TRACE_THREAD("SLEEP: Thread %d not in RUNNING state after wake, fixing", t->tid);
            atomic_thread_state_change(t, THREAD_STATE_RUNNING);
        }
        
        TRACE_THREAD_WAKEUP(t);
        spl(sr);
        return 0;
    }
}

long proc_thread_yield(void) {
    struct proc *p = curproc;
    struct thread *t;
    
    if (!p || !p->current_thread)
        return -EINVAL;
        
    t = p->current_thread;
    
    // For SCHED_FIFO and SCHED_RR, move to end of same-priority list
    if (t->policy == SCHED_FIFO || t->policy == SCHED_RR) {
        unsigned short sr = splhigh();
        
        // Only if we're in RUNNING state
        if (t->state == THREAD_STATE_RUNNING) {
            // Change to READY and add to end of ready queue
            atomic_thread_state_change(t, THREAD_STATE_READY);
            
            // Add to ready queue (will be added at end of same-priority list)
            add_to_ready_queue(t);
            
            // Force a reschedule
            spl(sr);
            proc_thread_schedule();
            return 0;
        }
        
        spl(sr);
    } else {
        // For SCHED_OTHER, just call schedule
        proc_thread_schedule();
    }
    
    return 0;
}
