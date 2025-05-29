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

/* Mutex for timer operations */
static int timer_mutex_locked = 0;

static int thread_switch_in_progress = 0;

static TIMEOUT *thread_switch_timeout = NULL;
static struct thread *switching_from = NULL;
static struct thread *switching_to = NULL;
static unsigned long switch_ticks = 0;

#define THREAD_MIN_TIMESLICE 5  // Minimum time a thread should run (in ticks)
#define THREAD_PREEMPT_INTERVAL_TICKS 5  // Preemption interval (in ticks)
#define THREAD_SWITCH_TIMEOUT_MS 500  /* 500ms timeout for thread switches */
#define MAX_SWITCH_RETRIES 3


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
    int from_valid = switching_from && switching_from->magic == CTXT_MAGIC;
    int to_valid = switching_to && switching_to->magic == CTXT_MAGIC;
    
    TRACE_THREAD("TIMEOUT: From thread %s, To thread %s",
                from_valid ? "valid" : "invalid",
                to_valid ? "valid" : "invalid");

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

        /* Small delay before retry - just yield CPU briefly */
        {
            volatile int i;
            for (i = 0; i < 1000; i++);
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

        /* Small delay before retry - just yield CPU briefly */
        {
            volatile int i;
            for (i = 0; i < 1000; i++);
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

// Ready queue management
void add_to_ready_queue(struct thread *t) {
    struct proc *p = t->proc;
    unsigned short sr;
    
    if (!t || (t->state & THREAD_STATE_RUNNING) || (t->state & THREAD_STATE_EXITED)) return;
    if (t->tid == 0 && (t->state & THREAD_STATE_EXITED)) return;
    
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
    TRACE_THREAD("READY_Q: Added Thread %d (pri %d, state %d) to ready queue", 
                t->tid, t->priority, t->state);

    // If thread has priority boost or higher priority than others
    if (t->priority_boost || t->priority > p->pri) {
        // Find the right position based on priority
        // This ensures FIFO order for same-priority threads
        struct thread **pp = &p->ready_queue;
        while (*pp && (*pp)->priority >= t->priority) {
            pp = &(*pp)->next_ready;
        }
        
        // Insert at the right position
        t->next_ready = *pp;
        *pp = t;
        
        if (t->priority_boost) {
            TRACE_THREAD("READY_Q: Thread %d has boosted priority, added in priority order", t->tid);
        }
    } else {
        // Add to end of queue (normal case)
        if (!p->ready_queue) {
            p->ready_queue = t;
            t->next_ready = NULL;
        } else {
            struct thread *last = p->ready_queue;
            while (last->next_ready) {
                last = last->next_ready;
            }
            last->next_ready = t;
            t->next_ready = NULL;
        }
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

    if (!to || from == to || (to->state & THREAD_STATE_EXITED) || from == to) {
        reset_thread_switch_state();
        TRACE_THREAD("SWITCH WARNING: Invalid request - to=%p, from=%p, to_state=%d", to, from, to ? to->state : -1);        
        return;
    }

    // Check magic numbers separately to identify which thread is invalid
    if (from->magic != CTXT_MAGIC) {
        TRACE_THREAD("SWITCH: Invalid 'from' thread magic: %p (magic=%lx)", from, from->magic);
        reset_thread_switch_state();
        return;
    }
    
    if (to->magic != CTXT_MAGIC) {
        TRACE_THREAD("SWITCH: Invalid 'to' thread magic: %p (magic=%lx)", to, to->magic);
        reset_thread_switch_state();
        return;
    }

    if (from->state & THREAD_STATE_EXITED || to->state & THREAD_STATE_EXITED) {
        TRACE_THREAD("SWITCH: Skipping switch involving exited thread (from:%d to:%d)", 
                    from->tid, to->tid);
        reset_thread_switch_state();
        return;
    }

    // Special handling for thread0
    if (from->tid == 0 && (from->state & THREAD_STATE_EXITED)) {
        // Don't switch away from thread0 if it's marked as exited but other threads exist
        if (from->proc->num_threads > 1) {
            TRACE_THREAD("SWITCH: Preventing thread0 exit while other threads are running");
            atomic_thread_state_change(from, THREAD_STATE_READY);
            reset_thread_switch_state();
            return;
        }
    }

    if ((from->state & THREAD_STATE_EXITED) || (to->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("SWITCH: Cannot switch to/from exited thread: from=%d (state=%d), to=%d (state=%d)",
                    from->tid, from->state, to->tid, to->state);
        reset_thread_switch_state();
        return;
    }

    // Disable interrupts during context switch
    unsigned short sr = splhigh();
    
    // Check if another thread switch is in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("SWITCH: Another switch in progress, aborting");
        reset_thread_switch_state();
        spl(sr);
        return;
    }
    
    thread_switch_in_progress = 1;
    
    TRACE_THREAD("SWITCH: Setting switch_in_progress=1");
    /* Store thread pointers for timeout handling */
    switching_from = from;
    switching_to = to;
    
    /* Record start time for timeout detection */
    switch_ticks = *((volatile unsigned long *)_hz_200);
    
    /* Set up timeout to detect deadlocks */
    if (thread_switch_timeout) {
        TRACE_THREAD("SWITCH: Cancelling existing thread switch timeout");
        canceltimeout(thread_switch_timeout);
        thread_switch_timeout = NULL;
    }
    thread_switch_timeout = addtimeout(curproc, THREAD_SWITCH_TIMEOUT_MS / 20, thread_switch_timeout_handler);
    TRACE_THREAD("SWITCH: Set up timeout for %dms", THREAD_SWITCH_TIMEOUT_MS);
    TRACE_THREAD("SWITCH: Switching threads: %d -> %d", from->tid, to->tid);    
    
    // Verify stacks are valid
    if (from->stack_magic != STACK_MAGIC || to->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("SWITCH ERROR: Stack corruption detected! from_magic=%lx, to_magic=%lx", 
                    from->stack_magic, to->stack_magic);
        thread_switch_in_progress = 0;
        reset_thread_switch_state();
        spl(sr);
        return;
    }

    // Vérifie si le thread source est dans la sleep queue
    struct thread *sleep_t = from->proc->sleep_queue;
    int in_sleep_queue = 0;
    
    while (sleep_t) {
        if (sleep_t == from) {
            in_sleep_queue = 1;
            break;
        }
        sleep_t = sleep_t->next_sleeping;
    }

    TRACE_THREAD("SWITCH: From thread in sleep queue: %s", in_sleep_queue ? "yes" : "no");

    // Sauvegarde l'USP du thread courant
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
    // Make sure USP is properly aligned to avoid issues
    current_usp &= ~3L;    
    from->ctxt[CURRENT].usp = current_usp;

    TRACE_THREAD("SWITCH: Saved USP=%lx for thread %d", current_usp, from->tid);

    // Si dans sleep queue, pas besoin de sauvegarder le contexte normal
    if (in_sleep_queue) {
        // For sleeping threads, just update states and switch directly
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        TRACE_THREAD("SWITCH: Sleeping thread, Switching from thread %d to thread %d (sleep queue)", from->tid, to->tid);
        // Prepare destination context
        CONTEXT *to_ctx = get_thread_context(to);
        TRACE_THREAD("SWITCH: Got context %p for thread %d", to_ctx, to->tid);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        reset_thread_switch_state();
        spl(sr);      
        set_thread_usp(to_ctx);
        TRACE_THREAD("SWITCH: calling change_context for thread %d", to->tid);
        change_context(to_ctx);
        TRACE_THREAD("SWITCH ERROR: Returned from change_context in sleep path!");
        
    } else if (save_context(&from->ctxt[CURRENT]) == 0) {
        // Save normal context OK - prepare destination context
        TRACE_THREAD("SWITCH: save_context returned 0, preparing destination context");
        atomic_thread_state_change(from, THREAD_STATE_READY);
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        TRACE_THREAD("IN thread_switch: READY QUEUE, Switching from thread %d to thread %d", from->tid, to->tid);
        CONTEXT *to_ctx = get_thread_context(to);
        TRACE_THREAD("SWITCH: Got context %p for thread %d", to_ctx, to->tid);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        reset_thread_switch_state();
        spl(sr);     
        set_thread_usp(to_ctx);
        TRACE_THREAD("SWITCH: calling change_context for thread %d", to->tid);
        change_context(to_ctx);
        
        TRACE_THREAD("SWITCH ERROR: Returned from change_context in normal path!");
    }
    
    reset_thread_switch_state();
    spl(sr);
    TRACE_THREAD("SWITCH: Completed switch back to thread %d", from->tid);
}

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

    check_and_wake_sleeping_threads(p);

    // If not current process, reschedule the timeout
    if (p != curproc) {
        new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
        if (new_timeout) {
            new_timeout->arg = arg;
            p->p_thread_timer.timeout = new_timeout;
        }
        return;
    }
    
    // Validate thread argument if provided
    if (thread_arg && (thread_arg->magic != CTXT_MAGIC || thread_arg->proc != p)) {
        TRACE_THREAD("PREEMPT: Invalid thread argument %lx, using current thread", arg);
        thread_arg = p->current_thread;
    }
    
    // Protection against reentrance
    if (p->p_thread_timer.in_handler) {
        TRACE_THREAD("PREEMPT: Already in handler, rescheduling");
        // IMPORTANT FIX: Don't add a new timeout if timer is disabled
        if (!p->p_thread_timer.enabled) {
            TRACE_THREAD("PREEMPT: Timer disabled, not rescheduling");
            return;
        }
        
        new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
        if (new_timeout) {
            new_timeout->arg = (long)p->current_thread;
            p->p_thread_timer.timeout = new_timeout;
        }
        return;
    }
    
    // Mark as in handler and clear timeout
    p->p_thread_timer.in_handler = 1;
    p->p_thread_timer.timeout = NULL;
    
    sr = splhigh();
    
    // CRITICAL FIX: Check if timer is enabled before proceeding
    if (!p->p_thread_timer.enabled) {
        TRACE_THREAD("PREEMPT: Timer disabled during handler, exiting");
        p->p_thread_timer.in_handler = 0;
        spl(sr);
        return;
    }
    
    // Only preempt if multiple threads exist
    if (p->num_threads > 1 && p->current_thread) {
        struct thread *curr_thread = p->current_thread;
        
        // Validate current thread
        if (curr_thread->magic != CTXT_MAGIC) {
            TRACE_THREAD("PREEMPT: Current thread has invalid magic, skipping preemption");
            p->p_thread_timer.in_handler = 0;
            spl(sr);
            return;
        }

        // Check if current thread had a priority boost and restore it
        if (curr_thread->priority_boost) {
            TRACE_THREAD("PREEMPT: Restoring thread %d priority from %d to %d",
                        curr_thread->tid, curr_thread->priority, curr_thread->original_priority);
            curr_thread->priority = curr_thread->original_priority;
            curr_thread->priority_boost = 0;
        }

        TRACE_THREAD("PREEMPT: Multiple threads (%d), current=%d, ready_queue=%d",
                    p->num_threads,
                    curr_thread->tid,
                    p->ready_queue ? p->ready_queue->tid : -1);
        
        // First, check sleep queue for threads that should wake up
        if (p->sleep_queue) {
            unsigned long current_time = *((volatile unsigned long *)_hz_200);
            struct thread *sleep_t = p->sleep_queue;
            int woke_threads = 0;
            
            TRACE_THREAD("PREEMPT: Checking sleep queue at time %lu", current_time);
            
            while (sleep_t) {
                struct thread *next_sleep = sleep_t->next_sleeping;
                
                // If thread should wake up
                if (sleep_t->magic == CTXT_MAGIC && 
                    !(sleep_t->state & THREAD_STATE_EXITED) &&
                    sleep_t->wakeup_time > 0 && 
                    sleep_t->wakeup_time <= current_time) {
                    
                    TRACE_THREAD("PREEMPT: Thread %d should wake up (wake at %lu)",
                                sleep_t->tid, sleep_t->wakeup_time);
                                
                    // Wake up thread
                    remove_from_sleep_queue(sleep_t);
                    sleep_t->wakeup_time = 0; // Clear wake-up time
                    atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
                    add_to_ready_queue(sleep_t);
                    woke_threads++;
                }
                
                sleep_t = next_sleep;
            }
            
            if (woke_threads > 0) {
                TRACE_THREAD("PREEMPT: Woke up %d threads from sleep queue", woke_threads);
            }
        }
        
        // Get next thread from ready queue
        struct thread *next = p->ready_queue;
        
        // Filter out invalid or non-ready threads
        while (next && (next->magic != CTXT_MAGIC ||
                      (next->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING)))) {
            TRACE_THREAD("PREEMPT: Skipping invalid thread %d in ready queue", next->tid);
            remove_from_ready_queue(next);
            next = p->ready_queue;
        }
        
        TRACE_THREAD("PREEMPT: After filtering, next thread=%d", next ? next->tid : -1);
        
        // If no thread to execute, reschedule timer and return
        if (!next) {
            TRACE_THREAD("PREEMPT: No ready threads, rescheduling timer");
            p->p_thread_timer.in_handler = 0;
            new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
            if (new_timeout) {
                new_timeout->arg = (long)curr_thread;
                p->p_thread_timer.timeout = new_timeout;
            }
            spl(sr);
            return;
        }
        
        // Don't preempt if switch already in progress
        if (thread_switch_in_progress) {
            TRACE_THREAD("PREEMPT: Thread switch already in progress, rescheduling");
            p->p_thread_timer.in_handler = 0;
            new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
            if (new_timeout) {
                new_timeout->arg = (long)curr_thread;
                p->p_thread_timer.timeout = new_timeout;
            }
            spl(sr);
            return;
        }
        
        // Check if current thread has used its minimum timeslice
        unsigned long current_ticks = *((volatile unsigned long *)_hz_200);
        unsigned long elapsed = current_ticks - curr_thread->last_scheduled;
        
        // Special cases for thread scheduling
        int should_preempt = 0;
        
        if (curr_thread->tid == 0) {
            // Special case: thread0 is always preemptible by other threads
            TRACE_THREAD("PREEMPT: thread0 is current, always allowing preemption");
            should_preempt = 1;
        }
        else if (elapsed < THREAD_MIN_TIMESLICE) {
            // Don't preempt if minimum timeslice hasn't been used
            TRACE_THREAD("PREEMPT: Current thread %d hasn't used minimum timeslice (%lu < %d), not switching",
                        curr_thread->tid, elapsed, THREAD_MIN_TIMESLICE);
            p->p_thread_timer.in_handler = 0;
            new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
            if (new_timeout) {
                new_timeout->arg = (long)curr_thread;
                p->p_thread_timer.timeout = new_timeout;
            }
            spl(sr);
            return;
        }
        else if (curr_thread->priority == next->priority) {
            // Same priority threads - use round-robin regardless of policy
            TRACE_THREAD("PREEMPT: Equal priority threads (%d and %d), using round-robin",
                        curr_thread->tid, next->tid);
            should_preempt = 1;
        }
        else {
            // Apply scheduling policy for different priority threads
            switch (curr_thread->policy) {
                case SCHED_FIFO:
                    // FIFO threads keep running until they yield, block, or a higher priority thread is ready
                    if (curr_thread->priority >= next->priority) {
                        TRACE_THREAD("PREEMPT: FIFO policy - current thread %d has priority %d >= next thread %d priority %d, not switching",
                                    curr_thread->tid, curr_thread->priority, next->tid, next->priority);
                        should_preempt = 0;
                    } else {
                        should_preempt = 1;
                    }
                    break;
                case SCHED_RR:
                    // RR threads are always preempted after their timeslice expires
                    should_preempt = 1;
                    break;
                case SCHED_OTHER:
                    // Default policy - implement fair scheduling
                    if (curr_thread->priority > next->priority) {
                        TRACE_THREAD("PREEMPT: OTHER policy - current thread %d has priority %d > next thread %d priority %d, not switching",
                                    curr_thread->tid, curr_thread->priority, next->tid, next->priority);
                        should_preempt = 0;
                    } else {
                        should_preempt = 1;
                    }
                    break;
                default:
                    // Unknown policy, use round-robin
                    should_preempt = 1;
                    break;
            }
        }
        
        if (!should_preempt) {
            // Don't preempt, just reschedule timer
            p->p_thread_timer.in_handler = 0;
            new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
            if (new_timeout) {
                new_timeout->arg = (long)curr_thread;
                p->p_thread_timer.timeout = new_timeout;
            }
            spl(sr);
            return;
        }
        
        // Remove next from ready queue
        remove_from_ready_queue(next);
        
        TRACE_THREAD("PREEMPT: Current thread state=%d, tid=%d",
                    curr_thread->state, curr_thread->tid);
        
        // Handle current thread based on its state
        if (curr_thread->state == THREAD_STATE_RUNNING) {
            // Check if current thread is in sleep queue
            if (is_in_sleep_queue(curr_thread)) {
                TRACE_THREAD("PREEMPT: Current thread %d is in sleep queue", curr_thread->tid);
                atomic_thread_state_change(curr_thread, THREAD_STATE_SLEEPING);
            } else {
                // Add current thread to ready queue
                TRACE_THREAD("PREEMPT: Adding current thread %d to ready queue", curr_thread->tid);
                atomic_thread_state_change(curr_thread, THREAD_STATE_READY);
                add_to_ready_queue(curr_thread);
            }
        }
        
        // Update next thread state and last scheduled time
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        next->last_scheduled = *((volatile unsigned long *)_hz_200);
        p->current_thread = next;
        
        TRACE_THREAD("PREEMPT: DEBUG: Before thread_switch in preempt handler: curr_thread=%p (tid=%d), next=%p (tid=%d)",
                    curr_thread, curr_thread->tid, next, next->tid);
        
        // Rearm timer before switch
        new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
        if (new_timeout) {
            new_timeout->arg = (long)next;
            p->p_thread_timer.timeout = new_timeout;
        }
        
        // CRITICAL FIX: Clear in_handler flag before thread switch
        p->p_thread_timer.in_handler = 0;
        
        // Perform the thread switch
        thread_switch(curr_thread, next);
        
        // We should never reach here directly after thread_switch
        TRACE_THREAD("PREEMPT: Returned from thread_switch: now running thread %d", 
                    p->current_thread ? p->current_thread->tid : -1);
        
        // If we do reach here, make sure we rearm the timer
        if (!p->p_thread_timer.timeout) {
            new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
            if (new_timeout) {
                new_timeout->arg = (long)p->current_thread;
                p->p_thread_timer.timeout = new_timeout;
            }
        }
    } else {
        // Single thread or no current thread, just rearm timer
        TRACE_THREAD("PREEMPT: Single thread or no current thread, just rearming timer");
        
        // Rearm timer
        p->p_thread_timer.in_handler = 0;
        new_timeout = addtimeout(p, THREAD_PREEMPT_INTERVAL_TICKS, thread_preempt_handler);
        if (new_timeout) {
            new_timeout->arg = (long)p->current_thread;
            p->p_thread_timer.timeout = new_timeout;
        }
    }
    
    // Clear in_handler flag if not already cleared
    p->p_thread_timer.in_handler = 0;
    
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
    t->proc = p;
    t->priority = p->pri;
    
    // Apply priority boost for new thread
    t->original_priority = t->priority;
    t->priority += 3;  // Boost by 3 levels - less than wake boost but still significant
    t->priority_boost = 1;  // Mark as boosted

    t->policy = DEFAULT_SCHED_POLICY;  // Default to FIFO
    t->timeslice = THREAD_DEFAULT_TIMESLICE;
    t->total_timeslice = THREAD_DEFAULT_TIMESLICE;
    t->last_scheduled = 0;

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
    
    // IMPORTANT: If this is the first thread, set it as the current thread
    if (p->num_threads == 1 && t->state != THREAD_STATE_EXITED) {
        atomic_thread_state_change(t, THREAD_STATE_RUNNING);
        p->current_thread = t;
    } else {
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        
        if (p->num_threads == 2) {
            TRACE_THREAD("(create_thread) Starting thread timer for process %d", p->pid);
            start_thread_timing(t);
        }
    }

    TRACE_THREAD("Thread %d created successfully", t->tid);
    spl(sr);
    return t->tid;
}

// Thread context initialization
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
    
    // Set magic values required by FreeMiNT
    t->magic = CTXT_MAGIC;
    
    // Copy to SYSCALL context
    memcpy(&t->ctxt[SYSCALL], &t->ctxt[CURRENT], sizeof(struct context));
}

