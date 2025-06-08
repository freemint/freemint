#include "proc_threads_scheduler.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_sleep_yield.h"
#include "proc_threads_policy.h"
#include "proc_threads_sync.h"

void reset_thread_switch_state(void);
void thread_switch_timeout_handler(PROC *p, long arg);

static int should_schedule_thread(struct thread *current, struct thread *next);
static void thread_switch(struct thread *from, struct thread *to);

/* Mutex for timer operations */
static int timer_operation_locked = 0;
static int thread_switch_in_progress = 0;
static TIMEOUT *thread_switch_timeout = NULL;

/**
 * Thread preemption handler
 * 
 * This function is called periodically to implement preemptive multitasking.
 * It checks if the current thread should be preempted and schedules another thread if needed.
 */
void thread_preempt_handler(PROC *p, long arg) {
    unsigned short sr;
    struct thread *thread_arg = (struct thread *)arg;
    
    if (!p) {
        TRACE_THREAD("PREEMPT: Invalid process pointer");
        return;
    }

    // If not current process, reschedule the timeout
    if (p != curproc) {
        /* Boost the timer for the current process */
        /* Should be disabled for non threaded mintlib's functions like sleep() */
        // make_process_eligible(p);

        reschedule_preemption_timer(p, (long)p->current_thread);
        return;
    }
    // Check for sleeping threads first
    if (p->sleep_queue) {
        TRACE_THREAD("PREEMPT: Checking sleep queue for process %d", p->pid);
        check_and_wake_sleeping_threads(p);
    }

    TRACE_THREAD("PREEMPT: Timer fired for process %d", p->pid);
    // Validate thread argument once
    if (thread_arg && (thread_arg->magic != CTXT_MAGIC || thread_arg->proc != p)) {
        TRACE_THREAD("PREEMPT: Invalid thread argument, using current thread");
        thread_arg = p->current_thread;
    }
    
    // Protection against reentrance
    if (p->p_thread_timer.in_handler) {
        if (!p->p_thread_timer.enabled) {
            TRACE_THREAD("PREEMPT: Timer disabled, not rescheduling");
            return;
        }
        TRACE_THREAD("PREEMPT: Already in handler, rescheduling");
        reschedule_preemption_timer(p, (long)p->current_thread);
        return;
    }
    
    p->p_thread_timer.in_handler = 1;
    p->p_thread_timer.timeout = NULL;
    sr = splhigh();
    
    struct thread *curr_thread = p->current_thread;
    
    // Update timeslice accounting (skip for FIFO threads)
    if (curr_thread->policy != SCHED_FIFO) {
        unsigned long elapsed = get_system_ticks() - curr_thread->last_scheduled;
        
        if (curr_thread->remaining_timeslice <= elapsed) {
            // Timeslice expired, reset and mark for potential preemption
            curr_thread->remaining_timeslice = curr_thread->timeslice;
            
            // For RR and OTHER, move to end of ready queue if preempted
            if (curr_thread->policy == SCHED_RR || curr_thread->policy == SCHED_OTHER) {
                TRACE_THREAD("PREEMPT: Thread %d timeslice expired", curr_thread->tid);
                atomic_thread_state_change(curr_thread, THREAD_STATE_READY);
                add_to_ready_queue(curr_thread);
            }
        } else {
            curr_thread->remaining_timeslice -= elapsed;
        }
    }

    struct thread *next = get_highest_priority_thread(p);
    
    if (!next) {
        TRACE_THREAD("PREEMPT: No other threads to run, continuing with current thread %d", curr_thread->tid);
        
        // If there are sleeping threads, create an idle thread
        if (p->sleep_queue) {
            next = get_idle_thread(p);
            if (next) {
                TRACE_THREAD("PREEMPT: Created idle thread to wait for sleeping threads");
                remove_from_ready_queue(next);
                atomic_thread_state_change(next, THREAD_STATE_RUNNING);
                p->current_thread = next;
                next->last_scheduled = get_system_ticks();
                
                // Rearm timer before switch
                reschedule_preemption_timer(p, (long)next);
                
                TRACE_THREAD("PREEMPT: Switching from thread %d to idle thread", curr_thread->tid);
                thread_switch(curr_thread, next);
                
                spl(sr);
                return;
            }
        }
        
        reschedule_preemption_timer(p, (long)curr_thread);
        spl(sr);
        return;
    }
    
    // Check if we should preempt current thread
    if (should_schedule_thread(curr_thread, next)) {
        // Don't preempt if thread switch already in progress
        if (thread_switch_in_progress) {
            TRACE_THREAD("PREEMPT: Thread switch already in progress");
            reschedule_preemption_timer(p, (long)curr_thread);
            spl(sr);
            return;
        }
        
        // Perform the switch
        remove_from_ready_queue(next);
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        next->last_scheduled = get_system_ticks();
        
        // Rearm timer before switch
        reschedule_preemption_timer(p, (long)next);
        
        TRACE_THREAD("PREEMPT: Switching from thread %d to thread %d", curr_thread->tid, next->tid);
        thread_switch(curr_thread, next);
        
        spl(sr);
        return;
    }
    
    // No switch needed, reschedule timer
    reschedule_preemption_timer(p, (long)curr_thread);
    spl(sr);
}

