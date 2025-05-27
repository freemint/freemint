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

static void init_thread_idle_context(struct thread *t);
static void thread_idle_loop(struct thread *t);
static void idle_start(void);

static CONTEXT* get_thread_context(struct thread *t);
static void set_thread_usp(CONTEXT *ctx);

/* Mutex for timer operations */
static int timer_mutex_locked = 0;

static int thread_switch_in_progress = 0;

static TIMEOUT *thread_switch_timeout = NULL;
static struct thread *switching_from = NULL;
static struct thread *switching_to = NULL;
static unsigned long switch_ticks = 0;

#define THREAD_SWITCH_TIMEOUT_MS 500  /* 500ms timeout for thread switches */
#define MAX_SWITCH_RETRIES 3

/* Signal Threads implementation */
/**
 * Timeout handler for sleeping threads.
 *
 * This function is called when a thread sleeping timeout expires.
 * It will wake up the thread and put it back into the ready queue,
 * unless the thread is not sleeping or is not blocked.
 *
 * @param p: the process containing the thread
 * @param arg: ignored
 */
void thread_timeout_handler(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    TRACE_THREAD("thread_timeout_handler: thread %d timeout", t ? t->tid : -1);
    if(!t) return;
    unsigned short sr = splhigh();
    if ((t->state & THREAD_STATE_BLOCKED) && t->wait_type == WAIT_SIGNAL) {
        t->sleep_reason = 1; // Timeout
        t->wait_type = WAIT_NONE;
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
    }
    spl(sr);
}

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
    
    if (++recovery_attempts > MAX_SWITCH_RETRIES) {
        TRACE_THREAD("CRITICAL: Max recovery attempts reached, system may be unstable");
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
    
    if (!t)
        return;

    /* Check if the new state is valid */
    if ((t->state & THREAD_STATE_EXITED) && !(new_state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("ERROR: Attempt to change state of EXITED thread %d from %d to %d", t->tid, t->state, new_state);
        return;
    }

    sr = splhigh();
    TRACE_THREAD("Thread %d state change: %d -> %d", t->tid, t->state, new_state);
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
    TRACE_THREAD("thread_timer: enabled set to 0 by %s:%d", __FILE__, __LINE__);
    p->p_thread_timer.timeout = NULL;
    p->p_thread_timer.in_handler = 0;
    
    TRACE_THREAD(("Thread timer initialized"));
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
    TRACE_THREAD("Starting thread timer for process %d", p->pid);
    
    /* Use a structured approach to ensure cleanup */
    int success = 0;
    
    sr = splhigh();
    
    /* If timer is already enabled, don't add another timeout */
    if (p->p_thread_timer.enabled && p->p_thread_timer.timeout) {
        TRACE_THREAD("Timer already enabled, not adding another timeout");
        success = 1;
        spl(sr);
        goto cleanup;
    }
    
    /* Create the timeout before modifying any state */
    new_timeout = addtimeout(p, 20, thread_preempt_handler);
    if (!new_timeout) {
        TRACE_THREAD("ERROR: Failed to create timeout");
        spl(sr);
        goto cleanup;
    }
    new_timeout->arg = (long)p->current_thread;
    /* Now that we have a valid timeout, update the timer state */
    p->p_thread_timer.timeout = new_timeout;
    TRACE_THREAD("Setting up preemption timer for thread %d == %d", p->current_thread->tid, p->p_thread_timer.thread_id);
    p->p_thread_timer.thread_id = p->current_thread->tid;
    /* Set enabled flag last to ensure everything is set up */
    TRACE_THREAD("Thread timer started for process %d", p->pid);
    p->p_thread_timer.enabled = 1;
    TRACE_THREAD("thread_timer: enabled set to 1 by %s:%d", __FILE__, __LINE__);
    p->p_thread_timer.in_handler = 0;
    
    success = 1;
    spl(sr);
    
cleanup:
    /* Always release the mutex */
    sr = splhigh();
    timer_mutex_locked = 0;
    spl(sr);
    
    TRACE_THREAD("thread_timer_start completed with %s", success ? "success" : "failure");
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
    
    int success = 0;
    
    sr = splhigh();
    
    /* Check if timer is already disabled */
    if (p->num_threads <= 1) {
        p->p_thread_timer.enabled = 0;
        TRACE_THREAD("thread_timer: enabled set to 0 by %s:%d", __FILE__, __LINE__);
    }
    
    /* Save the timeout pointer locally before clearing it */
    timeout_to_cancel = p->p_thread_timer.timeout;
    p->p_thread_timer.timeout = NULL;
    
    success = 1;
    spl(sr);
    
    /* Cancel the timeout outside the critical section */
    if (timeout_to_cancel) {
        canceltimeout(timeout_to_cancel);
    }
    
    /* Always release the mutex */
    sr = splhigh();
    timer_mutex_locked = 0;
    spl(sr);
    
    TRACE_THREAD("thread_timer_stop completed with %s", success ? "success" : "failure");
}

/*
 * Function to start timing a newly created thread
 * Call this after thread creation
 */
void start_thread_timing(struct thread *t)
{
    TRACE_THREAD("(start_thread_timing) start_thread_timing called for thread %d", t->tid);
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
            TRACE_THREAD("Thread %d already in ready queue", t->tid);
            spl(sr);
            return;
        }
        curr = curr->next_ready;
    }

    atomic_thread_state_change(t, THREAD_STATE_READY);
    TRACE_THREAD("ADD_TO_READY_Q: Thread %d (pri %d, state %d)", t->tid, t->priority, t->state);

    // Add to end of queue
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

    TRACE_THREAD("REMOVE_FROM_READY_Q: Thread %d", t->tid);

    struct thread **pp = &p->ready_queue;
    struct thread *curr = p->ready_queue;
    int found = 0;

    while (curr) {
        if (curr == t) {
            *pp = curr->next_ready;
            curr->next_ready = NULL;
            found = 1;
            TRACE_THREAD("Removed thread %d from ready queue", t->tid);
            break;
        }
        pp = &(curr->next_ready);
        curr = curr->next_ready;
    }

    if (!found) {
        TRACE_THREAD("Thread %d not found in ready queue", t->tid);
    }

    spl(sr);
}