// Thread start function
void thread_start(void) {
    
    // Get the current thread
    TRACE_THREAD("START: Thread trampoline started");
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("thread_start: No current thread!");
        return;
    }
    
    struct thread *t = p->current_thread;
    TRACE_THREAD("START: Current thread is %d", t->tid);
    // Démarrer le timer de préemption si besoin
    if (p->num_threads > 1 && !p->p_thread_timer.enabled) {
        TRACE_THREAD("START: Starting thread timer for process %d (thread %d)", p->pid, t->tid);
        thread_timer_start(p, t->tid);
    }    
    // Get function and argument from thread structure
    void (*func)(void*) = t->func;
    void *arg = t->arg;
    
    TRACE_THREAD("START: Thread function=%p, arg=%p", func, t->arg);

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

void schedule(void) {
    struct proc *p = curproc;
    if (!p) {
        TRACE_THREAD("SCHEDULE: Invalid current process");
        return;
    }

    unsigned short sr = splhigh();
    
    struct thread *current = p->current_thread;
    struct thread *next = NULL;

    // Check if current thread had a priority boost and restore it
    if (current) {
        if(current->priority_boost) {
            TRACE_THREAD("SCHEDULE: Restoring thread %d priority from %d to %d", 
                        current->tid, current->priority, current->original_priority);
            current->priority = current->original_priority;
            current->priority_boost = 0;
        }
    }

    TRACE_THREAD("SCHEDULE: current thread %d, ready queue head %d",
                current ? current->tid : -1,
                p->ready_queue ? p->ready_queue->tid : -1);
    
    // Check if a thread switch is already in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("SCHEDULE: Thread switch in progress, waiting");
        spl(sr);
        // Instead of recursively calling schedule, use a small delay
        {
            volatile int i;
            for (i = 0; i < 1000; i++);
        }
        return;
    }
    
    // First, check the sleep queue for threads that should wake up
    if (p->sleep_queue) {
        unsigned long current_time = *((volatile unsigned long *)_hz_200);
        struct thread *sleep_t = p->sleep_queue;
        int woke_threads = 0;
        
        TRACE_THREAD("SCHEDULE: Checking sleep queue at time %lu", current_time);
        
        while (sleep_t) {
            struct thread *next_sleep = sleep_t->next_sleeping;
            
            // If thread should wake up
            if (should_wake_thread(sleep_t)) {
                TRACE_THREAD("SCHEDULE: Thread %d should wake up (wake at %lu)",
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
            TRACE_THREAD("SCHEDULE: Woke up %d threads from sleep queue", woke_threads);
            // Refresh the next pointer since we may have added threads to the ready queue
            next = p->ready_queue;
        }
    }

    // Get the next thread from the ready queue
    next = p->ready_queue;
    
    // Filter out invalid or non-ready threads from the ready queue
    while (next && (next->magic != CTXT_MAGIC || 
                   (next->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING)))) {
        TRACE_THREAD("SCHEDULE: Skipping thread %d in ready queue (state: %d, magic: %lx)",
                    next->tid, next->state, next->magic);
        remove_from_ready_queue(next);
        next = p->ready_queue;
    }
    
    // If no ready threads, try to find thread0
    if (!next) {
        TRACE_THREAD("SCHEDULE: No ready threads, looking for thread0");
        struct thread *t0 = p->threads;
        
        while (t0 && (t0->tid != 0 || t0->magic != CTXT_MAGIC || (t0->state & THREAD_STATE_EXITED))) {
            t0 = t0->next;
        }
        
        if (t0) {
            TRACE_THREAD("SCHEDULE: Using thread0");
            next = t0;
        } else {
            TRACE_THREAD("SCHEDULE: No valid threads available");
            spl(sr);
            return;
        }
    }
    
    // If no current thread, switch directly
    if (!current) {
        TRACE_THREAD("SCHEDULE: No current thread, switching to thread %d", next->tid);
        
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }
        
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        CONTEXT *next_ctx = get_thread_context(next);
        if (!next_ctx) {
            TRACE_THREAD("SCHEDULE: Could not get context for thread %d", next->tid);
            spl(sr);
            return;
        }
        
        // Update last scheduled time
        next->last_scheduled = *((volatile unsigned long *)_hz_200);
        
        set_thread_usp(next_ctx);
        spl(sr);
        change_context(next_ctx);
        
        TRACE_THREAD("ERROR: Returned from change_context!");
        return;
    }
    
    // If next is the same as current, try to find another thread
    if (next == current) {
        next = next->next_ready;
        
        while (next && (next->magic != CTXT_MAGIC || 
                       (next->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING)))) {
            next = next->next_ready;
        }
        
        if (!next) {
            TRACE_THREAD("SCHEDULE: No other valid threads available");
            spl(sr);
            return;
        }
    }
    
    // Special cases for thread scheduling:
    // 1. thread0 is always preemptible
    // 2. Ensure fair distribution between thread1 and thread2 if they have the same priority
    // 3. Apply normal scheduling policies otherwise
    
    int should_schedule = 0;
    
    if (current->tid == 0) {
        // Special case: thread0 is always preemptible by other threads
        TRACE_THREAD("SCHEDULE: thread0 is current, always allowing scheduling");
        should_schedule = 1;
    }
    else if (current->state == THREAD_STATE_RUNNING) {
        // Check if current thread has used its minimum timeslice
        unsigned long current_ticks = *((volatile unsigned long *)_hz_200);
        unsigned long elapsed = current_ticks - current->last_scheduled;
        
        if (elapsed < THREAD_MIN_TIMESLICE) {
            TRACE_THREAD("SCHEDULE: Current thread %d hasn't used minimum timeslice (%lu < %d), not switching",
                        current->tid, elapsed, THREAD_MIN_TIMESLICE);
            spl(sr);
            return;
        }
        
        if (current->priority == next->priority) {
            // Same priority threads - use round-robin regardless of policy
            // This ensures fair distribution between thread1 and thread2
            TRACE_THREAD("SCHEDULE: Equal priority threads (%d and %d), using round-robin",
                        current->tid, next->tid);
            should_schedule = 1;
        }
        else {
            // Apply scheduling policy for different priority threads
            switch (current->policy) {
                case SCHED_FIFO:
                    // FIFO threads keep running until they yield, block, or a higher priority thread is ready
                    if (current->priority >= next->priority) {
                        TRACE_THREAD("SCHEDULE: FIFO policy - current thread %d has priority %d >= next thread %d priority %d, not switching",
                                    current->tid, current->priority, next->tid, next->priority);
                        should_schedule = 0;
                    } else {
                        should_schedule = 1;
                    }
                    break;
                    
                case SCHED_RR:
                    // RR threads are always preempted after their timeslice expires
                    should_schedule = 1;
                    break;
                    
                case SCHED_OTHER:
                    // Default policy - implement fair scheduling
                    // If current thread has higher priority, don't switch
                    if (current->priority > next->priority) {
                        TRACE_THREAD("SCHEDULE: OTHER policy - current thread %d has priority %d > next thread %d priority %d, not switching",
                                    current->tid, current->priority, next->tid, next->priority);
                        should_schedule = 0;
                    } else {
                        should_schedule = 1;
                    }
                    break;
                    
                default:
                    // Unknown policy, use round-robin
                    should_schedule = 1;
                    break;
            }
        }
    } else {
        // Current thread is not running, always schedule next thread
        should_schedule = 1;
    }
    
    if (!should_schedule) {
        spl(sr);
        return;
    }
    
    TRACE_THREAD("SCHEDULE: Switching threads: %d -> %d", current->tid, next->tid);
    
    // Remove next thread from ready queue
    if (is_in_ready_queue(next)) {
        remove_from_ready_queue(next);
    }
    
    // Handle current thread based on its state
    if (current->state == THREAD_STATE_RUNNING) {
        // Check if current thread is in sleep queue
        if (is_in_sleep_queue(current)) {
            TRACE_THREAD("SCHEDULE: Current thread %d is in sleep queue", current->tid);
            atomic_thread_state_change(current, THREAD_STATE_SLEEPING);
        } else {
            TRACE_THREAD("SCHEDULE: Adding current thread %d to ready queue", current->tid);
            atomic_thread_state_change(current, THREAD_STATE_READY);
            add_to_ready_queue(current);
        }
    }
    
    // Update next thread state and last scheduled time
    atomic_thread_state_change(next, THREAD_STATE_RUNNING);
    next->last_scheduled = *((volatile unsigned long *)_hz_200);
    p->current_thread = next;
    
    // Perform the thread switch
    spl(sr);
    TRACE_THREAD("SCHEDULE: Calling thread_switch");
    thread_switch(current, next);
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
}