/**
 * Schedule a new thread to run
 * 
 * This function implements the core scheduling algorithm for threads.
 * It selects the next thread to run based on priority and scheduling policy,
 * and performs the context switch if needed.
 */
void proc_thread_schedule(void) {
    struct proc *p = get_curproc();
    unsigned short sr;
    
    if (!p) {
        TRACE_THREAD("SCHED: Invalid current process");
        return;
    }

    TRACE_THREAD("SCHED: Entered scheduler");
    sr = splhigh();
    
    struct thread *current = p->current_thread;
    
    // Check and wake any sleeping threads
    check_and_wake_sleeping_threads(p);
    
    // Get highest priority thread from ready queue
    struct thread *next = get_highest_priority_thread(p);

    TRACE_THREAD("SCHED: Current thread %d, highest priority ready thread %d", 
                current ? current->tid : -1, 
                next ? next->tid : -1);

    // Validate next thread before proceeding
    if (next && (next->magic != CTXT_MAGIC || (next->state & THREAD_STATE_EXITED))) {
        TRACE_THREAD("SCHED: Next thread %d is invalid or exited, removing from ready queue", 
                    next ? next->tid : -1);
        if (next) {
            remove_from_ready_queue(next);
        }
        next = NULL;
    }

    if (!next) {
        // Always fallback to thread0 if available
        struct thread *thread0 = get_main_thread(p);
        
        if (thread0 && thread0->state == THREAD_STATE_READY) {
            next = thread0;
            TRACE_THREAD("SCHED: Falling back to thread0");
        } else if (current && !(current->state & THREAD_STATE_BLOCKED)) {
            TRACE_THREAD("SCHED: Continuing with current thread %d", current->tid);
            spl(sr);
            return;
        } else if (p->sleep_queue) {
            // If there are sleeping threads, create an idle thread
            next = get_idle_thread(p);
            TRACE_THREAD("SCHED: Created idle thread to wait for sleeping threads");
        }
    }

    // If thread switch in progress, wait
    if (thread_switch_in_progress) {
        TRACE_THREAD("SCHED: Thread switch in progress, waiting");
        spl(sr);

        proc_thread_schedule();  // Try again later
        return;
    }
    
    // If no current thread, switch directly to next
    if (!current) {
        if (!next) {
            TRACE_THREAD("SCHED: No current thread and no next thread, returning");
            spl(sr);
            return;
        }        
        TRACE_THREAD("SCHED: No current thread, switching to thread %d", next->tid);
        
        // If next is in ready queue, remove it
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }
        // Validate next thread again before switching
        if (next->magic != CTXT_MAGIC || (next->state & THREAD_STATE_EXITED)) {
            TRACE_THREAD("SCHED: Next thread %d is invalid or exited, aborting", next->tid);
            spl(sr);
            return;
        }        
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        // Record scheduling time for timeslice accounting
        next->last_scheduled = get_system_ticks();
        
        CONTEXT *next_ctx = get_thread_context(next);
        if (!next_ctx) {
            TRACE_THREAD("SCHED: Failed to get context for thread %d", next->tid);
            p->current_thread = NULL;  // Reset current thread
            spl(sr);
            return;
        }
        
        spl(sr);
        change_context(next_ctx);
        
        TRACE_THREAD("SCHED: ERROR: Returned from change_context!");
        return;
    }
    
    // Never switch to an EXITED thread
    if (next && (next->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("SCHED: Next thread %d is EXITED, not switching", next->tid);
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }
        spl(sr);
        return;
    }
    
    // If next is current, look for another thread
    if (next == current) {
        // Find another ready thread
        struct thread *alt_next = next->next_ready;
        while (alt_next && alt_next->state != THREAD_STATE_READY) {
            alt_next = alt_next->next_ready;
        }
        
        if (!alt_next) {
            TRACE_THREAD("SCHED: No other ready threads available");
            spl(sr);
            return;
        }
        
        next = alt_next;
    }
    
    // Check if we should schedule next thread
    if (should_schedule_thread(current, next)) {
        TRACE_THREAD("SCHED: Switching threads: %d -> %d", current->tid, next->tid);
        
        // Remove next from ready queue if it's there
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }
        
        // Update timeslice accounting for current thread
        update_thread_timeslice(current);
        
        // Handle thread state changes
        if (!current || !next)
            return;
            
        // Handle current thread based on its state
        if (current->state == THREAD_STATE_RUNNING) {
            // Determine why thread is being switched out
            if (current->wait_type != WAIT_NONE) {
                atomic_thread_state_change(current, THREAD_STATE_BLOCKED);
                
                // Priority inheritance for mutexes
                if (current->wait_type == WAIT_MUTEX && current->wait_obj) {
                    struct mutex *m = (struct mutex*)current->wait_obj;
                    if (m->owner && m->owner->priority < current->priority) {
                        TRACE_THREAD("PRI-INHERIT: Thread %d (pri %d) -> owner %d (pri %d)",
                                    current->tid, current->priority,
                                    m->owner->tid, m->owner->priority);
                        
                        // Boost the mutex owner's priority to the waiting thread's priority
                        boost_thread_priority(m->owner, current->priority - m->owner->priority);
                        
                        // Reinsert owner in ready queue if needed
                        if (m->owner->state == THREAD_STATE_READY) {
                            remove_from_ready_queue(m->owner);
                            add_to_ready_queue(m->owner);
                        }
                    }
                }
            }
            else {
                // Thread is runnable but being preempted
                TRACE_THREAD("THREAD_SCHED: Adding current thread %d to ready queue", current->tid);
                atomic_thread_state_change(current, THREAD_STATE_READY);
                add_to_ready_queue(current);
            }
        }
        
        // Update next thread state and make it current
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        // Record scheduling time for timeslice accounting
        next->last_scheduled = get_system_ticks();
        
        // Perform the context switch
        thread_switch(current, next);
    } else {
        TRACE_THREAD("SCHED: Not switching, current thread %d has higher priority or timeslice remaining",
                    current->tid);
    }
    
    spl(sr);
}

