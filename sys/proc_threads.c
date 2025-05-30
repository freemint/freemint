#include "proc_threads.h"

/* Threads stuff */

void thread_switch(struct thread *from, struct thread *to);
long create_thread(struct proc *p, void (*func)(void*), void *arg);
void init_thread_context(struct thread *t, void (*func)(void*), void *arg);
void thread_start(void);

void thread_exit(void);
void init_thread0(struct proc *p);

void thread_preempt_handler(PROC *p, long arg);
void thread_timer_start(struct proc *p, int thread_id);
void thread_timer_stop(PROC *p);
void start_thread_timing(struct thread *t);

void thread_switch_timeout_handler(PROC *p, long arg);
void reset_thread_switch_state(void);
int check_thread_switch_timeout(void);

int should_wake_thread(struct thread *t);
void check_and_wake_sleeping_threads(struct proc *p);
void thread_sleep_wakeup_handler(PROC *p, long arg);

static CONTEXT* get_thread_context(struct thread *t);
static void set_thread_usp(CONTEXT *ctx);

void reschedule_preemption_timer(PROC *p, long arg);
struct thread *get_highest_priority_thread(struct proc *p);
int should_schedule_thread(struct thread *current, struct thread *next);

/* Mutex for timer operations */
static int timer_mutex_locked = 0;

static int thread_switch_in_progress = 0;

static TIMEOUT *thread_switch_timeout = NULL;
static struct thread *switching_from = NULL;
static struct thread *switching_to = NULL;
static unsigned long switch_ticks = 0;

/*
 * Reset the thread switch state
 * Called when a thread switch completes or times out
 */
void reset_thread_switch_state(void)
{
    unsigned short sr = splhigh();
    
    thread_switch_in_progress = 0;
    switching_from = NULL;
    switching_to = NULL;
    switch_ticks = 0;
    
    if (thread_switch_timeout) {
        canceltimeout(thread_switch_timeout);
        thread_switch_timeout = NULL;
    }
    
    spl(sr);
}

/*
 * Check if a thread switch has timed out
 * Returns 1 if timed out, 0 otherwise
 */
int check_thread_switch_timeout(void)
{
    if (!thread_switch_in_progress || !switch_ticks)
        return 0;
        
    unsigned long current_ticks = *((volatile unsigned long *)_hz_200);
    unsigned long elapsed_ticks = current_ticks - switch_ticks;
    
    return (elapsed_ticks > (THREAD_SWITCH_TIMEOUT_MS / 5)); /* 200Hz = 5ms per tick */
}

/*
 * Thread switch timeout handler
 * Called when a thread switch takes too long
 */
