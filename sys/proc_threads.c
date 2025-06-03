#include "proc_threads.h"

/* Threads stuff */

void thread_switch(struct thread *from, struct thread *to);
long create_thread(struct proc *p, void (*func)(void*), void *arg);
void init_thread_context(struct thread *t, void (*func)(void*), void *arg);
void thread_start(void);

void thread_exit(void *retval);
void init_thread0(struct proc *p);

void thread_preempt_handler(PROC *p, long arg);
void thread_timer_start(struct proc *p, int thread_id);
void thread_timer_stop(PROC *p);

void thread_switch_timeout_handler(PROC *p, long arg);
void reset_thread_switch_state(void);

void check_and_wake_sleeping_threads(struct proc *p);
void thread_sleep_wakeup_handler(PROC *p, long arg);

static CONTEXT* get_thread_context(struct thread *t);

void reschedule_preemption_timer(PROC *p, long arg);
struct thread *get_highest_priority_thread(struct proc *p);
int should_schedule_thread(struct thread *current, struct thread *next);

void make_process_eligible(struct proc *p);

/* Mutex for timer operations */
static int timer_operation_locked = 0;

static int thread_switch_in_progress = 0;

static TIMEOUT *thread_switch_timeout = NULL;

/*
 * Reset the thread switch state
 * Called when a thread switch completes or times out
 */