// Thread exit
void proc_thread_exit(void *retval) {
    struct proc *p = curproc;
    if (!p) {
        TRACE_THREAD("EXIT ERROR: No current process");
        return;
    }
    
    struct thread *current = p->current_thread;
    if (!current) {
        TRACE_THREAD("EXIT ERROR: No current thread");
        return;
    }
    
    static int thread_exit_in_progress = 0;
    static int exit_owner_tid = -1;
    unsigned short sr = splhigh();
    
    // Protection against reentrance
    if (thread_exit_in_progress && exit_owner_tid != current->tid) {
        spl(sr);
        TRACE_THREAD("EXIT: Thread exit already in progress by thread %d, waiting", exit_owner_tid);
        proc_thread_exit(retval); // Pass retval to recursive call
        return;
    }
    
    int tid = current->tid;
    thread_exit_in_progress = 1;
    exit_owner_tid = tid;
    
    // Store the return value in the thread structure
    current->retval = retval;
    
    TRACE_THREAD("EXIT: Thread %d beginning exit process", tid);
    
    // Check if any thread is joining this one
    if (current->joiner && current->joiner->magic == CTXT_MAGIC) {
        struct thread *joiner = current->joiner;
        TRACE_THREAD("EXIT: Thread %d is being joined by thread %d", 
                    tid, joiner->tid);
        
        // If joiner is waiting for this thread
        if (joiner->wait_type == WAIT_JOIN && joiner->wait_obj == current) {
            // Wake up the joining thread
            joiner->wait_type = WAIT_NONE;
            joiner->wait_obj = NULL;
            // Store return value directly in joiner's requested location
            if (joiner->join_retval) {
                *(joiner->join_retval) = retval;
                // TRACE_THREAD("EXIT: Storing return value %p for joiner thread %d", retval, joiner->tid);
            }            
            // Mark as joined
            current->joined = 1;
            
            // Wake up joiner
            atomic_thread_state_change(joiner, THREAD_STATE_READY);
            add_to_ready_queue(joiner);
            TRACE_THREAD("EXIT: Woke up joining thread %d", joiner->tid);
        }
    }
    
    // Special handling for thread0 - wait for other threads to complete
    if (current->tid == 0 && p->num_threads > 1) {
        TRACE_THREAD("thread0 waiting for other threads to exit before exiting");
        // Wait until all other threads have exited
        while (p->num_threads > 1) {
            // Yield to other threads
            atomic_thread_state_change(current, THREAD_STATE_READY);
            add_to_ready_queue(current);
            proc_thread_schedule();
            spl(sr);
        }
        TRACE_THREAD("All other threads have exited, thread0 can now exit");
    }
    
    // Check if the thread is already exited or freed
    if (current->magic != CTXT_MAGIC || (current->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("WARNING: proc_thread_exit: Thread %d already exited or freed (magic=%lx, state=%d)",
                    tid, current->magic, current->state);
        thread_exit_in_progress = 0;
        exit_owner_tid = -1;
        spl(sr);
        return;
    }
    
    // CRITICAL: Cancel any timeouts associated with this thread
    // This is crucial to prevent timer callbacks from using the freed thread
    TIMEOUT *timelist, *next_timelist;
    for (timelist = tlist; timelist; timelist = next_timelist) {
        next_timelist = timelist->next;
        if (timelist->proc == p && timelist->arg == (long)current) {
            TRACE_THREAD("Cancelling timeout with thread %d as argument", tid);
            canceltimeout(timelist);
        }
    }

    // Remove from all queues
    remove_thread_from_wait_queues(current);
    remove_from_ready_queue(current);
    
    // Mark thread as exited
    atomic_thread_state_change(current, THREAD_STATE_EXITED);

    // ALWAYS decrement thread count when a thread exits
    p->num_threads--;
    TRACE_THREAD("EXIT: Thread %d exited, num_threads=%d", tid, p->num_threads);
    
    // Handle timers
    if (p->num_threads == 1) {
        TRACE_THREAD("Only one thread remaining, stopping all timers");
        if (thread_switch_timeout) {
            canceltimeout(thread_switch_timeout);
            thread_switch_timeout = NULL;
        }
        if (p->p_thread_timer.enabled) {
            thread_timer_stop(p);
        }
    }
    
    // Find next thread to run
    struct thread *next_thread = NULL;
    CONTEXT *target_ctx = NULL;
    
    // STEP 1: First check the sleep queue for threads that should wake up
    if (p->sleep_queue) {
        unsigned long current_time = get_system_ticks();
        int woke_threads;
        
        TRACE_THREAD("EXIT: Checking sleep queue at time %lu", current_time);
        
        // Use the extracted function to wake threads
        woke_threads = wake_threads_by_time(p, current_time);
        
        if (woke_threads > 0) {
            TRACE_THREAD("EXIT: Woke up %d threads from sleep queue", woke_threads);
        }
    }
    
    // STEP 2: Check the ready queue for the next thread to run
    next_thread = p->ready_queue;
    
    // Make sure the next thread is valid
    while (next_thread && (next_thread->magic != CTXT_MAGIC || 
                          (next_thread->state & THREAD_STATE_EXITED))) {
        TRACE_THREAD("EXIT: Skipping invalid thread %d in ready queue", next_thread->tid);
        remove_from_ready_queue(next_thread);
        next_thread = p->ready_queue;
    }
    
    if (next_thread) {
        TRACE_THREAD("EXIT: Found next thread %d in ready queue", next_thread->tid);
        remove_from_ready_queue(next_thread);
    }
    else {
        // STEP 3: If no thread in ready queue, try to find thread0 (if we're not thread0)
        if (tid != 0) {
            TRACE_THREAD("EXIT: No ready threads, looking for thread0");
            struct thread *t;
            int count = 0;
            for (t = p->threads; t != NULL && count < p->num_threads; t = t->next, count++) {
                if (t->tid == 0 && 
                    t->magic == CTXT_MAGIC && 
                    !(t->state & THREAD_STATE_EXITED) &&
                    !(t->wait_type == WAIT_JOIN)) {
                    next_thread = t;
                    TRACE_THREAD("EXIT: Found thread0 at %p, state=%d, wait_type=%d", next_thread, next_thread->state, next_thread->wait_type);
                    break;
                }
            }
        }
    }
    
    // If we found a valid next thread, prepare to switch to it
    if (next_thread && next_thread->magic == CTXT_MAGIC && !(next_thread->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("EXIT: Will switch to thread %d", next_thread->tid);
        atomic_thread_state_change(next_thread, THREAD_STATE_RUNNING);
        p->current_thread = next_thread;
        target_ctx = get_thread_context(next_thread);
        
        if (!target_ctx) {
            TRACE_THREAD("EXIT ERROR: Could not get context for thread %d", next_thread->tid);
            next_thread = NULL;
            p->current_thread = NULL;
        }
    }
    
    // STEP 4: If no valid thread found, use process context
    if (!target_ctx) {
        TRACE_THREAD("EXIT: No next thread, returning to process context");
        target_ctx = &p->ctxt[CURRENT];
        p->current_thread = NULL;
    }
    
    // Store thread info before potentially freeing it
    struct thread *old_thread = current;
    int should_free = old_thread->detached || old_thread->joined;
    
    // Remove from thread list if detached or joined
    if (should_free) {
        struct thread **tp;
        for (tp = &p->threads; *tp; tp = &(*tp)->next) {
            if (*tp == old_thread) {
                *tp = old_thread->next;
                break;
            }
        }
    }
    
    // Clear current_thread pointer to prevent use after free
    if (p->current_thread == old_thread) {
        p->current_thread = NULL;
    }
    
    // Free resources if detached or joined
    if (old_thread && old_thread->magic == CTXT_MAGIC && should_free) {
        if (old_thread->t_sigctx) {
            kfree(old_thread->t_sigctx);
            TRACE_THREAD("EXIT: Freed signal context for thread %d", tid);
            old_thread->t_sigctx = NULL;
        }
        
        if (old_thread->stack && tid != 0) {
            kfree(old_thread->stack);
            old_thread->stack = NULL;
        }
        
        TRACE_THREAD("EXIT: Freed resources for thread %d", tid);
        
        // Clear magic BEFORE freeing to prevent use after free
        old_thread->magic = 0;
        
        kfree(old_thread);
        TRACE_THREAD("EXIT: KFREE thread %d", tid);
    } else {
        TRACE_THREAD("EXIT: Thread %d not detached or joined, keeping resources", tid);
    }
    
    TRACE_THREAD("Thread %d exited", tid);
    
    thread_exit_in_progress = 0;
    exit_owner_tid = -1;
    
    // Switch to target context
    TRACE_THREAD("EXIT: Switching to target context, PC=%lx", target_ctx->pc);
    
    // CRITICAL FIX: Do NOT save context when exiting!
    // The exiting thread should never resume - just switch directly
    spl(sr);
    change_context(target_ctx);
    
    // Should NEVER reach here - if we do, it's a critical error
    TRACE_THREAD("CRITICAL ERROR: Returned from change_context after thread exit!");
    
    // Try to recover by scheduling another thread
    proc_thread_schedule();
}

/**
 * Helper function to determine if a thread should be scheduled
 * 
 * This function implements POSIX-compliant scheduling policies:
 * - SCHED_FIFO: First-in, first-out scheduling without time slicing
 * - SCHED_RR: Round-robin scheduling with time slicing
 * - SCHED_OTHER: Default time-sharing scheduling
 * 
 * @param current The currently running thread
 * @param next The candidate thread to be scheduled next
 * @return 1 if next should preempt current, 0 otherwise
 */
static int should_schedule_thread(struct thread *current, struct thread *next) {
    if (!next) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Invalid thread");
        return 0;
    }
    
    // If no current thread, always schedule next thread
    if (!current) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): No current thread, scheduling next thread %d", 
                    next->tid);
        return 1;
    }

    /* Special case: thread0 is always preemptible by other threads */
    if (current->tid == 0 && next->tid != 0) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): thread0 is current, allowing switch to thread %d", next->tid);
        return 1;
    }

    /* If current thread is not running, always schedule next thread */
    if ((current->state != THREAD_STATE_RUNNING) || (current->state & THREAD_STATE_BLOCKED)) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Current thread %d is not running, scheduling next thread %d",
                    current->tid, next->tid);
        return 1;
    }

    /* Calculate elapsed time once */
    unsigned long elapsed = get_system_ticks() - current->last_scheduled;

    /* PRIORITY CHECK FIRST - Higher priority always preempts */
    if (next->priority > current->priority) {
        /* But respect minimum timeslice for non-boosted threads */
        if (!next->priority_boost && elapsed < next->proc->thread_min_timeslice) {
            TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Higher priority thread %d waiting for min timeslice",
                        next->tid);
            return 0;
        }
        
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Higher priority thread %d (pri %d%s) preempting thread %d (pri %d)",
                    next->tid, next->priority, next->priority_boost ? " boosted" : "",
                    current->tid, current->priority);
        return 1;
    }

    /* Check minimum timeslice for equal/lower priority */
    if (elapsed < current->proc->thread_min_timeslice) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Current thread %d hasn't used minimum timeslice (%lu < %d)",
                    current->tid, elapsed, current->proc->thread_min_timeslice);
        return 0;
    }

    /* RT threads always preempt SCHED_OTHER threads (regardless of priority) */
    if ((next->policy == SCHED_FIFO || next->policy == SCHED_RR) &&
        next->priority > 0 && current->policy == SCHED_OTHER) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): RT thread %d (pri %d) preempting SCHED_OTHER thread %d",
                    next->tid, next->priority, current->tid);
        return 1;
    }

    /* Equal priority handling */
    if (next->priority == current->priority) {
        /* SCHED_FIFO threads continue running until preempted by higher priority */
        if (current->policy == SCHED_FIFO) {
            TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_FIFO thread %d continues (equal priority)",
                        current->tid);
            return 0;
        }

        /* SCHED_RR and SCHED_OTHER use timeslice */
        if (elapsed >= current->timeslice) {
            TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Thread %d timeslice expired (%lu >= %d), switching to %d",
                        current->tid, elapsed, current->timeslice, next->tid);
            return 1;
        }

        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Thread %d timeslice not expired (%lu < %d)",
                    current->tid, elapsed, current->timeslice);
        return 0;
    }

    /* Lower priority threads don't preempt higher priority ones */
    return 0;
}