void thread_switch(struct thread *from, struct thread *to) {
    if (!to || from == to || (to->state & THREAD_STATE_EXITED)) {
        TRACE_THREAD("Invalid thread switch request");
        reset_thread_switch_state();
        return;
    }
    
    // Disable interrupts during context switch
    unsigned short sr = splhigh();
    
    // Check if another thread switch is in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("Thread switch already in progress, aborting");
        reset_thread_switch_state();
        spl(sr);
        return;
    }
    
    thread_switch_in_progress = 1;
    
    /* Store thread pointers for timeout handling */
    switching_from = from;
    switching_to = to;
    
    /* Record start time for timeout detection */
    switch_ticks = *((volatile unsigned long *)_hz_200);
    
    /* Set up timeout to detect deadlocks */
    if (thread_switch_timeout) {
        TRACE_THREAD("Cancelling existing thread switch timeout");
        canceltimeout(thread_switch_timeout);
    }
    thread_switch_timeout = addtimeout(curproc, THREAD_SWITCH_TIMEOUT_MS / 20, thread_switch_timeout_handler);
        
    TRACE_THREAD("Switching threads: %d -> %d", from->tid, to->tid);
    
    // Verify stacks are valid
    if (from->stack_magic != STACK_MAGIC || to->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("ERROR: Stack corruption detected during thread switch!");
        thread_switch_in_progress = 0;
        spl(sr);
        reset_thread_switch_state();
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
    from->ctxt[CURRENT].usp = current_usp;

    // Si dans sleep queue, pas besoin de sauvegarder le contexte normal
    if (in_sleep_queue) {
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        TRACE_THREAD("IN thread_switch: SLEEP QUEUE, Switching from thread %d to thread %d (sleep queue)", from->tid, to->tid);
        // Prépare le contexte destination
        CONTEXT *to_ctx = get_thread_context(to);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        reset_thread_switch_state();
        set_thread_usp(to_ctx);
        change_context(to_ctx);
        
    } else if (save_context(&from->ctxt[CURRENT]) == 0) {
        // Save normal context OK - prepare destination context
        atomic_thread_state_change(from, THREAD_STATE_READY);
        atomic_thread_state_change(to, THREAD_STATE_RUNNING);
        from->proc->current_thread = to;
        TRACE_THREAD("IN thread_switch: READY QUEUE, Switching from thread %d to thread %d", from->tid, to->tid);
        CONTEXT *to_ctx = get_thread_context(to);
        if (!to_ctx) {
            reset_thread_switch_state();
            spl(sr);
            return;
        }
        reset_thread_switch_state();
        set_thread_usp(to_ctx);
        change_context(to_ctx);
        
        TRACE_THREAD("ERROR: Returned from change_context!");
    }
    
    // Return from context switch
    TRACE_THREAD("Returned to thread %d after context switch", from->tid);
    reset_thread_switch_state();
    spl(sr);
}