// Thread yielding
long _cdecl sys_p_yieldthread(void)
{
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("sys_p_yield: invalid current process/thread");
        return -EINVAL;
    }

    TRACE_THREAD("sys_p_yield: thread %d (pri %d) yielding", 
                p->current_thread->tid, p->current_thread->priority);

    struct thread *current = p->current_thread;
    
    // Don't yield if we're the only thread
    if (p->num_threads <= 1) {
        TRACE_THREAD("Only one thread, not yielding");
        return 0;
    }
    
    // Save current SR
    unsigned short sr = splhigh();
    
    // Verify stack integrity
    if (current->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("ERROR: Stack corruption detected during yield!");
        spl(sr);
        return -EFAULT;
    }
    
    // Check if a thread switch is already in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("sys_p_yield: thread switch already in progress, not yielding");
        spl(sr);
        return 0;
    }
    
    // Check if there are any ready threads
    struct thread *next = p->ready_queue;
    
    // If no ready threads, check if there are sleeping threads that should wake up
    if (!next && p->sleep_queue) {
        TRACE_THREAD("sys_p_yield: No ready threads, checking sleep queue");
        
        // Get current time
        unsigned long current_time = *((volatile unsigned long *)_hz_200);
        
        // Check if any sleeping threads should wake up
        struct thread *sleep_t = p->sleep_queue;
        while (sleep_t) {
            struct thread *next_sleep = sleep_t->next_sleeping;
            
            // If thread should wake up
            if (sleep_t->wakeup_time <= current_time) {
                TRACE_THREAD("sys_p_yield: Waking up thread %d from sleep queue", sleep_t->tid);
                
                // Skip invalid or exited threads
                if (sleep_t->magic != CTXT_MAGIC || (sleep_t->state & THREAD_STATE_EXITED)) {
                    TRACE_THREAD("sys_p_yield: Skipping invalid thread %d in sleep queue", sleep_t->tid);
                    remove_from_sleep_queue(sleep_t);
                    sleep_t = next_sleep;
                    continue;
                }
                
                // Wake up thread
                remove_from_sleep_queue(sleep_t);
                atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
                add_to_ready_queue(sleep_t);
            }
            
            sleep_t = next_sleep;
        }
        
        // Check if we now have ready threads
        next = p->ready_queue;
    }
    
    // If still no ready threads, just return
    if (!next) {
        TRACE_THREAD("No other threads to run after yield");
        
        // If current thread is in RUNNING state, keep it there
        if (current->state == THREAD_STATE_RUNNING) {
            // No state change needed
        } else {
            // Otherwise, set it to READY and add to ready queue
            atomic_thread_state_change(current, THREAD_STATE_READY);
            add_to_ready_queue(current);
        }
        
        spl(sr);
        return 0;
    }
    
    // If next is the same as current, try to find another thread
    if (next == current) {
        // Try to find another thread in the ready queue
        next = next->next_ready;
        
        // If no other thread, just return
        if (!next) {
            TRACE_THREAD("Next thread is current, not switching");
            remove_from_ready_queue(current);
            atomic_thread_state_change(current, THREAD_STATE_RUNNING);
            spl(sr);
            return 0;
        }
    }
    
    // Verify next thread's stack integrity
    if (next->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("ERROR: Next thread stack corruption detected!");
        
        // Try to find another valid thread
        remove_from_ready_queue(next);
        next = p->ready_queue;
        
        // If no valid threads to switch to, just return
        if (!next || next == current || next->stack_magic != STACK_MAGIC) {
            TRACE_THREAD("No valid threads to switch to");
            
            // If current thread is in RUNNING state, keep it there
            if (current->state == THREAD_STATE_RUNNING) {
                // No state change needed
            } else {
                // Otherwise, set it to READY and add to ready queue
                atomic_thread_state_change(current, THREAD_STATE_READY);
                add_to_ready_queue(current);
            }
            
            spl(sr);
            return -EFAULT;
        }
    }
    
    // Remove next thread from ready queue
    remove_from_ready_queue(next);
    
    // Check if current thread is in the sleep queue using the existing function
    int in_sleep_queue = is_in_sleep_queue(current);
    
    // Handle current thread based on its state and whether it's in the sleep queue
    if (in_sleep_queue) {
        // If in sleep queue, maintain SLEEPING state
        TRACE_THREAD("Current thread %d is in sleep queue, maintaining SLEEPING state", current->tid);
        atomic_thread_state_change(current, THREAD_STATE_SLEEPING);
        // No need to re-add to sleep queue as it's already there
    } else if (current->state == THREAD_STATE_RUNNING) {
        // If running and not in sleep queue, set to READY and add to ready queue
        atomic_thread_state_change(current, THREAD_STATE_READY);
        add_to_ready_queue(current);
        TRACE_THREAD("Added current thread %d to ready queue", current->tid);
    }
    
    // Update thread states
    atomic_thread_state_change(next, THREAD_STATE_RUNNING);
    p->current_thread = next;
    
    TRACE_THREAD("IN YIELD: Yielding from thread %d to thread %d", current->tid, next->tid);
    
    // Get context for next thread
    CONTEXT *next_ctx = get_thread_context(next);
    if (!next_ctx) {
        TRACE_THREAD("ERROR: Could not get context for next thread %d", next->tid);
        
        // Restore current thread as running
        p->current_thread = current;
        atomic_thread_state_change(current, THREAD_STATE_RUNNING);
        remove_from_ready_queue(current);
        
        // Add next thread back to ready queue
        atomic_thread_state_change(next, THREAD_STATE_READY);
        add_to_ready_queue(next);
        
        spl(sr);
        return -EFAULT;
    }
    
    // Set USP for next thread
    set_thread_usp(next_ctx);
    
    // Re-enable interrupts before context switch to allow I/O to flush
    spl(sr);
    
    // Save current context and switch to next thread
    if (save_context(&current->ctxt[CURRENT]) == 0) {
        // This is the initial save_context call
        change_context(next_ctx);
        
        // We should never reach here
        TRACE_THREAD("ERROR: Returned from change_context!");
    } else {
        // This is the return from save_context after another thread has yielded to us
        TRACE_THREAD("Thread %d resumed after yield", current->tid);
        
        // Call schedule to ensure proper thread scheduling
        schedule();
    }
    
    return 0;
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