static void thread_switch(struct thread *from, struct thread *to) {
    unsigned short sr;
    
    // Fast validation first
    if (!to) {
        TRACE_THREAD("SWITCH: Invalid destination thread pointer");
        return;
    }
    
    // Special case: if from is NULL, just switch to the destination thread
    if (!from) {
        TRACE_THREAD("SWITCH: Switching to thread %d (no source thread)", to->tid);
        sr = splhigh();
        change_context(get_thread_context(to));
        spl(sr);
        return;
    }
    
    if (from == to) {
        TRACE_THREAD("SWITCH: Source and destination threads are the same");
        return;
    }
    
    TRACE_THREAD("SWITCH: Switching from %d to %d", from->tid, to->tid);
    // Check magic numbers and states in one go
    if (from->magic != CTXT_MAGIC || to->magic != CTXT_MAGIC ||
        (from->state & THREAD_STATE_EXITED) || (to->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("SWITCH: Invalid thread magic or exited state: from=%d, to=%d", from->tid, to->tid);
        return;
    }
    
    // Special handling for thread0
    if (from->tid == 0 && (from->state & THREAD_STATE_EXITED) && from->proc->num_threads > 1) {
        TRACE_THREAD("SWITCH: Preventing thread0 exit while other threads running");
        atomic_thread_state_change(from, THREAD_STATE_READY);
        return;
    }
    
    sr = splhigh();
    
    // Check if another switch is in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("SWITCH: Another switch in progress, aborting");
        spl(sr);
        return;
    }
    
    // Set switch in progress and setup deadlock detection
    thread_switch_in_progress = 1;
    
    if (thread_switch_timeout) {
        canceltimeout(thread_switch_timeout);
    }
    thread_switch_timeout = addtimeout(from->proc, ((from->proc->thread_default_timeslice * MS_PER_TICK) / 20), 
                                      thread_switch_timeout_handler);
    
    TRACE_THREAD("SWITCH: Switching threads: %d -> %d", from->tid, to->tid);
    
    // Verify stack integrity
    if (from->stack_magic != STACK_MAGIC || to->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("SWITCH ERROR: Stack corruption detected!");
        thread_switch_in_progress = 0;
        spl(sr);
        return;
    }
    
    /* FIXED: Only reset priority boost when switching AWAY from a thread that has run,
     * not when switching TO a boosted thread that hasn't run yet */
    if (from->priority_boost && from->tid != 0) {
        /* Only reset boost if thread has actually consumed some CPU time */
        unsigned long elapsed = get_system_ticks() - from->last_scheduled;
        if (elapsed > from->proc->thread_min_timeslice || from->wait_type != WAIT_NONE) {
            TRACE_THREAD("SWITCH: Resetting priority boost for thread %d (current pri: %d, original: %d, elapsed: %lu)",
                        from->tid, from->priority, from->original_priority, elapsed);
            reset_thread_priority(from);
        } else {
            TRACE_THREAD("SWITCH: Keeping priority boost for thread %d (elapsed: %lu < min: %d)",
                        from->tid, elapsed, from->proc->thread_min_timeslice);
        }
    }

    // Get destination context once
    CONTEXT *to_ctx = get_thread_context(to);
    if (!to_ctx) {
        reset_thread_switch_state();
        spl(sr);
        return;
    }
    
    // Handle context switch based on thread state
    if (from->wait_type == WAIT_SLEEP  || from->wait_type == WAIT_JOIN) {
        // Sleeping thread path - direct context switch
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        
        reset_thread_switch_state();
        spl(sr);
        change_context(to_ctx);
        
        TRACE_THREAD("SWITCH ERROR: Returned from change_context!");
    } 
    else 
    if (save_context(&from->ctxt[CURRENT]) == 0) {

        // Only change state if not blocked on mutex/semaphore
        if (from->wait_type == WAIT_NONE) {
            atomic_thread_state_change(from, THREAD_STATE_READY);
        }
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        
        reset_thread_switch_state();
        spl(sr);
        change_context(to_ctx);
        
        TRACE_THREAD("SWITCH ERROR: Returned from change_context!");
    }
    
    // Return path after being switched back
    reset_thread_switch_state();
    spl(sr);
}