void thread_preempt_handler(PROC *p, long arg)
{

    unsigned short sr;
    TIMEOUT *new_timeout = NULL;
    
    // TRACE_THREAD("thread_preempt_handler called for process %d", p->pid);
    
    if (p != curproc) {
        p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        p->p_thread_timer.timeout->arg = arg;
        return;
    }
    
    /* Protection contre la réentrance */
    if (p->p_thread_timer.in_handler) {
        if (!p->p_thread_timer.enabled) {
            p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
            p->p_thread_timer.timeout->arg = arg;
            return;
        }
        p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        p->p_thread_timer.timeout->arg = arg;
        return;
    }
    
    p->p_thread_timer.in_handler = 1;
    p->p_thread_timer.timeout = NULL;
    
    sr = splhigh();
    
    /* Ne préempte que si plusieurs threads */
    if (p->num_threads > 1 && p->current_thread) {
        struct thread *curr_thread = p->current_thread;
        struct thread *next = p->ready_queue;
        
        // Vérifie la sleep queue si pas de thread ready
        if (!next && p->sleep_queue) {
            struct thread *sleep_t = p->sleep_queue;
            while (sleep_t && sleep_t->wakeup_time <= *((volatile unsigned long *)_hz_200)) {
                struct thread *next_sleep = sleep_t->next_sleeping;
                remove_from_sleep_queue(sleep_t);
                if(sleep_t->state & THREAD_STATE_EXITED) {
                    sleep_t = next_sleep;
                    continue; // Skip exited threads
                }
                atomic_thread_state_change(sleep_t, THREAD_STATE_READY);
                add_to_ready_queue(sleep_t);
                sleep_t = next_sleep;
            }
            next = p->ready_queue;
        }
        
        /* Si pas de thread à exécuter */
        if (!next) {
            new_timeout = addtimeout(p, 20, thread_preempt_handler);
            p->p_thread_timer.timeout = new_timeout;
            p->p_thread_timer.in_handler = 0;
            return;
        }
        
        /* Pas de préemption si switch déjà en cours */
        if (thread_switch_in_progress) {
            new_timeout = addtimeout(p, 20, thread_preempt_handler);
            new_timeout->arg = (long)curr_thread;
            p->p_thread_timer.timeout = new_timeout;
            p->p_thread_timer.in_handler = 0;
            return;
        }
        
        /* Retire next de la ready queue */
        remove_from_ready_queue(next);
        
        /* Ajoute current à la ready queue si RUNNING et pas thread0 */
        if ((curr_thread->state == THREAD_STATE_RUNNING) && curr_thread->tid != 0) {
            atomic_thread_state_change(curr_thread, THREAD_STATE_READY);
            add_to_ready_queue(curr_thread);
        }
        
        /* Met à jour les états et fait le switch */
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        
        /* Réarme le timer avant le switch */
        TRACE_THREAD("CALLING thread_switch: Switching from thread %d to thread %d", curr_thread->tid, next->tid);
        thread_switch(curr_thread, next);

        TRACE_THREAD("Returned from thread_switch: now running thread %d, rearming timer", next->tid);
        new_timeout = addtimeout(p, 20, thread_preempt_handler);
        new_timeout->arg = (long)next;
        p->p_thread_timer.timeout = new_timeout;
        p->p_thread_timer.in_handler = 0;

        spl(sr);        
    }
    
    /* Réarme le timer si pas de switch */
    new_timeout = addtimeout(p, 20, thread_preempt_handler);
    new_timeout->arg = (long)p->current_thread;
    p->p_thread_timer.timeout = new_timeout;
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
    atomic_thread_state_change(t, THREAD_STATE_READY);

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
    
    t->idle_stack = NULL;  // No idle stack initially
    t->idle_stack_top = NULL;  // No idle stack top initially
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
        // Add to ready queue and start timing if needed
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
    TRACE_THREAD("Thread trampoline started");
    
    // Get the current thread
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("thread_start: No current thread!");
        return;
    }
    
    struct thread *t = p->current_thread;
    // Démarrer le timer de préemption si besoin
    if (p->num_threads > 1 && !p->p_thread_timer.enabled) {
        TRACE_THREAD("thread_start: Starting thread timer for process %d (thread %d)", p->pid, t->tid);
        thread_timer_start(p, t->tid);
    }    
    // Get function and argument from thread structure
    void (*func)(void*) = t->func;
    void *arg = t->arg;
    
    TRACE_THREAD("Thread function=%p, arg=%p", func, arg);
    
    // Call the function
    if (func) {
        TRACE_THREAD("Calling thread function");
        func(arg);
        TRACE_THREAD("Thread function returned");
    }
    
    // Exit the thread when done
    TRACE_THREAD("Thread exiting");
    thread_exit();
    
    // Should never reach here
    TRACE_THREAD("ERROR: Thread didn't exit properly!");
    while(1) { /* hang */ }
}