void thread_switch_timeout_handler(PROC *p, long arg)
{
    static int recovery_attempts = 0;
    
    TRACE_THREAD("TIMEOUT: Thread switch from %d to %d timed out after %dms",
                 switching_from ? switching_from->tid : -1,
                 switching_to ? switching_to->tid : -1,
                 THREAD_SWITCH_TIMEOUT_MS);
    
    // Check if the threads are still valid
    TRACE_THREAD("TIMEOUT: From thread %s, To thread %s",
                (switching_from && switching_from->magic == CTXT_MAGIC) ? "valid" : "invalid",
                (switching_to && switching_to->magic == CTXT_MAGIC) ? "valid" : "invalid");

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
 * Initialize the thread timer
 * Called during system initialization
 */
void thread_timer_init(PROC *p)
{
    p->p_thread_timer.thread_id = 0;
    p->p_thread_timer.enabled = 0;
    p->p_thread_timer.timeout = NULL;
    p->p_thread_timer.in_handler = 0;
}

/*
 * Start timing a specific process/thread
 */
void thread_timer_start(struct proc *p, int thread_id)
{
    unsigned short sr, retry_count = 0;
    TIMEOUT *new_timeout = NULL;
    
    TRACE_THREAD("thread_timer_start called for process %d", p->pid);
    if (!p)
        return;

    /* Try to acquire the timer mutex with a timeout */
    while (1) {
        sr = splhigh();
        if (!timer_mutex_locked) {
            TRACE_THREAD("TIMER: Acquired timer mutex");
            timer_mutex_locked = 1;
            spl(sr);
            break;
        }
        spl(sr);
        
        /* If we've tried too many times, give up */
        if (++retry_count > 10) {
            TRACE_THREAD("TIMER WARNING: Failed to acquire timer mutex after 10 retries");
            return;
        }
    }

    /* CRITICAL SECTION - We now have the mutex */
    TRACE_THREAD("TIMER: Starting thread timer for process %d", p->pid);
    
    sr = splhigh();
    
    /* If timer is already enabled, don't add another timeout */
    if (p->p_thread_timer.enabled && p->p_thread_timer.timeout) {
        TRACE_THREAD("TIMER: Timer already enabled, not adding another timeout");
        spl(sr);
        goto cleanup;
    }
    
    /* Create the timeout before modifying any state */
    new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
    if (!new_timeout) {
        TRACE_THREAD("TIMER ERROR: Failed to create timeout");
        spl(sr);
        goto cleanup;
    }
    new_timeout->arg = (long)p->current_thread;

    /* Now that we have a valid timeout, update the timer state */
    p->p_thread_timer.timeout = new_timeout;
    p->p_thread_timer.thread_id = p->current_thread->tid;

    /* Set enabled flag last to ensure everything is set up */
    p->p_thread_timer.enabled = 1;
    p->p_thread_timer.in_handler = 0;
    
    TRACE_THREAD("TIMER: Thread timer started for process %d", p->pid);
    spl(sr);
    
cleanup:
    /* Always release the mutex */
    sr = splhigh();
    timer_mutex_locked = 0;
    spl(sr);
    
    TRACE_THREAD("TIMER: Timer mutex released");
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

    /* Try to acquire the timer mutex with a timeout */
    while (1) {
        sr = splhigh();
        if (!timer_mutex_locked) {
            timer_mutex_locked = 1;
            spl(sr);
            break;
        }
        spl(sr);
        
        /* If we've tried too many times, give up */
        if (++retry_count > 10) {
            TRACE_THREAD("WARNING: Failed to acquire timer mutex after 10 retries");
            return;
        }

    }

    /* CRITICAL SECTION - We now have the mutex */
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
    
    /* Always release the mutex */
    sr = splhigh();
    timer_mutex_locked = 0;
    spl(sr);
    
    TRACE_THREAD("TIMER: Timer mutex released");
}

/*
 * Function to start timing a newly created thread
 * Call this after thread creation
 */
void start_thread_timing(struct thread *t)
{
    TRACE_THREAD("(start_thread_timing) start_thread_timing called by thread %d creation", t->tid);
    if (!t || !t->proc) return;
    
    TRACE_THREAD("(start_thread_timing) Starting thread timer for thread %d in process %d", t->tid, t->proc->pid);
    thread_timer_start(t->proc, t->tid);
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

void thread_switch(struct thread *from, struct thread *to) {
    unsigned short sr;
    
    // Validate input parameters
    if (!from || !to || from == to) {
        TRACE_THREAD("SWITCH: Invalid thread pointers - from=%p, to=%p", from, to);
        return;
    }
    
    // Check magic numbers to identify invalid threads
    if (from->magic != CTXT_MAGIC) {
        TRACE_THREAD("SWITCH: Invalid 'from' thread magic: %p (magic=%lx)", from, from->magic);
        return;
    }
    
    if (to->magic != CTXT_MAGIC) {
        TRACE_THREAD("SWITCH: Invalid 'to' thread magic: %p (magic=%lx)", to, to->magic);
        return;
    }
    
    // Check for exited threads
    if ((from->state & THREAD_STATE_EXITED) || (to->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("SWITCH: Cannot switch involving exited thread: from=%d (state=%d), to=%d (state=%d)",
                    from->tid, from->state, to->tid, to->state);
        return;
    }
    
    // Special handling for thread0
    if (from->tid == 0 && (from->state & THREAD_STATE_EXITED) && from->proc->num_threads > 1) {
        TRACE_THREAD("SWITCH: Preventing thread0 exit while other threads are running");
        atomic_thread_state_change(from, THREAD_STATE_READY);
        return;
    }
    
    // Disable interrupts during context switch
    sr = splhigh();
    
    // Check if another thread switch is in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("SWITCH: Another switch in progress, aborting");
        spl(sr);
        return;
    }
    
    // Set switch in progress flag
    thread_switch_in_progress = 1;
    
    // Store thread pointers and setup timeout for deadlock detection
    switching_from = from;
    switching_to = to;
    switch_ticks = *((volatile unsigned long *)_hz_200);
    
    if (thread_switch_timeout) {
        canceltimeout(thread_switch_timeout);
    }
    thread_switch_timeout = addtimeout(from->proc, THREAD_SWITCH_TIMEOUT_MS / 20, 
                                      thread_switch_timeout_handler);
    
    TRACE_THREAD("SWITCH: Switching threads: %d -> %d", from->tid, to->tid);
    
    // Verify stack integrity
    if (from->stack_magic != STACK_MAGIC || to->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("SWITCH ERROR: Stack corruption detected! from_magic=%lx, to_magic=%lx",
                    from->stack_magic, to->stack_magic);
        thread_switch_in_progress = 0;
        spl(sr);
        return;
    }
    
    // Check if source thread is in sleep queue
    int in_sleep_queue = 0;
    struct thread *sleep_t = from->proc->sleep_queue;
    
    while (sleep_t) {
        if (sleep_t == from) {
            in_sleep_queue = 1;
            break;
        }
        sleep_t = sleep_t->next_sleeping;
    }
    
    // Save current USP
    unsigned long current_usp;
    asm volatile (
        "move.l %%a0,-(%%sp)\n\t"
        "move.l %%usp,%%a0\n\t"
        "move.l %%a0,%0\n\t"
        "move.l (%%sp)+,%%a0"
        : "=m" (current_usp)
        :
        : "memory"
    );
    
    // Align USP and save it
    from->ctxt[CURRENT].usp = current_usp & ~3L;
    
    // Handle context switch based on thread state
    if (in_sleep_queue) {
        // For sleeping threads, just update states and switch directly
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        
        // Get destination context
        CONTEXT *to_ctx = get_thread_context(to);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        
        // Reset switch state before context change
        reset_thread_switch_state();
        spl(sr);
        
        // Set USP and switch context
        set_thread_usp(to_ctx);
        change_context(to_ctx);
        
        // Should never reach here
        TRACE_THREAD("SWITCH ERROR: Returned from change_context!");
    } 
    else if (save_context(&from->ctxt[CURRENT]) == 0) {
        // Normal context save path
        
        // Update thread states
        atomic_thread_state_change(from, THREAD_STATE_READY);
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        
        // Get destination context
        CONTEXT *to_ctx = get_thread_context(to);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        
        // Reset switch state before context change
        reset_thread_switch_state();
        spl(sr);
        
        // Set USP and switch context
        set_thread_usp(to_ctx);
        change_context(to_ctx);
        
        // Should never reach here
        TRACE_THREAD("SWITCH ERROR: Returned from change_context!");
    }
    
    // This is the return path after being switched back to
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
    TIMEOUT *new_timeout = NULL;
    struct thread *thread_arg = (struct thread *)arg;
    
    // Basic validation
    if (!p) {
        TRACE_THREAD("PREEMPT: Invalid process pointer");
        return;
    }

    // If not current process, reschedule the timeout
    if (p != curproc) {
        // TRACE_THREAD("PREEMPT: Not current process, rescheduling timeout, wanted pid=%d and current pid=%d", p->pid, curproc->pid);
        new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
        if (new_timeout) {
            new_timeout->arg = arg;
            p->p_thread_timer.timeout = new_timeout;
        }
        return;
    }

    // check_and_wake_sleeping_threads(p);

    // Validate thread argument if provided
    if (thread_arg && (thread_arg->magic != CTXT_MAGIC || thread_arg->proc != p)) {
        TRACE_THREAD("PREEMPT: Invalid thread argument %lx, using current thread", arg);
        thread_arg = p->current_thread;
    }
    
    // Protection against reentrance
    if (p->p_thread_timer.in_handler) {
        // Don't add a new timeout if timer is disabled
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
    
    // Only preempt if multiple threads exist
    if (p->num_threads > 1 && p->current_thread) {
        struct thread *curr_thread = p->current_thread;
        struct thread *next = NULL;
        
        // Update timeslice accounting for current thread
        if (curr_thread->policy != SCHED_FIFO) {  // FIFO threads don't use timeslicing
            TRACE_THREAD("PREEMPT: Updating timeslice for thread %d", curr_thread->tid);
            unsigned long current_ticks = *((volatile unsigned long *)_hz_200);
            unsigned long elapsed = current_ticks - curr_thread->last_scheduled;
            
            // Check if timeslice has expired
            if (curr_thread->remaining_timeslice <= elapsed) {
                // Timeslice expired, reset it and mark for preemption
                curr_thread->remaining_timeslice = curr_thread->timeslice;
                
                // For SCHED_RR and SCHED_OTHER, move to end of ready queue
                if (curr_thread->policy == SCHED_RR || curr_thread->policy == SCHED_OTHER) {
                    TRACE_THREAD("PREEMPT: Thread %d timeslice expired, moving to end of ready queue",
                                curr_thread->tid);
                    
                    // Add to ready queue (will be at end)
                    atomic_thread_state_change(curr_thread, THREAD_STATE_READY);
                    add_to_ready_queue(curr_thread);
                }
            } else {
                // Update remaining timeslice
                curr_thread->remaining_timeslice -= elapsed;
            }
        }

        TRACE_THREAD("PREEMPT: Checking for READY threads");

        // Get highest priority thread from ready queue
        next = get_highest_priority_thread(p);
        
        // If no thread in ready queue, check sleep queue
        if (!next && p->sleep_queue) {
            TRACE_THREAD("PREEMPT: No ready threads, checking sleep queue");
            check_and_wake_sleeping_threads(p);
            next = get_highest_priority_thread(p);
        }

        // If still no thread, use current thread
        if (!next) {
            TRACE_THREAD("PREEMPT: No other threads to run, continuing with current thread %d",
                        curr_thread->tid);
            
            // Reschedule timer and return
            reschedule_preemption_timer(p, (long)curr_thread);
            spl(sr);
            return;
        }
        
        // Check if we should preempt current thread
        if (should_schedule_thread(curr_thread, next)) {
            // Don't preempt if thread switch already in progress
            if (thread_switch_in_progress) {
                TRACE_THREAD("PREEMPT: Thread switch already in progress, not preempting");
                reschedule_preemption_timer(p, (long)curr_thread);
                spl(sr);
                return;
            }
            
            // Remove next thread from ready queue
            remove_from_ready_queue(next);
            
            // Update thread states
            atomic_thread_state_change(next, THREAD_STATE_RUNNING);
            p->current_thread = next;
            
            // Record scheduling time for timeslice accounting
            next->last_scheduled = *((volatile unsigned long *)_hz_200);
            
            // Rearm timer before switch
            reschedule_preemption_timer(p, (long)next);
            TRACE_THREAD("PREEMPT: Switching from thread %d to thread %d", curr_thread->tid, next->tid);
            thread_switch(curr_thread, next);
            
            TRACE_THREAD("PREEMPT: Returned from thread_switch, now running thread %d", next->tid);
        }
    }
    
    // Reschedule timer
    reschedule_preemption_timer(p, (long)p->current_thread);    
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
    memset(t, 0, sizeof(struct thread));

    t->tid = p->num_threads++;
    // Link into process
    // Use a more robust approach to update the thread list
    struct thread **tp = &p->threads;
    // Check for duplicate thread IDs (shouldn't happen, but better safe than sorry)
    while (*tp) {
        if ((*tp)->tid == t->tid) {
            TRACE_THREAD("ERROR: Duplicate thread ID %d detected!", t->tid);
            t->tid = p->num_threads++;  // Generate a new ID
        }
        tp = &(*tp)->next;
    }
    *tp = t;  // Add to end of list
    t->next = NULL;

    t->proc = p;

    t->priority = p->pri;
    t->original_priority = p->pri;
    t->policy = DEFAULT_SCHED_POLICY;
    t->timeslice = THREAD_DEFAULT_TIMESLICE;
    t->remaining_timeslice = THREAD_DEFAULT_TIMESLICE;
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

    TRACE_THREAD("Thread %d stack: base=%p, top=%p", t->tid, t->stack, t->stack_top);
    
    // Initialize context
    init_thread_context(t, func, arg);
    
    atomic_thread_state_change(t, THREAD_STATE_READY);
    add_to_ready_queue(t);

    if(!p->p_thread_timer.enabled){
        start_thread_timing(t);
    }

    TRACE_THREAD("Thread %d created successfully", t->tid);
    spl(sr);
    return t->tid;
}

void init_thread_context(struct thread *t, void (*func)(void*), void *arg) {
    TRACE_THREAD("Initializing context for thread %d", t->tid);
    
    // Clear context
    memset(&t->ctxt[CURRENT], 0, sizeof(struct context));
    memset(&t->ctxt[SYSCALL], 0, sizeof(struct context));
    
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
    
    TRACE_THREAD("Exception frame created at %p:", frame_ptr);
    TRACE_THREAD("  SR = %04x (supervisor mode)", frame_ptr[1]);
    TRACE_THREAD("  PC = %04x%04x", frame_ptr[2], frame_ptr[3]);
    TRACE_THREAD("  Thread pointer stored in D0 = %p", t);
    
    // Set magic values required by FreeMiNT
    t->magic = CTXT_MAGIC;
    
    // Copy to SYSCALL context
    memcpy(&t->ctxt[SYSCALL], &t->ctxt[CURRENT], sizeof(struct context));
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
    
    // Exit the thread when done
    TRACE_THREAD("START: Thread exiting");
    thread_exit();
    
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
    
    sr = splhigh();
    
    struct thread *current = p->current_thread;
    
    // Check and wake any sleeping threads
    check_and_wake_sleeping_threads(p);
    
    // Get highest priority thread from ready queue
    struct thread *next = get_highest_priority_thread(p);
    
    TRACE_THREAD("SCHED: Current thread %d, highest priority ready thread %d", 
                current ? current->tid : -1, 
                next ? next->tid : -1);
    
    // If still no valid thread and we have a current thread, just continue
    if (!next && current) {
        TRACE_THREAD("SCHED: No ready threads or thread0, continuing with current thread %d", current->tid);
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
        TRACE_THREAD("SCHED: No current thread, switching to thread %d", next->tid);
        
        // If next is in ready queue, remove it
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }
        
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        // Record scheduling time for timeslice accounting
        next->last_scheduled = *((volatile unsigned long *)_hz_200);
        
        CONTEXT *next_ctx = get_thread_context(next);
        if (!next_ctx) {
            TRACE_THREAD("SCHED: Failed to get context for thread %d", next->tid);
            spl(sr);
            return;
        }
        
        set_thread_usp(next_ctx);
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
            // Check if current thread is in sleep queue
            if (is_in_sleep_queue(current)) {
                TRACE_THREAD("THREAD_SCHED: Current thread %d is in sleep queue", current->tid);
                atomic_thread_state_change(current, THREAD_STATE_SLEEPING);
            } else {
                TRACE_THREAD("THREAD_SCHED: Adding current thread %d to ready queue", current->tid);
                atomic_thread_state_change(current, THREAD_STATE_READY);
                add_to_ready_queue(current);
            }
        }
        
        // Update next thread state and make it current
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        // Record scheduling time for timeslice accounting
        next->last_scheduled = *((volatile unsigned long *)_hz_200);
        
        // Perform the context switch
        thread_switch(current, next);
    } else {
        TRACE_THREAD("SCHED: Not switching, current thread %d has higher priority or timeslice remaining",
                    current->tid);
    }
    
    spl(sr);
}

// Thread exit
void thread_exit(void) {

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
        thread_exit();
        return;
    }
    
    int tid = current->tid;
    thread_exit_in_progress = 1;
    exit_owner_tid = tid;
    
    TRACE_THREAD("EXIT: Thread %d beginning exit process", tid);
    
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
    
    // Mark as EXITED and remove from queues
    atomic_thread_state_change(current, THREAD_STATE_EXITED);
    remove_from_ready_queue(current);
    remove_from_sleep_queue(current);
    
    // Remove from thread list
    struct thread **tp;
    for (tp = &p->threads; *tp; tp = &(*tp)->next) {
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
        unsigned long current_time = *((volatile unsigned long *)_hz_200);
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
                remove_from_sleep_queue(sleep_t);
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
    
    // Free resources
    if (old_thread && old_thread->magic == CTXT_MAGIC) {
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
        
        TRACE_THREAD("EXIT: Before freeing thread %d, current_thread=%p, switching_from=%p, switching_to=%p",
                    tid, p->current_thread, switching_from, switching_to);
        
        kfree(old_thread);
        
        TRACE_THREAD("EXIT: After freeing thread %d", tid);
        old_thread = NULL;
        TRACE_THREAD("Thread %d exited", tid);
    } else {
        TRACE_THREAD("EXIT ERROR: Attempt to exit thread id %d, but No current thread to free", tid);
    }
    
    thread_exit_in_progress = 0;
    exit_owner_tid = -1;
    
    // Switch to target context
    TRACE_THREAD("EXIT: Switching to target context, PC=%lx", target_ctx->pc);
    
    // KEY CHANGE: Use a proper context switch mechanism
    // Create a temporary context to save the current state
    CONTEXT temp_ctx;
    
    // Save the current context
    if (save_context(&temp_ctx) == 0) {
        // This is the initial save_context call
        
        // If we're switching to a thread, set its USP
        if (p->current_thread) {
            set_thread_usp(target_ctx);
        }
        
        spl(sr);
        
        // Switch to the target context
        change_context(target_ctx);
        
        // Should never reach here
        TRACE_THREAD("ERROR: Returned from change_context after thread exit!");
    } else {
        // This is the return from save_context
        // We should never get here because the exiting thread's context is gone
        TRACE_THREAD("CRITICAL ERROR: Returned to exited thread context!");
        
        // Try to recover by scheduling another thread
        spl(sr);
        schedule();
    }
}

void init_thread0(struct proc *p) {
    if (p->current_thread) return; // Already initialized
    
    struct thread *t0 = kmalloc(sizeof(struct thread));

    if (!t0) return;
    TRACE_THREAD("KMALLOC: Allocated thread0 structure at %p for process %d", t0, p->pid);
    TRACE_THREAD("Initializing thread0 for process %d", p->pid);
    
    memset(t0, 0, sizeof(struct thread));
    t0->tid = 0;
    t0->proc = p;
    t0->priority = p->pri;

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
    atomic_thread_state_change(t0, THREAD_STATE_RUNNING);
    TRACE_THREAD("Thread0 initialized for process %d", p->pid);
    TRACE_THREAD("(init_thread0) Starting thread timer for process %d", p->pid);
    start_thread_timing(t0);
}

// Thread creation syscall
long _cdecl sys_p_createthread(void (*func)(void*), void *arg, void *stack)
{
    long tid;
    
    TRACE_THREAD("sys_p_createthread: func=%p arg=%p stack=%p", func, arg, stack);
    
    // Ensure thread0 is initialized
    init_thread0(curproc);
    
    // Create the thread
    tid = create_thread(curproc, func, arg);

    return tid;
}

// Thread exit syscall
long _cdecl sys_p_exitthread(void) {
    TRACE_THREAD("sys_p_exitthread called");
    thread_exit();
    return 0;
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
    
    TRACE_THREAD("SLEEP: Thread %d sleeping for %ld ms", t->tid, ms);
    
    // Calculate wake time with better precision
    unsigned long current_time = *((volatile unsigned long *)_hz_200);
    unsigned long ticks = (ms + 4) / 5; // Convert ms to ticks, rounding up
    
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
        
        // Set up the thread for sleeping
        t->wakeup_time = current_time + ticks;
        TRACE_THREAD("SLEEP: Thread %d sleeping for %ld ms, will wake at tick %lu (current: %lu)",
                    t->tid, ms, t->wakeup_time, current_time);
        
        atomic_thread_state_change(t, THREAD_STATE_SLEEPING);
        remove_from_ready_queue(t);
        add_to_sleep_queue(t);
        
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
        current_time = *((volatile unsigned long *)_hz_200);
        if (t->wakeup_time > 0 && current_time > t->wakeup_time) {
            TRACE_THREAD("SLEEP: Thread %d woke up late by %lu ms",
                        t->tid, (current_time - t->wakeup_time) * 5);
        }
        
        t->wakeup_time = 0; // Clear wake-up time
        
        // Ensure we're not in the sleep queue anymore
        if (is_in_sleep_queue(t)) {
            TRACE_THREAD("SLEEP: Thread %d still in sleep queue after wake, removing", t->tid);
            remove_from_sleep_queue(t);
        }
        
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

int should_wake_thread(struct thread *t)
{
    if (!t || t->magic != CTXT_MAGIC || (t->state & THREAD_STATE_EXITED))
        return 0;
        
    if (!(t->state & THREAD_STATE_SLEEPING))
        return 0;
        
    unsigned long current_time = *((volatile unsigned long *)_hz_200);
    return (t->wakeup_time > 0 && t->wakeup_time <= current_time);
}

void add_to_sleep_queue(struct thread *t)
{
    struct proc *p;
    unsigned short sr;

    if (!t || !t->proc) return;
    p = t->proc;

    sr = splhigh();

    // Validate wake-up time
    if (t->wakeup_time == 0) {
        TRACE_THREAD("SLEEP_Q: ERROR: Invalid wakeup time for thread %d", t->tid);
        spl(sr);
        return;
    }

    // Check if already in sleep queue
    if (is_in_sleep_queue(t)) {
        TRACE_THREAD("SLEEP_Q: Thread %d already in sleep queue", t->tid);
        spl(sr);
        return;
    }

    // Add to sleep queue, sorted by wake-up time
    struct thread **pp = &p->sleep_queue;
    while (*pp && (*pp)->wakeup_time <= t->wakeup_time) {
        pp = &(*pp)->next_sleeping;
    }
    
    t->next_sleeping = *pp;
    *pp = t;

    TRACE_THREAD("SLEEP_Q: Added thread %d to sleep queue (wake at %lu)", 
                t->tid, t->wakeup_time);
    
    spl(sr);
}

void remove_from_sleep_queue(struct thread *t)
{
    struct proc *p;
    unsigned short sr;

    if (!t || !t->proc) return;
    p = t->proc;

    sr = splhigh();

    struct thread **pp = &p->sleep_queue;
    int found = 0;
    
    while (*pp) {
        if (*pp == t) {
            *pp = t->next_sleeping;
            t->next_sleeping = NULL;
            found = 1;
            TRACE_THREAD("SLEEP_Q: Removed thread %d from sleep queue", t->tid);
            break;
        }
        pp = &(*pp)->next_sleeping;
    }

    if (!found) {
        TRACE_THREAD("SLEEP_Q: Thread %d not found in sleep queue", t->tid);
    }

    spl(sr);
}

static CONTEXT* get_thread_context(struct thread *t)
{
    if (!t) {
        TRACE_THREAD("get_thread_context: NULL thread pointer");
        return NULL;
    }
    TRACE_THREAD("GET_CTX: Getting context for thread %d", t->tid);
    
    // Check if thread is valid
    if (t->magic != CTXT_MAGIC) {
        TRACE_THREAD("GET_CTX ERROR: Invalid thread magic %lx for thread %d", t->magic, t->tid);
        return NULL;
    }
    
    // Check if thread is exited
    if (t->state & THREAD_STATE_EXITED) {
        TRACE_THREAD("GET_CTX ERROR: Attempt to get context of EXITED thread %d", t->tid);
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

static void set_thread_usp(CONTEXT *ctx) {
    if (!ctx) return;
    
    // Ensure USP is properly aligned
    unsigned long aligned_usp = ctx->usp & ~3L;
    
    TRACE_THREAD("SET THREAD USP: Set USP=%lx", aligned_usp);
    
    asm volatile (
        "move.l %0,%%a0\n\t"
        "move.l %%a0,%%usp"
        : 
        : "r" (aligned_usp)
        : "a0"
    );
}

int is_in_sleep_queue(struct thread *t)
{
    if (!t || !t->proc)
        return 0;

    struct thread *curr = t->proc->sleep_queue;
    while (curr) {
        if (curr == t)
            return 1;
        curr = curr->next_sleeping;
    }
    return 0;
}

// Function to set thread scheduling policy
long _cdecl sys_p_setthreadpolicy(enum sched_policy policy, short priority, short timeslice)
{
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("sys_p_setthreadpolicy: invalid current process/thread");
        return -EINVAL;
    }

    struct thread *current = p->current_thread;
    unsigned short sr = splhigh();
    
    // Set policy
    if (policy >= SCHED_FIFO && policy <= SCHED_OTHER) {
        current->policy = policy;
    }
    
    // Set priority if valid
    if (priority >= MIN_NICE && priority <= MAX_NICE) {
        current->priority = priority;
    }
    
    // Set timeslice if valid
    if (timeslice > 0) {
        current->timeslice = timeslice;
        current->total_timeslice = timeslice;
    }
    
    spl(sr);
    return 0;
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
        // TRACE_THREAD("SLEEP_CHECK: No sleep queue to check or invalid process");
        return;
    }
    
    #ifdef DEBUG_THREADS    
    unsigned long current_time = *((volatile unsigned long *)_hz_200);
    #endif
    struct thread *sleep_t = p->sleep_queue;
    int woke_threads = 0;
    #ifdef DEBUG_THREADS
    TRACE_THREAD("SLEEP_CHECK: Checking sleep queue at time %lu", current_time);
    #endif
    while (sleep_t) {
        struct thread *next_sleep = sleep_t->next_sleeping;
        
        // If thread should wake up
        if (should_wake_thread(sleep_t)) {
            #ifdef DEBUG_THREADS
            TRACE_THREAD("SLEEP_CHECK: Thread %d should wake up (wake at %lu, current %lu, delay %lu ms)",
                        sleep_t->tid, sleep_t->wakeup_time, current_time,
                        (current_time - sleep_t->wakeup_time) * 5);
            #endif
            // Wake up thread with priority boost
            remove_from_sleep_queue(sleep_t);
            
            // Boost priority
            sleep_t->original_priority = sleep_t->priority;
            sleep_t->priority = sleep_t->priority + 5; // Boost by 5 levels
            sleep_t->priority_boost = 1;
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
    if ((t->state & THREAD_STATE_SLEEPING) && t->wakeup_time > 0) {
        TRACE_THREAD("SLEEP_WAKEUP: Direct wakeup for thread %d", t->tid);

        // Save original priority and boost
        t->original_priority = t->priority;
        t->priority = t->priority + 5;  // Boost by 5 levels instead of using a constant  // Use highest priority
        t->priority_boost = 1;
        
        TRACE_THREAD("SLEEP_WAKEUP: Boosted thread %d priority from %d to %d", 
                    t->tid, t->original_priority, t->priority);

        // Wake up thread
        remove_from_sleep_queue(t);
        t->wakeup_time = 0;  // Clear wake-up time
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        // Force a schedule to run this thread immediately if possible
        spl(sr);

        if (p == curproc) {  // Only schedule if this is the current process
            schedule();
        }

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
    if (!p || !p->ready_queue){
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Ready queue is empty");
        return NULL;
    }
        
    struct thread *highest = NULL;
    struct thread *curr = p->ready_queue;
    int highest_priority = -1;
    
    #ifdef DEBUG_THREAD
    // Dump ready queue for debugging
    TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Current ready queue:");
    curr = p->ready_queue;
    while (curr) {
        TRACE_THREAD("  Thread %d (pri %d, policy %d, boost=%d, state=%d)", 
                    curr->tid, curr->priority, curr->policy, 
                    curr->priority_boost, curr->state);
        curr = curr->next_ready;
    }
    #endif

    // First pass: find the highest priority thread
    curr = p->ready_queue;
    while (curr) {
        if (curr->magic == CTXT_MAGIC &&
            !(curr->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING))) {
            
            if (curr->priority > highest_priority) {
                TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Thread %d has higher priority (%d > %d)",
                            curr->tid, curr->priority, highest_priority);
                highest = curr;
                highest_priority = curr->priority;
            }
            else if (curr->priority == highest_priority && curr->tid < highest->tid) {
                TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Thread %d has same priority but lower TID (%d < %d)",
                            curr->tid, curr->tid, highest->tid);
                highest = curr;
            }
        }
        curr = curr->next_ready;
    }
    
    // If we found a valid thread, return it
    if (highest) {
        TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Selected highest priority thread %d (priority %d, policy %d)",
                    highest->tid, highest->priority, highest->policy);
        return highest;
    }
    
    // If no valid thread found, return the first valid one
    curr = p->ready_queue;
    while (curr) {
        if (curr->magic == CTXT_MAGIC &&
            !(curr->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING))) {
            TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): Selected first valid thread %d as fallback",
                        curr->tid);
            return curr;
        }
        curr = curr->next_ready;
    }
    
    TRACE_THREAD("THREAD_SCHED (get_highest_priority_thread): No valid thread found in ready queue");
    return NULL;
}