/**
 * Helper function to reschedule the preemption timer
 */
void reschedule_preemption_timer(PROC *p, long arg) {
    if (!p){
        TRACE_THREAD("SCHED_TIMER: Invalid process reference");
        return;
    }

    // Cancel existing timeout first
    if (p->p_thread_timer.timeout) {
        canceltimeout(p->p_thread_timer.timeout);
        p->p_thread_timer.timeout = NULL;
    }

    struct thread *t = (struct thread *)arg;
    // TRACE_THREAD("SCHED_TIMER: Rescheduling preemption timer for process %d, arg tid %d", p->pid, t->tid);
    p->p_thread_timer.timeout = addtimeout(p, p->thread_preempt_interval, thread_preempt_handler);
    p->p_thread_timer.in_handler = 0;
    if (p->p_thread_timer.timeout) {
        p->p_thread_timer.timeout->arg = (long)t;
    } else {
        TRACE_THREAD("SCHED_TIMER: Failed to reschedule preemption timer for process %d", p->pid);
    }
}

/*
 * Reset the thread switch state
 * Called when a thread switch completes or times out
 */
void reset_thread_switch_state(void) {

    unsigned short sr = splhigh();
    
    thread_switch_in_progress = 0;
    
    if (thread_switch_timeout) {
        canceltimeout(thread_switch_timeout);
        thread_switch_timeout = NULL;
    }
    
    spl(sr);
}