void check_and_wake_sleeping_threads(struct proc *p)
{
    if (!p || !p->sleep_queue)
        return;
        
    unsigned long current_time = *((volatile unsigned long *)_hz_200);
    struct thread *sleep_t = p->sleep_queue;
    int woke_threads = 0;
    
    TRACE_THREAD("SLEEP_CHECK: Checking sleep queue at time %lu", current_time);
    
    while (sleep_t) {
        struct thread *next_sleep = sleep_t->next_sleeping;
        
        // If thread should wake up
        if (sleep_t->magic == CTXT_MAGIC && 
            !(sleep_t->state & THREAD_STATE_EXITED) &&
            sleep_t->wakeup_time > 0 && 
            sleep_t->wakeup_time <= current_time) {
            
            TRACE_THREAD("SLEEP_CHECK: Thread %d should wake up (wake at %lu, current %lu, delay %lu ms)",
                        sleep_t->tid, sleep_t->wakeup_time, current_time, 
                        (current_time - sleep_t->wakeup_time) * 5);
                        
            // Wake up thread with priority boost
            remove_from_sleep_queue(sleep_t);
            
            // Boost priority
            sleep_t->original_priority = sleep_t->priority;
            sleep_t->priority = sleep_t->priority + 5;  // Boost by 5 levels instead of using a constant
            sleep_t->priority_boost = 1;

            sleep_t->wakeup_time = 0;  // Clear wake-up time
            atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
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
/* End of Threads stuff */