/**
 * Helper function to reschedule the preemption timer
 */
void reschedule_preemption_timer(PROC *p, long arg)
{
    if (!p)
        return;

    TIMEOUT *new_timeout = NULL;

    TRACE_THREAD("SCHED_TIMER: Rescheduling preemption timer for process %d, arg %ld", p->pid, arg);
    new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
    p->p_thread_timer.timeout = new_timeout;
    p->p_thread_timer.in_handler = 0;
    if (new_timeout) {
        new_timeout->arg = arg;
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
    if (!current || !next){
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Invalid thread (should_schedule_thread)");
        return 0;
    }
        
    // Special case: thread0 is always preemptible by other threads
    if (current->tid == 0) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): thread0 is current, always allowing scheduling");
        return 1;
    }
    
    // If current thread is not running, always schedule next thread
    if (current->state != THREAD_STATE_RUNNING) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Current thread %d is not running, scheduling next thread %d", current->tid, next->tid);
        return 1;
    }
    
    // Get current time for timeslice calculations
    unsigned long current_ticks = *((volatile unsigned long *)_hz_200);
    unsigned long elapsed = current_ticks - current->last_scheduled;
    
    // Check if current thread has used its minimum timeslice
    // This prevents too frequent context switches
    if (elapsed < THREAD_MIN_TIMESLICE) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Current thread %d hasn't used minimum timeslice (%lu < %d), not switching",
                    current->tid, elapsed, THREAD_MIN_TIMESLICE);
        return 0;
    }
    
    // SCHED_FIFO threads (priority > 0) always preempt SCHED_OTHER threads
    if (next->policy == SCHED_FIFO && next->priority > 0 && current->policy == SCHED_OTHER) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_FIFO thread %d (pri %d) preempting SCHED_OTHER thread %d",
                    next->tid, next->priority, current->tid);
        return 1;
    }
    
    // SCHED_RR threads (priority > 0) always preempt SCHED_OTHER threads
    if (next->policy == SCHED_RR && next->priority > 0 && current->policy == SCHED_OTHER) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_RR thread %d (pri %d) preempting SCHED_OTHER thread %d",
                    next->tid, next->priority, current->tid);
        return 1;
    }
    
    // Higher priority always preempts lower priority
    if (next->priority > current->priority) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Higher priority thread %d (pri %d) preempting thread %d (pri %d)",
                    next->tid, next->priority, current->tid, current->priority);
        return 1;
    }
    
    // Equal priority handling depends on scheduling policy
    if (next->priority == current->priority) {
        switch (current->policy) {
            case SCHED_FIFO:
                // SCHED_FIFO threads with equal priority continue running until they
                // yield, block, or are preempted by higher priority threads
                TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_FIFO thread %d continues running (equal priority)",
                            current->tid);
                return 0;
                
            case SCHED_RR:
                // SCHED_RR threads are preempted after their timeslice expires
                if (elapsed >= current->timeslice) {
                    TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_RR thread %d timeslice expired (%lu >= %d), switching to thread %d",
                                current->tid, elapsed, current->timeslice, next->tid);
                    return 1;
                }
                TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_RR thread %d timeslice not expired (%lu < %d), continuing",
                            current->tid, elapsed, current->timeslice);
                return 0;
                
            case SCHED_OTHER:
            default:
                // SCHED_OTHER uses dynamic priority based on time waiting
                // For simplicity, we'll use round-robin for equal static priority
                if (elapsed >= current->timeslice) {
                    TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_OTHER thread %d timeslice expired (%lu >= %d), switching to thread %d",
                                current->tid, elapsed, current->timeslice, next->tid);
                    return 1;
                }
                TRACE_THREAD("THREAD_SCHED (should_schedule_thread): SCHED_OTHER thread %d timeslice not expired (%lu < %d), continuing",
                            current->tid, elapsed, current->timeslice);
                return 0;
        }
    }
    
    // Lower priority SCHED_FIFO/RR threads don't preempt higher priority ones
    if ((next->policy == SCHED_FIFO || next->policy == SCHED_RR) && 
        (current->policy == SCHED_FIFO || current->policy == SCHED_RR)) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): Lower priority RT thread %d (pri %d) not preempting higher priority RT thread %d (pri %d)",
                    next->tid, next->priority, current->tid, current->priority);
        return 0;
    }
    
    // SCHED_OTHER threads only run when no real-time threads are runnable
    if (current->policy == SCHED_OTHER && (next->policy == SCHED_FIFO || next->policy == SCHED_RR)) {
        TRACE_THREAD("THREAD_SCHED (should_schedule_thread): RT thread %d (policy %d) preempting SCHED_OTHER thread %d",
                    next->tid, next->policy, current->tid);
        return 1;
    }
    
    // Default case: don't preempt
    return 0;
}