/*
 * Thread switch timeout handler
 * Called when a thread switch takes too long
 */
void thread_switch_timeout_handler(PROC *p, long arg) {

    static int recovery_attempts = 0;
    
    TRACE_THREAD("TIMEOUT: Thread switch timed out after %dms", ((p->thread_default_timeslice * 5) / 20));
    
    if (++recovery_attempts > MAX_SWITCH_RETRIES) {
        TRACE_THREAD("CRITICAL: Max recovery attempts reached, system may be unstable");
        
        // More aggressive recovery - try to restore a known good state
        if (p && p->current_thread && p->current_thread->magic == CTXT_MAGIC) {
            TRACE_THREAD("TIMEOUT: Attempting to restore current thread %d", 
                        p->current_thread->tid);

            change_context(get_thread_context(p->current_thread));

        }
        
        recovery_attempts = 0;
    }
    
    /* Force reset of thread switch state */
    reset_thread_switch_state();
    
    /* Schedule next thread to try to recover */
    proc_thread_schedule();
}

/*
 * Start timing a specific process/thread
 */
void thread_timer_start(struct proc *p, int thread_id) {
    unsigned short sr, retry_count = 0;
    
    TRACE_THREAD("TIMER: thread_timer_start called for process %d", p->pid);
    if (!p)
        return;

    /* Try to acquire the timer operation lock with a timeout */
    while (1) {
        sr = splhigh();
        if (!timer_operation_locked) {
            TRACE_THREAD("TIMER: Acquired timer operation lock");
            timer_operation_locked = 1;
            break;
        }
        spl(sr);
        
        /* If we've tried too many times, give up */
        if (++retry_count > 10) {
            TRACE_THREAD("TIMER WARNING: Failed to acquire timer operation lock after 10 retries");
            return;
        }
    }

    /* CRITICAL SECTION - We now have the timer operation lock */
    TRACE_THREAD("TIMER: Starting thread timer for process %d", p->pid);
    
    sr = splhigh();
    
    /* If timer is already enabled, don't add another timeout */
    if (p->p_thread_timer.enabled && p->p_thread_timer.timeout) {
        TRACE_THREAD("TIMER: Timer already enabled, not adding another timeout");
        spl(sr);
        goto cleanup;
    }
    
    /* Create the timeout before modifying any state */
    p->p_thread_timer.timeout = addtimeout(p, p->thread_preempt_interval, thread_preempt_handler);
    if (!(p->p_thread_timer.timeout)) {
        TRACE_THREAD("TIMER ERROR: Failed to create timeout");
        spl(sr);
        goto cleanup;
    }
    p->p_thread_timer.timeout->arg = (long)p->current_thread;

    /* Now that we have a valid timeout, update the timer state */
    p->p_thread_timer.thread_id = p->current_thread->tid;

    /* Set enabled flag last to ensure everything is set up */
    p->p_thread_timer.enabled = 1;
    p->p_thread_timer.in_handler = 0;
    
    TRACE_THREAD("TIMER: Thread timer started for process %d", p->pid);
    spl(sr);
    
cleanup:
    /* Always release the timer operation lock */
    sr = splhigh();
    timer_operation_locked = 0;
    spl(sr);
    
    TRACE_THREAD("TIMER: Timer timer operation lock released");
}