/*
Thread scheduling
Purpose:
    The scheduler decides which thread should run next.
    It examines the ready queue, picks the next eligible thread, and (if needed) initiates a context switch.
What it does:
    Looks at all threads (or processes) that are ready to run.
    Picks the "best" one (often the next in the ready queue, or based on priority).
    If the chosen thread is not the current one, it calls thread_switch(current, next) to actually perform the switch.
*/
void schedule(void) {
    struct proc *p = curproc;
    unsigned short sr;
    
    if (!p) {
        TRACE_THREAD("schedule: Invalid current process");
        return;
    }

    sr = splhigh();
    
    struct thread *current = p->current_thread;
    struct thread *next = p->ready_queue;

    TRACE_THREAD("In schedule: current thread %d, ready queue head %d", 
                current ? current->tid : -1, 
                next ? next->tid : -1);

    // Ignore les threads non READY
    while (next && (next->state & (THREAD_STATE_EXITED | THREAD_STATE_BLOCKED | THREAD_STATE_SLEEPING))) {
        TRACE_THREAD("Skipping thread %d in ready queue (state: %d)", next->tid, next->state);
         // Si le thread est dans la sleep queue, le retirer
        remove_from_ready_queue(next);
        next = p->ready_queue;
    }

    TRACE_THREAD("schedule: next=%d", next ? next->tid : -1);

    if (thread_switch_in_progress) {
        TRACE_THREAD("Thread switch in progress, waiting");
        // Au lieu de forcer le reset, on attend que le switch se termine naturellement
        spl(sr);
        // Petit délai puis on réessaie
        {
            volatile int i;
            for(i = 0; i < 1000; i++);
        }
        schedule();  // Réessayer plus tard
        return;
    }

    // Si pas de thread READY, regarder les threads SLEEPING
    if (!next && p->sleep_queue) {
        TRACE_THREAD("No READY threads, checking SLEEPING threads");
        next = p->sleep_queue;
        
        // Réveiller les threads dont le temps est écoulé
        while (next && next->wakeup_time <= *((volatile unsigned long *)_hz_200)) {
            struct thread *t = next;
            next = next->next_sleeping;
            
            remove_from_sleep_queue(t);
            if (t->state == THREAD_STATE_EXITED) continue; // Ne pas réveiller les threads EXITED;
            atomic_thread_state_change(t, THREAD_STATE_READY);
            add_to_ready_queue(t);
        }
        
        // Si encore des threads sleeping, prendre le premier
        if (!p->ready_queue && p->sleep_queue) {
            next = p->sleep_queue;
        } else {
            next = p->ready_queue;
        }
    }

    // Si pas de thread prêt, chercher thread0
    if (!next) {
        struct thread *t0 = p->threads;

        while (t0 && t0->tid != 0) t0 = t0->next;

        if (t0 && !(t0->state & THREAD_STATE_EXITED)) {
            TRACE_THREAD("No ready threads >0, switching to thread0");
            next = t0;
        } else {
            TRACE_THREAD("No ready threads available");
            spl(sr);
            return;
        }
    }
    
    // Si pas de thread courant, switch direct
    if (!current) {
        TRACE_THREAD("No current thread, switching to thread %d", next->tid);
        remove_from_ready_queue(next);
        atomic_thread_state_change(next, THREAD_STATE_RUNNING);
        p->current_thread = next;
        TRACE_THREAD("IN schedule: No current thread, Switching to thread %d", next->tid);
        CONTEXT *next_ctx = get_thread_context(next);
        set_thread_usp(next_ctx);
        
        change_context(&next->ctxt[CURRENT]);
        TRACE_THREAD("ERROR: Returned from change_context!");
        spl(sr);
        return;
    }

    // Ne jamais switcher vers un thread EXITED
    if (next->state & THREAD_STATE_EXITED) {
        TRACE_THREAD("Next thread is EXITED, not switching");
        remove_from_ready_queue(next);
        spl(sr);
        return;
    }

    if (next == current) {
        // Chercher le prochain thread ready dans la queue
        next = next->next_ready;
        while (next && next->state != THREAD_STATE_READY) {
            next = next->next_ready; 
        }
        
        if (!next) {
            TRACE_THREAD("No other ready threads available");
            spl(sr);
            return;
        }
    }

    TRACE_THREAD("Switching threads: %d -> %d", current->tid, next->tid);
    
    // Prépare le switch
    if (is_in_ready_queue(next)) {
        remove_from_ready_queue(next);
    }
    
    // Ne modifie l'état du thread courant que s'il est RUNNING
    if (current->state == THREAD_STATE_RUNNING) {
        // Vérifie si le thread est dans la sleep queue
        struct thread *sleep_t = p->sleep_queue;
        int in_sleep_queue = 0;
        while (sleep_t) {
            if (sleep_t == current) {
                in_sleep_queue = 1;
                break;
            }
            sleep_t = sleep_t->next_sleeping;
        }

        // Si dans la sleep queue, remettre en SLEEPING, sinon en READY
        if (in_sleep_queue) {
            atomic_thread_state_change(current, THREAD_STATE_SLEEPING);
        } else {
            if (current->state == THREAD_STATE_EXITED) return; // Ne jamais switcher vers un thread EXITED
            atomic_thread_state_change(current, THREAD_STATE_READY);
            add_to_ready_queue(current);
        }
    }
    
    // Met à jour l'état du prochain thread en dernier
    atomic_thread_state_change(next, THREAD_STATE_RUNNING);

    p->current_thread = next;
    
    // Effectue le switch
    thread_switch(current, next);
    spl(sr);
}