/* End of Threads stuff */

// /*
// Misc

// */
// /* Spinlock implementation */
// #define SPIN_LOCKED   1
// #define SPIN_UNLOCKED 0
// /* Simple mutex implementation for single-core systems */
// static inline void mutex_lock(short *lock)
// {
//     unsigned short sr = splhigh();
    
//     if (*lock == SPIN_LOCKED) {
//         /* Lock is already held - we need to wait */
//         /* Instead of spinning, we'll yield the CPU */
//         spl(sr);
        
//         /* Try a few times with yielding */
//         int retry_count = 0;
//         while (retry_count < 3) {
//             /* Small yield */
//             {
//                 volatile int i;
//                 for (i = 0; i < 1000; i++);
//             }
            
//             /* Check if lock is available now */
//             sr = splhigh();
//             if (*lock == SPIN_UNLOCKED) {
//                 *lock = SPIN_LOCKED;
//                 return; /* Keep interrupts disabled */
//             }
//             spl(sr);
//             retry_count++;
//         }
        
//         /* If still locked after retries, sleep briefly */
//         /* This allows the lock holder to run */
//         sleep(READY_Q, (long)lock);
        
//         /* When we wake up, try again */
//         sr = splhigh();
//     }
    
//     /* Lock is available */
//     *lock = SPIN_LOCKED;
//     /* Return with interrupts still disabled */
// }

// static inline void mutex_unlock(short *lock)
// {
//     /* Mark as unlocked */
//     *lock = SPIN_UNLOCKED;
    
//     /* Wake up any threads waiting on this lock */
//     wakeup((long)lock);
    
//     /* Restore interrupts */
//     spl(0);
// }