/*
 * Stop the thread timer
 */
void thread_timer_stop(PROC *p)
{
    unsigned short sr;
    TIMEOUT *timeout_to_cancel = NULL;
    int retry_count = 0;

    if (!p) {
        return;
    }

    /* Try to acquire the timer operation lock with a timeout */
    while (1) {
        sr = splhigh();
        if (!timer_operation_locked) {
            timer_operation_locked = 1;
            spl(sr);
            break;
        }
        spl(sr);
        
        /* If we've tried too many times, give up */
        if (++retry_count > 10) {
            TRACE_THREAD("WARNING: Failed to acquire timer operation lock after 10 retries");
            return;
        }

    }

    /* CRITICAL SECTION - We now have the operation lock */
    TRACE_THREAD("Stopping thread timer for process %d", p->pid);
    
    sr = splhigh();
    
    /* Check if timer is already disabled */
    if (p->num_threads <= 1) {
        TRACE_THREAD("TIMER: Disabling timer for process %d (num_threads=%d)", p->pid, p->num_threads);
        p->p_thread_timer.enabled = 0;
    }
    
    /* Save the timeout pointer locally before clearing it */
    timeout_to_cancel = p->p_thread_timer.timeout;
    p->p_thread_timer.timeout = NULL;
    TRACE_THREAD("TIMER: Cleared timeout pointer");
    
    spl(sr);
    
    /* Cancel the timeout outside the critical section */
    if (timeout_to_cancel) {
        canceltimeout(timeout_to_cancel);
        TRACE_THREAD("TIMER: Cancelled timeout");
    }
    
    /* Always release the timer operation lock */
    sr = splhigh();
    timer_operation_locked = 0;
    spl(sr);
    
    TRACE_THREAD("TIMER: Timer operation lock released");
}