// Thread exit
void thread_exit(void) {
    struct proc *p = curproc;
    if (!p) {
        TRACE_THREAD("thread_exit: No current process");
        return;
    }
    struct thread *current = p->current_thread;
    
    if (!current) {
        TRACE_THREAD("thread_exit: No current thread");
        return;
    }

    static int thread_exit_in_progress = 0;
    static int exit_owner_tid = -1;
    unsigned short sr = splhigh();

    // Protection contre la réentrance
    if (thread_exit_in_progress && exit_owner_tid != current->tid) {
        spl(sr);
        TRACE_THREAD("Thread exit already in progress by thread %d, waiting", exit_owner_tid);
        thread_exit();
        return;
    }

    int tid = current->tid;
    thread_exit_in_progress = 1;
    exit_owner_tid = tid;

    // Marquer EXITED et retirer des queues
    atomic_thread_state_change(current, THREAD_STATE_EXITED);
    remove_from_ready_queue(current);
    remove_from_sleep_queue(current);

    // Retirer de la liste des threads
    struct thread **tp;
    for (tp = &p->threads; *tp; tp = &(*tp)->next) {
        if (*tp == current) {
            *tp = current->next;
            break;
        }
    }
    p->num_threads--;

    // Gestion des timers
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

    // Chercher le prochain thread AVANT de libérer quoi que ce soit
    struct thread *next = p->ready_queue;
    CONTEXT saved_ctx;

    if (next) {
        // Sauvegarder le contexte du prochain thread
        if (is_in_ready_queue(next)) {
            remove_from_ready_queue(next);
        }

        if (next->state & THREAD_STATE_EXITED) {
            next = NULL;
        } else {
            CONTEXT *next_ctx = get_thread_context(next);
            if (next_ctx) {
                memcpy(&saved_ctx, next_ctx, sizeof(CONTEXT));
                atomic_thread_state_change(next, THREAD_STATE_RUNNING);
                p->current_thread = next;
            } else {
                p->current_thread = NULL; // Clear current thread if context is invalid
                next = NULL;
            }
        }
    }

    if (!next) {
        // Pas de thread suivant, préparer le retour au process
        memcpy(&saved_ctx, &p->ctxt[CURRENT], sizeof(CONTEXT));
        p->current_thread = NULL;
    }

    if(current){
        // Libérer les ressources du thread courant
        if (current->t_sigctx) {
            kfree(current->t_sigctx);
            current->t_sigctx = NULL;
        }
        if (current->idle_stack) {
            kfree(current->idle_stack);
            current->idle_stack = NULL;
        }
        if (current->stack && tid != 0) {
            kfree(current->stack);
            current->stack = NULL;
        }

        // Libérer la structure thread
        current->magic = 0;
        TRACE_THREAD("KFREE: Freeing thread %d", tid);
        kfree(current);
        current = NULL;
        TRACE_THREAD("Thread %d exited", tid);
    } else {
        TRACE_THREAD("Attempt to exit thread id %d, but No current thread to free", tid);
    }

    thread_exit_in_progress = 0;
    exit_owner_tid = -1;
    spl(sr);

    // Switch vers le prochain contexte
    change_context(&saved_ctx);
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
    atomic_thread_state_change(t0, THREAD_STATE_RUNNING);
    
    // Use process stack for thread0
    t0->stack = p->stack;
    t0->stack_top = (char*)p->stack + STKSIZE;
    t0->stack_magic = STACK_MAGIC;
    
    // Copy process context to thread context
    memcpy(&t0->ctxt[CURRENT], &p->ctxt[CURRENT], sizeof(CONTEXT));
    memcpy(&t0->ctxt[SYSCALL], &p->ctxt[SYSCALL], sizeof(CONTEXT));
    
    // Link into process
    t0->next = NULL;
    p->threads = t0;
    p->current_thread = t0;
    p->num_threads = 1;
    
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
    
    // Check if there are any other ready threads
    if (!p->ready_queue) {
        TRACE_THREAD("No other threads ready, not yielding");
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
    
    // Remove from ready queue if present (shouldn't be necessary for current thread)
    remove_from_ready_queue(current);
    
    if (current->state == THREAD_STATE_RUNNING) {
        atomic_thread_state_change(current, THREAD_STATE_READY);
        add_to_ready_queue(current);
    }
    
    // Find next thread to run
    struct thread *next = p->ready_queue;
    if (!next) {
        // No other threads to run, just return
        TRACE_THREAD("No other threads to run after yield");
        atomic_thread_state_change(current, THREAD_STATE_RUNNING);
        spl(sr);
        return 0;
    }
    
    // If next is the same as current, don't switch
    if (next == current) {
        TRACE_THREAD("Next thread is current, not switching");
        remove_from_ready_queue(next);
        atomic_thread_state_change(current, THREAD_STATE_RUNNING);
        spl(sr);
        return 0;
    }
    
    // Verify next thread's stack integrity
    if (next->stack_magic != STACK_MAGIC) {
        TRACE_THREAD("ERROR: Next thread stack corruption detected!");
        // Try to find another valid thread
        remove_from_ready_queue(next);
        next = p->ready_queue;
        if (!next || next == current || next->stack_magic != STACK_MAGIC) {
            // No valid threads to switch to
            atomic_thread_state_change(current, THREAD_STATE_RUNNING);
            spl(sr);
            return -EFAULT;
        }
    }
    
    // Remove next thread from ready queue
    remove_from_ready_queue(next);
    
    // Update thread states
    atomic_thread_state_change(next, THREAD_STATE_RUNNING);
    p->current_thread = next;
    
    TRACE_THREAD("IN YIELD: Yielding from thread %d to thread %d", current->tid, next->tid);
    CONTEXT *next_ctx = get_thread_context(next);
    set_thread_usp(next_ctx);
    
    // Save current context and switch to next thread
    if (save_context(&current->ctxt[CURRENT]) == 0) {
        // This is the initial save_context call
        change_context(next_ctx);
        // We should never reach here
        TRACE_THREAD("ERROR: Returned from change_context!");
    } else {
        // This is the return from save_context after another thread has yielded to us
        TRACE_THREAD("Thread %d resumed after yield", current->tid);
    }
    
    spl(sr);
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

static void thread_idle_loop(struct thread *t)
{
    if (!t) return;

    unsigned short sr = splhigh();
    // Ajoute à la sleep queue si pas déjà dedans
    add_to_sleep_queue(t);
    spl(sr);

    // TRACE_THREAD("Thread %d entering idle loop, will wake up at %lu", t->tid, t->wakeup_time);
    
    // Boucle d'attente jusqu'au temps de réveil
    while (*((volatile unsigned long *)_hz_200) < t->wakeup_time) {
        asm volatile("nop");  // Donne la main aux autres threads
    }

    sr = splhigh();

    remove_from_sleep_queue(t);
    atomic_thread_state_change(t, THREAD_STATE_READY);
    add_to_ready_queue(t);
    
    spl(sr);
    // TRACE_THREAD("Thread %d waking up from idle loop", t->tid);
    return;
}

static void init_thread_idle_context(struct thread *t)
{
    if (!t) return;

    // Clear contexts
    memset(&t->idle_ctx[CURRENT], 0, sizeof(struct context));
    memset(&t->idle_ctx[SYSCALL], 0, sizeof(struct context));

    // Allouer une stack dédiée
    if (!t->idle_stack) {
        t->idle_stack = kmalloc(STKSIZE);
        TRACE_THREAD("KMALLOC: Allocated idle stack at %p for thread %d", t->idle_stack, t->tid);
    }
    if (!t->idle_stack) {
        TRACE_THREAD("Failed to allocate idle stack");
        return;
    }
    t->idle_stack_top = (char*)t->idle_stack + STKSIZE;

    // Utiliser la stack dédiée
    unsigned long ssp = ((unsigned long)t->idle_stack_top - 128) & ~3L;
    unsigned long usp = ((unsigned long)t->idle_stack_top - 256) & ~3L;
    
    // Set up initial context - utiliser idle_start au lieu de thread_idle_loop
    t->idle_ctx[CURRENT].ssp = ssp;
    t->idle_ctx[CURRENT].usp = usp;
    t->idle_ctx[CURRENT].pc = (unsigned long)idle_start;  // Pointer vers la fonction trampoline
    t->idle_ctx[CURRENT].sr = 0x2000;
    
    // Create exception frame pour RTE - utiliser idle_start
    unsigned short *frame_ptr = (unsigned short *)(ssp - 8);
    frame_ptr[0] = 0x0000;          // Format/Vector
    frame_ptr[1] = 0x2000;          // SR
    frame_ptr[2] = (unsigned short)((unsigned long)idle_start >> 16);   // PC high
    frame_ptr[3] = (unsigned short)((unsigned long)idle_start);         // PC low
    
    // Update SSP to point to frame
    t->idle_ctx[CURRENT].ssp = (unsigned long)frame_ptr;
    
    // Set thread pointer in D0 pour passage à idle_start
    t->idle_ctx[CURRENT].regs[0] = (unsigned long)t;
    
    // Copy to SYSCALL context
    memcpy(&t->idle_ctx[SYSCALL], &t->idle_ctx[CURRENT], sizeof(struct context));
    
    TRACE_THREAD("Idle context initialized for thread %d:", t->tid);
    TRACE_THREAD("  SSP=%08lx, USP=%08lx", 
                 t->idle_ctx[CURRENT].ssp, 
                 t->idle_ctx[CURRENT].usp);
}

long _cdecl sys_p_sleepthread(long ms)
{
    struct proc *p = curproc;
    struct thread *t = p ? p->current_thread : NULL;
    unsigned short sr;

    if (!p || !t) return -EINVAL;
    if (t->tid == 0) return -EINVAL;
    if (ms <= 0) return 0;

    sr = splhigh();
    // Forcer l'utilisation du contexte idle
    CONTEXT *idle_ctx = &t->idle_ctx[CURRENT];
    if (save_context(&t->ctxt[CURRENT]) == 0) {
        // Configure le temps de réveil
        if(!t->wakeup_time){
            t->wakeup_time = *((volatile unsigned long *)_hz_200) + (ms / 5);
        } else {
            if (*((volatile unsigned long *)_hz_200) >= t->wakeup_time) {
                // Déjà l'heure du réveil, ne pas aller en idle
                TRACE_THREAD("Thread %d wake time already passed", t->tid);
                t->wakeup_time = 0;  // Reset wakeup time
                spl(sr);
                schedule();  // Re-schedule immediately
                return 0;
            }
        }

        // Change l'état du thread en SLEEPING et retire de la ready queue
        atomic_thread_state_change(t, THREAD_STATE_SLEEPING);
        remove_from_ready_queue(t);

        TRACE_THREAD("Sleeping for thread %d, will wake up in %ld ms", t->tid, ms);
        // Initialise le contexte idle si pas déjà fait
        init_thread_idle_context(t);


        TRACE_THREAD("IN sys_p_sleepthread: Switching to idle context for thread %d", t->tid);
        set_thread_usp(idle_ctx);
        spl(sr);  // Restaure SR avant le change_context
        change_context(idle_ctx);
    } else {
        // Si on revient ici, c'est qu'on a été réveillé
        TRACE_THREAD("Thread %d woke up from sleep", t->tid);
        t->wakeup_time = 0;  // Reset wakeup time
        spl(sr);
        schedule();  // Re-schedule immediately
        return 0;
    }

    // On ne devrait JAMAIS arriver ici
    TRACE_THREAD("ERROR: Returned from idle context switch!");
    spl(sr);
    return -1;
}

void add_to_sleep_queue(struct thread *t)
{
    struct proc *p = t->proc;
    unsigned short sr;

    if (!t || !p) return;

    sr = splhigh();

    // Ajouter vérification de validité du wakeup_time
    if (t->wakeup_time == 0) {
        TRACE_THREAD("ERROR: Invalid wakeup time for thread %d", t->tid);
        spl(sr);
        return;
    }

    // Vérifie si déjà dans la sleep queue
    struct thread *curr = p->sleep_queue;
    while (curr) {
        if (curr == t) {
            TRACE_THREAD("Thread %d already in sleep queue", t->tid);
            spl(sr);
            return;
        }
        curr = curr->next_sleeping;
    }

    // Ajoute à la sleep queue, triée par wakeup_time
    struct thread **pp = &p->sleep_queue;
    while (*pp && (*pp)->wakeup_time <= t->wakeup_time) {
        pp = &(*pp)->next_sleeping;
    }
    t->next_sleeping = *pp;
    *pp = t;

    TRACE_THREAD("ADD_TO_SLEEP_Q: Thread %d (wakeup at %lu)", t->tid, t->wakeup_time);
    spl(sr);
}

void remove_from_sleep_queue(struct thread *t)
{
    struct proc *p = t->proc;
    unsigned short sr;

    if (!t || !p) return;

    sr = splhigh();

    struct thread **pp = &p->sleep_queue;
    while (*pp) {
        if (*pp == t) {
            *pp = t->next_sleeping;
            t->next_sleeping = NULL;
            TRACE_THREAD("REMOVE_FROM_SLEEP_Q: Thread %d", t->tid);
            break;
        }
        pp = &(*pp)->next_sleeping;
    }

    spl(sr);
}

static CONTEXT* get_thread_context(struct thread *t)
{
    if (!t) return NULL;

    // Pour thread0, utiliser le contexte process
    if (t->tid == 0) {
        if (!t->proc || t->proc->magic != CTXT_MAGIC) {
            TRACE_THREAD("ERROR: Invalid process magic for thread0");
            return NULL;
        }
        TRACE_THREAD("Using process context for thread0");
        return &t->proc->ctxt[CURRENT];
    }

    // Pour les autres threads, vérifier leur magic
    if (t->magic != CTXT_MAGIC) {
        TRACE_THREAD("ERROR: Invalid thread magic in get_thread_context");
        return NULL;
    }

    // Pour les threads SLEEPING, utiliser le contexte idle
    if (t->state == THREAD_STATE_SLEEPING) {
        TRACE_THREAD("Using idle context for sleeping thread %d", t->tid);
        return &t->idle_ctx[CURRENT];
    }

    TRACE_THREAD("Using current context for thread %d", t->tid);
    return &t->ctxt[CURRENT];
}

static void set_thread_usp(CONTEXT *ctx) {
    if (!ctx) return;
    
    unsigned long aligned_usp = ctx->usp & ~3L;
    asm volatile (
        "move.l %0,%%a0\n\t"
        "move.l %%a0,%%usp"
        : 
        : "r" (aligned_usp)
        : "a0"
    );
}

// Fonction trampoline pour le contexte idle
static void idle_start(void) {
    struct proc *p = curproc;
    if (!p || !p->current_thread) {
        TRACE_THREAD("idle_start: No current thread!");
        return;
    }
    
    struct thread *t = p->current_thread;
    TRACE_THREAD("Idle trampoline started for thread %d", t->tid);
    
    // Appeler la boucle idle avec le thread comme argument
    thread_idle_loop(t);

    CONTEXT *orig_ctx = &t->ctxt[CURRENT];
    if (!orig_ctx) {
        TRACE_THREAD("ERROR: Cannot get original context!");
        return;
    }

    // Restaure directement le contexte original
    set_thread_usp(orig_ctx);
    change_context(orig_ctx);
}

/* End of Threads stuff */