void reset_thread_switch_state(void)
{
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
void thread_switch_timeout_handler(PROC *p, long arg)
{
    static int recovery_attempts = 0;
    
    TRACE_THREAD("TIMEOUT: Thread switch timed out after %dms", ((p->thread_default_timeslice * 5) / 20));
    
    if (++recovery_attempts > MAX_SWITCH_RETRIES) {
        TRACE_THREAD("CRITICAL: Max recovery attempts reached, system may be unstable");
        
        // More aggressive recovery - try to restore a known good state
        if (p && p->current_thread && p->current_thread->magic == CTXT_MAGIC) {
            TRACE_THREAD("TIMEOUT: Attempting to restore current thread %d", 
                        p->current_thread->tid);
            
            // Try to restore the current thread's context
            change_context(&p->current_thread->ctxt[CURRENT]);
        }
        
        recovery_attempts = 0;
    }
    
    /* Force reset of thread switch state */
    reset_thread_switch_state();
    
    /* Schedule next thread to try to recover */
    schedule();
}

/*
 * Atomic thread state change function
 * Ensures that thread state transitions are atomic operations
 * to prevent race conditions between concurrent threads
 */
void atomic_thread_state_change(struct thread *t, int new_state)
{
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
 * Start timing a specific process/thread
 */
void thread_timer_start(struct proc *p, int thread_id)
{
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
    
    // Dump current ready queue for debugging
    TRACE_THREAD("READY_Q: Current queue before adding thread %d:", t->tid);
    curr = p->ready_queue;
    while (curr) {
        TRACE_THREAD("  Thread %d (pri %d, policy %d, boost=%d)", 
                    curr->tid, curr->priority, curr->policy, curr->priority_boost);
        curr = curr->next_ready;
    }
    
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
    
    // Dump final ready queue for debugging
    TRACE_THREAD("READY_Q: Final queue after adding thread %d:", t->tid);
    curr = p->ready_queue;
    while (curr) {
        TRACE_THREAD("  Thread %d (pri %d, policy %d, boost=%d)", 
                    curr->tid, curr->priority, curr->policy, curr->priority_boost);
        curr = curr->next_ready;
    }
    
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

void thread_switch(struct thread *from, struct thread *to)
{
    unsigned short sr;
    
    // Fast validation first
    if (!from || !to || from == to) {
        TRACE_THREAD("SWITCH: Invalid thread pointers - from=%p, to=%p", from, to);
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
    thread_switch_timeout = addtimeout(from->proc, ((from->proc->thread_default_timeslice * 5) / 20), 
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
            from->priority = from->original_priority;
            from->priority_boost = 0;
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
    if (from->wait_type == WAIT_SLEEP) {
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
 * Thread preemption handler
 * 
 * This function is called periodically to implement preemptive multitasking.
 * It checks if the current thread should be preempted and schedules another thread if needed.
 */
void thread_preempt_handler(PROC *p, long arg)
{
    unsigned short sr;
    struct thread *thread_arg = (struct thread *)arg;
    
    if (!p) {
        TRACE_THREAD("PREEMPT: Invalid process pointer");
        return;
    }

    // If not current process, reschedule the timeout
    if (p != curproc) {
        make_process_eligible(p);
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
    
    // Only preempt if multiple threads exist and current thread is valid
    if (p->num_threads <= 1 || !p->current_thread) {
        TRACE_THREAD("PREEMPT: Only one thread or invalid current thread, not preempting");
        reschedule_preemption_timer(p, (long)p->current_thread);
        spl(sr);
        return;
    }
    
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

    // Check for ready threads
    check_and_wake_sleeping_threads(p);
    struct thread *next = get_highest_priority_thread(p);
    
    if (!next) {
        TRACE_THREAD("PREEMPT: No other threads to run, continuing with current thread %d", curr_thread->tid);
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

// Thread creation
long create_thread(struct proc *p, void (*func)(void*), void *arg) {
    unsigned short sr;
    
    // Allocate thread structure
    sr = splhigh();  // Disable interrupts before allocation to prevent race conditions
    
    // Check if process is valid
    if (!p || p->magic != CTXT_MAGIC) {
        spl(sr);
        return -EINVAL;
    }
    
    struct thread *t = kmalloc(sizeof(struct thread));
    if (!t) {
        return -ENOMEM;
    }
    TRACE_THREAD("KMALLOC: Allocated thread structure at %p", t);
    TRACE_THREAD("Creating thread: pid=%d, func=%p, arg=%p", p->pid, func, arg);
    
    // Basic initialization
    // memset(t, 0, sizeof(struct thread));
    mint_bzero (t, sizeof(*t));

    t->tid = p->total_threads++;
    p->num_threads++;
    // Link into process
    t->proc = p;

    t->priority = p->pri;
    t->original_priority = p->pri;
    t->policy = DEFAULT_SCHED_POLICY;
    t->timeslice = t->proc->thread_default_timeslice;
    t->remaining_timeslice = t->proc->thread_default_timeslice;
    t->last_scheduled = 0;
    t->priority_boost = (t->tid > 0) ? 1 : 0;
    if (t->tid > 0) {
        t->priority = THREAD_CREATION_PRIORITY_BOOST;
        TRACE_THREAD("Applied priority boost to new thread %d: priority %d", 
                    t->tid, t->priority);        
    }

    /* Initialize signal fields */
    t->t_sigpending = 0;
    t->t_sigmask = p->p_sigmask;  /* Inherit process signal mask */
    t->t_sig_in_progress = 0;
    t->t_sigctx = NULL;

    // Allocate stack
    t->stack = kmalloc(STKSIZE);
    if (!t->stack) {
		p->num_threads--;  // Revert the thread count increment
        TRACE_THREAD("KFREE: Stack allocation failed");
        kfree(t);
        spl(sr);
        return -ENOMEM;
    }
    TRACE_THREAD("KMALLOC: Allocated stack at %p for thread %d", t->stack, t->tid);
    
    t->stack_top = (char*)t->stack + STKSIZE;
    t->stack_magic = STACK_MAGIC;
    t->magic = CTXT_MAGIC;
    
    t->wakeup_time = 0;  // No wakeup time initially
    t->next_sleeping = NULL;  // Not in sleep queue initially
    t->next_ready = NULL;  // Not in ready queue initially
    t->next = NULL;  // Not linked to any other thread yet
    t->wait_type = WAIT_NONE;  // Not waiting for anything initially
    t->sleep_reason = 0;  // No sleep reason initially
    
    // Initialize join-related fields
    t->retval = NULL;
    t->joiner = NULL;
    t->detached = 0;  // Default is joinable
    t->joined = 0;

    TRACE_THREAD("Thread %d stack: base=%p, top=%p", t->tid, t->stack, t->stack_top);
    
    // Initialize context
    init_thread_context(t, func, arg);
    
    atomic_thread_state_change(t, THREAD_STATE_READY);
    add_to_ready_queue(t);

    if(!p->p_thread_timer.enabled){
        thread_timer_start(t->proc, t->tid);
    }

    TRACE_THREAD("Thread %d created successfully", t->tid);
    spl(sr);
    return t->tid;
}

void init_thread_context(struct thread *t, void (*func)(void*), void *arg) {
    TRACE_THREAD("INIT CONTEXT: Initializing context for thread %d", t->tid);
    
    // Clear context
    // memset(&t->ctxt[CURRENT], 0, sizeof(struct context));
    mint_bzero (&t->ctxt[CURRENT], sizeof(t->ctxt[CURRENT]));
    // memset(&t->ctxt[SYSCALL], 0, sizeof(struct context));
    mint_bzero (&t->ctxt[SYSCALL], sizeof(t->ctxt[SYSCALL]));
    
    // Set up stack pointers - ensure proper alignment with more space
    unsigned long ssp = ((unsigned long)t->stack_top - 128) & ~3L;
    unsigned long usp = ((unsigned long)t->stack_top - 256) & ~3L;
    
    TRACE_THREAD("Stack pointers: SSP=%lx, USP=%lx", ssp, usp);
    
    // Store function and argument in thread structure
    t->func = func;
    t->arg = arg;
    
    // Set up initial context
    t->ctxt[CURRENT].ssp = ssp;
    t->ctxt[CURRENT].usp = usp;
    t->ctxt[CURRENT].pc = (unsigned long)thread_start;
    t->ctxt[CURRENT].sr = 0x2000;  // Supervisor mode (0x2000)
    
    // Store thread pointer in D0 register
    t->ctxt[CURRENT].regs[0] = (unsigned long)t;
    
    // Create a proper exception frame for RTE
    unsigned short *frame_ptr = (unsigned short *)(ssp - 8);
    frame_ptr[0] = 0x0000;  // Format/Vector
    frame_ptr[1] = 0x2000;  // SR (Supervisor mode)
    frame_ptr[2] = (unsigned short)((unsigned long)thread_start >> 16);
    frame_ptr[3] = (unsigned short)((unsigned long)thread_start);
    
    // Update SSP to point to our exception frame
    t->ctxt[CURRENT].ssp = (unsigned long)frame_ptr;
    
    // Copy to SYSCALL context
    memcpy(&t->ctxt[SYSCALL], &t->ctxt[CURRENT], sizeof(struct context));

    TRACE_THREAD("INIT CONTEXT: Thread id. %d initialized for process %d",t->tid, t->proc->pid);
    TRACE_THREAD("Exception frame created at %p:", frame_ptr);
    TRACE_THREAD("  SR = %04x (supervisor mode)", frame_ptr[1]);
    TRACE_THREAD("  PC = %04x%04x", frame_ptr[2], frame_ptr[3]);
    TRACE_THREAD("  Thread pointer stored in D0 = %p", t);     
}

// Thread start function
void thread_start(void) {
    struct thread *t;
    struct proc *p;
    
    // Get thread pointer from D0 register
    asm volatile (
        "move.l %%d0,%0"
        : "=m" (t)
        :
        : "memory"
    );
    
    TRACE_THREAD("START: Thread trampoline started");
    
    if (!t || t->magic != CTXT_MAGIC) {
        TRACE_THREAD("START: Invalid thread pointer %p or magic %lx", t, t ? t->magic : 0);
        return;
    }
    
    p = t->proc;
    if (!p) {
        TRACE_THREAD("START: No process for thread %d", t->tid);
        return;
    }
    
    TRACE_THREAD("START: Current thread is %d", t->tid);
    
    // Start preemption timer if needed
    if (p->num_threads > 1 && !p->p_thread_timer.enabled) {
        TRACE_THREAD("START: Starting thread timer for process %d (thread %d)", p->pid, t->tid);
        thread_timer_start(p, t->tid);
    }
    
    // Get function and argument from thread structure
    void (*func)(void*) = t->func;
    void *arg = t->arg;
    
    TRACE_THREAD("START: Thread function=%p, arg=%p", func, arg);
    
    // Call the function
    if (func) {
        TRACE_THREAD("START: Calling thread function");
        func(arg);
        TRACE_THREAD("START: Thread function returned");
    }
    
    if (t && t->magic == CTXT_MAGIC) {
        TRACE_THREAD("START: Thread %d is exiting", t->tid);
        thread_exit(NULL);
    }
    
    // Should never reach here
    TRACE_THREAD("START: Thread didn't exit properly!");
    while(1) { /* hang */ }
}

/**
 * Schedule a new thread to run
 * 
 * This function implements the core scheduling algorithm for threads.
 * It selects the next thread to run based on priority and scheduling policy,
 * and performs the context switch if needed.
 */
void schedule(void)
{
    struct proc *p = curproc;
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
        struct thread *thread0 = NULL;
        for (struct thread *t = p->threads; t != NULL; t = t->next) {
            if (t->tid == 0 && t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED)) {
                thread0 = t;
                break;
            }
        }
        
        if (thread0 && thread0->state == THREAD_STATE_READY) {
            next = thread0;
            TRACE_THREAD("SCHED: Falling back to thread0");
        } else if (current && !(current->state & THREAD_STATE_BLOCKED)) {
            TRACE_THREAD("SCHED: Continuing with current thread %d", current->tid);
            spl(sr);
            return;
        }
    }

    // If still no next thread, just return
    if (!next) {
        TRACE_THREAD("SCHED: No thread to schedule, returning");
        spl(sr);
        return;
    }

    // If thread switch in progress, wait
    if (thread_switch_in_progress) {
        TRACE_THREAD("SCHED: Thread switch in progress, waiting");
        spl(sr);

        schedule();  // Try again later
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
                        
                        // Save original priority if not already boosted
                        if (!m->owner->priority_boost) {
                            m->owner->original_priority = m->owner->priority;
                        }
                        m->owner->priority = current->priority;
                        m->owner->priority_boost = 1;
                        
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
void thread_exit(void *retval) {
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
        thread_exit(retval);  // Pass retval to recursive call
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
            schedule();
            spl(sr);
        }
        TRACE_THREAD("All other threads have exited, thread0 can now exit");
    }
    
    // Check if the thread is already exited or freed
    if (current->magic != CTXT_MAGIC || (current->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("WARNING: thread_exit: Thread %d already exited or freed (magic=%lx, state=%d)",
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
    
    remove_thread_from_wait_queues(current);
    remove_from_ready_queue(current);
    
    atomic_thread_state_change(current, THREAD_STATE_EXITED);
    
    // Remove from thread list
    struct thread **tp;
    for (tp = &p->threads; *tp; tp = &(*tp)->next) {
        remove_from_sleep_queue(p, *tp);
        if (*tp == current) {
            *tp = current->next;
            break;
        }
    }
    
    // Update thread count
    p->num_threads--;
    
    // Handle timers
    TRACE_THREAD("EXIT: Thread %d removed from thread list, num_threads=%d", tid, p->num_threads);
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
        struct thread *sleep_t = p->sleep_queue;
        int woke_threads = 0;
        
        TRACE_THREAD("EXIT: Checking sleep queue at time %lu", current_time);
        
        while (sleep_t) {
            struct thread *next_sleep = sleep_t->next_sleeping;
            
            // If thread should wake up
            if (sleep_t->magic == CTXT_MAGIC && 
                !(sleep_t->state & THREAD_STATE_EXITED) &&
                sleep_t->wakeup_time > 0 && 
                sleep_t->wakeup_time <= current_time) {
                
                TRACE_THREAD("EXIT: Thread %d should wake up (wake at %lu)",
                            sleep_t->tid, sleep_t->wakeup_time);
                            
                // Wake up thread
                sleep_t->wait_type = WAIT_NONE;
                sleep_t->wakeup_time = 0;  // Clear wake-up time
                atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
                add_to_ready_queue(sleep_t);
                woke_threads++;
            }
            
            sleep_t = next_sleep;
        }
        
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
            for (t = p->threads; t != NULL; t = t->next) {
                if (t->tid == 0 && t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED)) {
                    next_thread = t;
                    TRACE_THREAD("EXIT: Found thread0 at %p", next_thread);
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
    
    // Store a copy of the thread pointer before nullifying it
    struct thread *old_thread = current;
    
    // If thread is detached or has been joined, free resources
    // Otherwise, keep resources for joining thread to access
    if (old_thread && old_thread->magic == CTXT_MAGIC) {
        if (old_thread->detached || old_thread->joined) {
            // Free resources for detached or joined threads
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
            old_thread = NULL;
        } else {
            // Thread is not detached and not joined yet
            // Keep resources for joining thread to access
            TRACE_THREAD("EXIT: Thread %d not detached or joined, keeping resources", tid);
        }
        
        TRACE_THREAD("Thread %d exited", tid);
    } else {
        TRACE_THREAD("EXIT ERROR: Attempt to exit thread id %d, but No current thread to free", tid);
    }
    
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
    schedule();
}

void init_thread0(struct proc *p) {
    if (p->current_thread) return; // Already initialized
    
    struct thread *t0 = kmalloc(sizeof(struct thread));

    if (!t0) return;
    TRACE_THREAD("KMALLOC: Allocated thread0 structure at %p for process %d", t0, p->pid);
    TRACE_THREAD("INIT CONTEXT: Initializing thread0 for process %d", p->pid);
    
    // memset(t0, 0, sizeof(struct thread));
    mint_bzero (t0, sizeof(*t0));
    t0->tid = 0;
    t0->proc = p;
    // t0->priority = p->pri;
    t0->priority = 0;
    t0->original_priority = 0;

    // Use process stack for thread0
    t0->stack = p->stack;
    t0->stack_top = (char*)p->stack + STKSIZE;
    t0->stack_magic = STACK_MAGIC;
    
    // Copy process context to thread context
    memcpy(&t0->ctxt[CURRENT], &p->ctxt[CURRENT], sizeof(CONTEXT));
    memcpy(&t0->ctxt[SYSCALL], &p->ctxt[SYSCALL], sizeof(CONTEXT));
    
    // Link into process
    t0->next = NULL;

    t0->magic = CTXT_MAGIC;

    p->threads = t0;
    p->current_thread = t0;
    p->num_threads = 1;
    p->total_threads = 1;

    atomic_thread_state_change(t0, THREAD_STATE_RUNNING);
    TRACE_THREAD("INIT CONTEXT: Thread id. %d initialized for process %d",t0->tid, p->pid);
    TRACE_THREAD("INIT CONTEXT: Starting thread timer for process %d", p->pid);
    thread_timer_start(t0->proc, t0->tid);
}

// Thread creation syscall
long _cdecl sys_p_createthread(void (*func)(void*), void *arg, void *stack)
{    
    TRACE_THREAD("CREATETHREAD: func=%p arg=%p stack=%p", func, arg, stack);

    init_thread0(curproc);
    return create_thread(curproc, func, arg);
}

/**
 * Thread exit/join/detach syscall
 * 
 * @param mode Operation mode: THREAD_EXIT, THREAD_JOIN, or THREAD_DETACH
 * @param arg1 For join/detach: thread ID, for exit: return value (cast to void*)
 * @param arg2 For join: pointer to store return value
 * @return 0 on success, error code on failure
 */
long _cdecl sys_p_exitthread(long mode, long arg1, long arg2) {
    switch (mode) {
        case THREAD_EXIT: // Exit thread
            TRACE_THREAD("EXIT: sys_p_exitthread called with exit mode");
            thread_exit((void*)arg1);  // Use arg1 as the return value
            return 0;  // Should never reach here
            
        case THREAD_JOIN: // Join thread
            TRACE_THREAD("JOIN: sys_p_jointhread called for tid=%ld", arg1);
            return sys_p_jointhread(arg1, (void**)arg2);
            
        case THREAD_DETACH: // Detach thread
            TRACE_THREAD("DETACH: sys_p_detachthread called for tid=%ld", arg1);
            return sys_p_detachthread(arg1);
            
        default:
            TRACE_THREAD("ERROR: sys_p_exitthread called with invalid mode %d", mode);
            return -EINVAL;
    }
}


int is_in_ready_queue(struct thread *t)
{
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

long _cdecl sys_p_sleepthread(long ms)
{
    struct proc *p = curproc;
    struct thread *t = p ? p->current_thread : NULL;
    unsigned short sr;
    TIMEOUT *sleep_timeout = NULL;
    
    if (!p || !t) return -EINVAL;
    if (t->tid == 0) return -EINVAL; // thread0 can't sleep
    if (ms <= 0) return 0; // No need to sleep

    // Prevent nested blocking
    if (t->wait_type != WAIT_NONE) {
        TRACE_THREAD("SLEEP: Thread %d already blocked", t->tid);
        return EDEADLK;
    }

    TRACE_THREAD("SLEEP: Thread %d sleeping for %ld ms", t->tid, ms);
    
    // Calculate wake time with better precision
    unsigned long current_time = get_system_ticks();
    unsigned long ticks = (ms + MS_PER_TICK -1 ) / MS_PER_TICK; // Convert ms to ticks, rounding up
    // Set wakeup time
    t->wakeup_time = current_time + ticks;
    
    // Add to sleep queue
    sr = splhigh();
    
    // Remove from sleep queue if already there
    struct thread **pp = &p->sleep_queue;
    while (*pp) {
        if (*pp == t) {
            *pp = t->next_sleeping;
            break;
        }
        pp = &(*pp)->next_sleeping;
    }
    
    // Add to sleep queue
    t->next_sleeping = p->sleep_queue;
    p->sleep_queue = t;
    TRACE_THREAD("SLEEP: Added thread %d to sleep queue (wake at %lu)", 
                t->tid, t->wakeup_time);
    spl(sr);
        
    // Set up a direct timeout for this thread
    sleep_timeout = addtimeout(p, ticks, thread_sleep_wakeup_handler);
    if (sleep_timeout) {
        sleep_timeout->arg = (long)t;
    } else {
        TRACE_THREAD("SLEEP: Failed to set up sleep timeout");
    }
    
    // CRITICAL: Save the current context
    sr = splhigh();
    if (save_context(&t->ctxt[CURRENT]) == 0) {
        // First time through - this is the "going to sleep" path
        TRACE_THREAD("SLEEP: Thread %d sleeping for %ld ms, will wake at tick %lu (current: %lu)",
                    t->tid, ms, t->wakeup_time, current_time);
        
        atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
        t->wait_type = WAIT_SLEEP;  // Set wait type
        remove_from_ready_queue(t);
        
        // Schedule another thread
        spl(sr);
        schedule();
        
        // We should never reach here - schedule() will switch to another thread
        TRACE_THREAD("SLEEP: ERROR - Returned from schedule() in sleep path!");
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
        t->wait_type = WAIT_NONE;
        t->wakeup_time = 0; // Clear wake-up time
        
        // Ensure we're in the RUNNING state
        if (t->state != THREAD_STATE_RUNNING) {
            TRACE_THREAD("SLEEP: Thread %d not in RUNNING state after wake, fixing", t->tid);
            atomic_thread_state_change(t, THREAD_STATE_RUNNING);
        }
        
        TRACE_THREAD("SLEEP: Thread %d woke up after sleeping", t->tid);
        spl(sr);
        return 0;
    }
}

static CONTEXT* get_thread_context(struct thread *t)
{
    if (!t || t->magic != CTXT_MAGIC || (t->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("GET_CTX ERROR: Invalid thread reference");
        return NULL;
    }

    // Pour thread0, utiliser le contexte process
    if (t->tid == 0) {
        if (!t->proc || t->proc->magic != CTXT_MAGIC) {
            TRACE_THREAD("GET_CTX ERROR: Invalid process magic for thread0");
            return NULL;
        }
        TRACE_THREAD("GET_CTX Using process context for thread0");
        return &t->proc->ctxt[CURRENT];
    }

    TRACE_THREAD("GET_CTX: Using current context for thread %d", t->tid);
    return &t->ctxt[CURRENT];
}

/**
 * Check for and wake up any sleeping threads that are ready
 * 
 * This function applies priority boosting to woken threads to prevent
 * starvation and ensure responsiveness.
 */
void check_and_wake_sleeping_threads(struct proc *p)
{
    if (!p || !p->sleep_queue){
        return;
    }
    unsigned short sr = splhigh();
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
    struct thread *sleep_t = p->sleep_queue;
    int woke_threads = 0;

    while (sleep_t) {
        struct thread *next_sleep = sleep_t->next_sleeping;

        TRACE_THREAD("SLEEP_CHECK: Checking thread %d (wake at %lu, current %lu)",
                    sleep_t->tid, sleep_t->wakeup_time, current_time);

        // If thread should wake up
        if (sleep_t->magic == CTXT_MAGIC && 
            (sleep_t->state & THREAD_STATE_BLOCKED) &&
            !(sleep_t->state & THREAD_STATE_EXITED) &&
            sleep_t->wakeup_time > 0 && 
            sleep_t->wakeup_time <= get_system_ticks()) {
            TRACE_THREAD("SLEEP_CHECK: Thread %d should wake up!", sleep_t->tid);
            
            // Remove from sleep queue
            struct thread **pp = &p->sleep_queue;
            while (*pp) {
                if (*pp == sleep_t) {
                    *pp = sleep_t->next_sleeping;
                    break;
                }
                pp = &(*pp)->next_sleeping;
            }

            // Boost priority
            sleep_t->original_priority = sleep_t->priority;
            sleep_t->priority = sleep_t->priority + 5; // Boost by 5 levels
            sleep_t->priority_boost = 1;
            sleep_t->wait_type = WAIT_NONE;
            sleep_t->wakeup_time = 0; // Clear wake-up time
            
            // Update thread state and add to ready queue
            atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
            
            // For SCHED_FIFO threads, add to end of ready queue as per POSIX
            add_to_ready_queue(sleep_t);
            
            woke_threads++;
        }
        
        sleep_t = next_sleep;
    }
    
    if (woke_threads > 0) {
        TRACE_THREAD("SLEEP_CHECK: Woke up %d threads from sleep queue", woke_threads);
        // Force a schedule if we woke up any threads and this is the current process
        if (p == curproc) {
            TRACE_THREAD("SLEEP_CHECK: Forcing schedule after waking threads");
            spl(sr);
            schedule();
            return;
        }        
    }
}

// Direct wakeup handler for sleeping threads
void thread_sleep_wakeup_handler(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    
    if (!t || !p || t->proc != p) {
        return;
    }
    
    unsigned short sr = splhigh();
    
    // Only wake up if still sleeping
    if ((t->state & THREAD_STATE_BLOCKED) && t->wait_type == WAIT_SLEEP) {
        TRACE_THREAD("SLEEP_WAKEUP: Direct wakeup for thread %d", t->tid);

        // Save original priority and boost
        t->original_priority = t->priority;
        t->priority = t->priority + 5;  // Boost by 5 levels instead of using a constant  // Use highest priority
        t->priority_boost = 1;
        
        TRACE_THREAD("SLEEP_WAKEUP: Boosted thread %d priority from %d to %d", 
                    t->tid, t->original_priority, t->priority);

        // Wake up thread
        t->wait_type = WAIT_NONE;
        t->wakeup_time = 0;  // Clear wake-up time
        remove_from_sleep_queue(p, t);
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        // Force a schedule to run this thread immediately if possible
        spl(sr);

        reschedule_preemption_timer(p, t->tid);
        schedule();
        // if (p == curproc) {  // Only schedule if this is the current process
        //     schedule();
        // }
        
        return;
    }
    
    spl(sr);
}

/**
 * Helper function to get the highest priority thread from the ready queue
 *
 * This function implements POSIX-compliant thread selection:
 * - Highest priority threads are selected first
 * - For equal priority SCHED_FIFO threads, the one that's been waiting longest is selected
 * - For equal priority SCHED_RR threads, round-robin order is used
 */
struct thread *get_highest_priority_thread(struct proc *p)
{
    if (!p || !p->ready_queue) {
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Ready queue is empty");
        return NULL;
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
inline unsigned long get_system_ticks(void)
{
    return *((volatile unsigned long *)_hz_200);
}

/**
 * Helper function to reschedule the preemption timer
 */
void reschedule_preemption_timer(PROC *p, long arg)
{
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
int should_schedule_thread(struct thread *current, struct thread *next)
{
    if (!current || !next) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Invalid thread");
        return 0;
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

/**
 * Remove a thread from the sleep queue if it's there
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

/* Remove thread from all wait queues */
void remove_thread_from_wait_queues(struct thread *t)
{
    if (!t) return;
    
    unsigned short sr = splhigh();
    
    // Remove from mutex wait queues
    if (t->wait_type == WAIT_MUTEX && t->wait_obj) {
        struct mutex *m = (struct mutex *)t->wait_obj;
        struct thread **pp = &m->wait_queue;
        while (*pp) {
            if (*pp == t) {
                TRACE_THREAD("Removing thread %d from mutex wait queue", t->tid);
                *pp = t->next_wait;
                break;
            }
            pp = &(*pp)->next_wait;
        }
    }
    // Remove from semaphore wait queues
    else if (t->wait_type == WAIT_SEMAPHORE && t->wait_obj) {
        struct semaphore *sem = (struct semaphore *)t->wait_obj;
        struct thread **pp = &sem->wait_queue;
        while (*pp) {
            if (*pp == t) {
                TRACE_THREAD("Removing thread %d from semaphore wait queue", t->tid);
                *pp = t->next_wait;
                break;
            }
            pp = &(*pp)->next_wait;
        }
    }
    
    // Clear wait state
    t->next_wait = NULL;
    t->wait_type = WAIT_NONE;
    t->wait_obj = NULL;
    
    spl(sr);
}

/**
 * Make a process eligible for immediate selection as curproc
 * This increases the chance that curproc will become equal to p
 */
void make_process_eligible(struct proc *p) {
    if (!p) return;
    
    unsigned short sr = splhigh();
    
    // Set slices to 0 to ensure it's eligible to run immediately
    p->slices = 0;
    
    // If not in READY_Q or CURPROC_Q, add to READY_Q
    if (p->wait_q != READY_Q && p->wait_q != CURPROC_Q) {
        // Remove from current queue if any
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

void cleanup_process_threads(struct proc *pcurproc) {
	/* Clean up threads if any exist */
	if (pcurproc->threads) {
		TRACE(("terminate: cleaning up threads for pid=%d", pcurproc->pid));
		
		/* Cancel any thread-related timeouts */
		if (pcurproc->p_thread_timer.enabled) {
			TRACE(("terminate: stopping thread timer"));
			thread_timer_stop(pcurproc);
		}
		
		/* Cancel any thread-specific timeouts */
		TIMEOUT *timelist, *next_timelist;
		for (timelist = tlist; timelist; timelist = next_timelist) {
			next_timelist = timelist->next;
			if (timelist->proc == pcurproc) {
				TRACE(("terminate: cancelling timeout for pid=%d", pcurproc->pid));
				canceltimeout(timelist);
			}
		}
		
		/* Free all threads */
		struct thread *t = pcurproc->threads;
		struct thread *next;
		
		while (t) {
			next = t->next;
			
			/* Only process valid threads */
			if (t->magic == CTXT_MAGIC) {
				TRACE(("terminate: cleaning up thread %d", t->tid));
				
				/* Remove from any wait queues */
				remove_thread_from_wait_queues(t);
				remove_from_ready_queue(t);
				
				/* Free thread-specific resources */
				if (t->t_sigctx) {
					kfree(t->t_sigctx);
				}
				
				/* Mark as exited */
				t->state |= THREAD_STATE_EXITED;
				t->magic = 0;  /* Invalidate magic to prevent further use */
				
				/* Free thread stack (except for thread0) */
				if (t->tid != 0 && t->stack) {
					kfree(t->stack);
				}
				kfree(t);
			}
			t = next;
		}
		pcurproc->threads = NULL;
		pcurproc->current_thread = NULL;
		pcurproc->num_threads = 0;
		pcurproc->total_threads = 0;
	}

	/* Clean up any thread synchronization objects */
	cleanup_thread_sync_states(pcurproc);
}

/**
 * Detach a thread - mark it as not joinable
 * 
 * @param tid Thread ID to detach
 * @return 0 on success, error code on failure
 */
long _cdecl sys_p_detachthread(long tid)
{
    struct proc *p = curproc;
    struct thread *target = NULL;
    unsigned short sr;
    
    if (!p)
        return -EINVAL;
    
    // Find target thread
    for (target = p->threads; target; target = target->next) {
        if (target->tid == tid)
            break;
    }
    
    if (!target)
        return -ESRCH;  // No such thread
    
    sr = splhigh();
    
    // Check if thread is already joined
    if (target->joined) {
        spl(sr);
        return -EINVAL;  // Thread already joined
    }
    
    // Check if thread is already detached
    if (target->detached) {
        spl(sr);
        return 0;  // Already detached, not an error
    }
    
    // Mark thread as detached
    target->detached = 1;
    
    // If thread already exited, free its resources
    if (target->state & THREAD_STATE_EXITED) {
        TRACE_THREAD("DETACH: Thread %d already exited, freeing resources", target->tid);
        
        // Free resources
        if (target->t_sigctx) {
            kfree(target->t_sigctx);
            target->t_sigctx = NULL;
        }
        
        if (target->stack && target->tid != 0) {
            kfree(target->stack);
            target->stack = NULL;
        }
        
        // Clear magic and free
        target->magic = 0;
        kfree(target);
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
long _cdecl sys_p_jointhread(long tid, void **retval)
{
    struct proc *p = curproc;
    struct thread *current, *target = NULL;
    unsigned short sr;
    
    if (!p || !p->current_thread)
        return -EINVAL;
    
    current = p->current_thread;
    
    // Cannot join self - would deadlock
    if (current->tid == tid)
        return -EDEADLK;
    
    // Find target thread
    for (target = p->threads; target; target = target->next) {
        if (target->tid == tid)
            break;
    }
    
    if (!target)
        return -ESRCH;  // No such thread
    
    // Check if thread is joinable
    if (target->detached)
        return -EINVAL;  // Thread is detached, cannot join
    
    // Check if thread is already joined
    if (target->joined)
        return -EINVAL;  // Thread already joined
    
    sr = splhigh();
    
    // Check if thread already exited
    if (target->state & THREAD_STATE_EXITED) {
        // Thread already exited, get return value and return immediately
        if (retval)
            *retval = target->retval;
        
        // Mark as joined so resources can be freed
        target->joined = 1;
        
        // Free resources now
        if (target->t_sigctx) {
            kfree(target->t_sigctx);
            target->t_sigctx = NULL;
        }
        
        if (target->stack && target->tid != 0) {
            kfree(target->stack);
            target->stack = NULL;
        }
        
        // Clear magic and free
        target->magic = 0;
        kfree(target);
        
        spl(sr);
        return 0;
    }
    
    // Mark target as being joined by current thread
    target->joiner = current;
    target->joined = 0;  // Will be set to 1 when join completes

    // Mark current thread as waiting for join
    current->wait_type = WAIT_JOIN;
    current->wait_obj = target;
    
    TRACE_THREAD("JOIN: Thread %d waiting for thread %d to exit",
                current->tid, target->tid);
    
    // Block the current thread
    atomic_thread_state_change(current, THREAD_STATE_BLOCKED);
    
    // CRITICAL: Save the current context before scheduling
    // This ensures we can resume from this point when the target thread exits
    if (save_context(&current->ctxt[CURRENT]) == 0) {
        // First time through - going to sleep
        spl(sr);
        
        // Schedule another thread
        schedule();
        
        // Should never reach here
        TRACE_THREAD("JOIN: ERROR - Returned from schedule() in join path!");
        return -1;
    }
    
    // Second time through - waking up after target thread exited
    sr = splhigh();
    
    // Ensure we're in the RUNNING state
    if (current->state != THREAD_STATE_RUNNING) {
        TRACE_THREAD("JOIN: Thread %d not in RUNNING state after wake, fixing", current->tid);
        atomic_thread_state_change(current, THREAD_STATE_RUNNING);
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

/* End of Threads stuff */