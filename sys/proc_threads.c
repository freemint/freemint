
# include "libkern/libkern.h" /* memset and memcpy */

#include "proc.h"
#include "mint/signal.h"
#include "timeout.h"
#include "arch/context.h"
#include "mint/arch/asm_spl.h"
#include "kmemory.h"
#include "kentry.h"

/* Threads stuff */

#define WITH_THTIMER 1

void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);
void thread_switch(struct thread *from, struct thread *to);
long create_thread(struct proc *p, void (*func)(void*), void *arg);
void init_thread_context(struct thread *t, void (*func)(void*), void *arg);
void thread_start(void);
void schedule(void);
void thread_exit(void);
void init_thread0(struct proc *p);

void thread_preempt_handler(PROC *p, long arg);
void thread_timer_start(struct proc *p, int thread_id);
void thread_timer_stop(PROC *p);
void start_thread_timing(struct thread *t);

void thread_switch_timeout_handler(PROC *p, long arg);
void reset_thread_switch_state(void);
int check_thread_switch_timeout(void);

void atomic_thread_state_change(struct thread *t, int new_state);

/* Thread signal handling function prototypes */
int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig);
int thread_aware_raise(struct proc *p, int sig);
int check_thread_signals(struct thread *t);
void handle_thread_signal(struct thread *t, int sig);
void save_thread_context(struct thread *t, struct thread_sigcontext *ctx);
void restore_thread_context(struct thread *t, struct thread_sigcontext *ctx);
void thread_signal_trampoline(int sig, void *arg);

void thread_timeout_handler(PROC *p, long arg);

void delay5(long time);

/* Thread signal handler table */
struct thread_signal_handler {
	void (*handler)(int, void*);
	void *arg;
};

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
    if (t && t->state == THREAD_BLOCKED) {
        t->sleep_reason = 1; // Timeout

        atomic_thread_state_change(t, THREAD_READY);
        add_to_ready_queue(t);
        TRACE_THREAD("thread_timeout_handler: waking up thread %d", t->tid);
    }
    spl(sr);
}

/*
 * Save thread execution context before signal handling
 */
void save_thread_context(struct thread *t, struct thread_sigcontext *ctx)
{
    if (!t || !ctx)
        return;
        
    // Copier d0-d7 (8), a0-a6 (7) → total 15 regs
    for (int i = 0; i < 15; i++)
        ctx->sc_regs[i] = t->ctxt[CURRENT].regs[i];

    // a7 (user stack pointer) = usp
    ctx->sc_regs[15] = t->ctxt[CURRENT].usp;

    ctx->sc_usp = t->ctxt[CURRENT].usp;
    ctx->sc_pc  = t->ctxt[CURRENT].pc;
    ctx->sc_sr  = t->ctxt[CURRENT].sr;

    ctx->sc_thread = t;
    // sc_sig et sc_handler_arg sont à remplir dans le trampoline
}

/*
 * Restore thread execution context after signal handling
 */
void restore_thread_context(struct thread *t, struct thread_sigcontext *ctx)
{
    if (!t || !ctx || ctx->sc_thread != t)
        return;
        
    // Restaurer d0-d7, a0-a6
    for (int i = 0; i < 15; i++)
        t->ctxt[CURRENT].regs[i] = ctx->sc_regs[i];

    t->ctxt[CURRENT].usp = ctx->sc_usp;
    t->ctxt[CURRENT].pc  = ctx->sc_pc;
    t->ctxt[CURRENT].sr  = ctx->sc_sr;
}

/*
 * Signal trampoline function to call thread signal handlers
 */
void thread_signal_trampoline(int sig, void *arg)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    struct thread_sigcontext *ctx;
    
    if (!t || !p || !p->p_sigacts || sig <= 0 || sig >= NSIG)
        return;
        
    /* Get handler */
    void (*handler)(int, void*) = p->p_sigacts->thread_handlers[sig].handler;
    void *handler_arg = p->p_sigacts->thread_handlers[sig].arg;
    
    if (!handler)
        return;
    
    /* Save context */
    ctx = (struct thread_sigcontext *)kmalloc(sizeof(struct thread_sigcontext));
    if (!ctx)
        return;
        
    save_thread_context(t, ctx);
    
    /* Store context and signal info */
    ctx->sc_sig = sig;
    ctx->sc_handler_arg = handler_arg;
    t->t_sigctx = ctx;
    
    /* Call handler */
    (*handler)(sig, handler_arg);
    
    /* Restore context and free it */
    restore_thread_context(t, ctx);
    t->t_sigctx = NULL;
    kfree(ctx);
    
    /* Clear signal in progress flag */
    t->t_sig_in_progress = 0;
}

/*
 * Process thread signal
 */
void handle_thread_signal(struct thread *t, int sig)
{
    struct proc *p;
    
    if (!t || !t->proc || sig <= 0 || sig >= NSIG)
        return;
        
    p = t->proc;
    
    /* Check if there's a thread-specific handler */
    if (p->p_sigacts && p->p_sigacts->thread_handlers[sig].handler) {
        /* Execute handler in thread context */
        thread_signal_trampoline(sig, p->p_sigacts->thread_handlers[sig].arg);
    } else {
        /* Fall back to process-level signal handling */
        /* This will use the standard signal dispatcher */
        p->sigpending |= (1UL << sig);
    }
}

/*
 * Deliver a signal to a specific thread
 * Returns 1 if signal was delivered, 0 otherwise
 */
int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig)
{
    if (!p || !t || sig <= 0 || sig >= NSIG)
        return 0;
        
    /* Check if this signal should be handled at thread level */
    if (!p->p_sigacts->thread_signals && !IS_THREAD_SIGNAL(sig))
        return 0;  /* Process-level signal, don't handle here */
        
    /* Check if signal is blocked by thread */
    if (t->t_sigmask & (1UL << sig))
        return 0;
        
    /* Mark signal as pending for this thread */
    t->t_sigpending |= (1UL << sig);
    
    /* If thread is blocked, wake it up */
    if (t->state == THREAD_BLOCKED) {
        /* Wake up the thread */
        t->state = THREAD_READY;
        add_to_ready_queue(t);
    }
    
    return 1;
}

/*
 * Modified version of raise() that's thread-aware
 */
int thread_aware_raise(struct proc *p, int sig)
{
    if (!p || sig <= 0 || sig >= NSIG)
        return -EINVAL;
        
    /* Check if this process has thread-specific signal handling enabled */
    if (p->p_sigacts && p->p_sigacts->thread_signals) {
        /* For certain signals, deliver to specific thread */
        if (IS_THREAD_SIGNAL(sig)) {
            /* For SIGUSR1/SIGUSR2, try to deliver to current thread first */
            if (p->current_thread && 
                deliver_signal_to_thread(p, p->current_thread, sig))
                return 0;
                
            /* If that fails, try all threads */
            struct thread *t;
            for (t = p->threads; t != NULL; t = t->next) {
                if (deliver_signal_to_thread(p, t, sig))
                    return 0;
            }
        }
    }
    
    /* Default: deliver to process as before */
    p->sigpending |= (1UL << sig);
    return 0;
}

/*
 * Check for pending signals in a thread
 * Returns signal number if a signal is pending, 0 otherwise
 */
int check_thread_signals(struct thread *t)
{
    int sig;
    
    if (!t || !t->proc)
        return 0;
        
    /* Don't process signals if we're already handling one */
    if (t->t_sig_in_progress)
        return 0;
        
    /* Check for pending signals that aren't masked */
    ulong pending = t->t_sigpending & ~t->t_sigmask;
    
    if (!pending)
        return 0;
        
    /* Find the first pending signal */
    for (sig = 1; sig < NSIG; sig++) {
        if (pending & (1UL << sig)) {
            /* Clear pending flag */
            t->t_sigpending &= ~(1UL << sig);
            
            /* Mark that we're processing a signal */
            t->t_sig_in_progress = sig;
            
            return sig;
        }
    }
    
    return 0;
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
    if (t->state == THREAD_EXITED && new_state != THREAD_EXITED) {
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
    
	p->timer_operation_in_progress = 0;
    DEBUG(("Thread timer initialized"));
}

/*
 * Start timing a specific process/thread
 */
void thread_timer_start(struct proc *p, int thread_id)
{
    unsigned short sr, retry_count = 0;
    TRACE_THREAD("thread_timer_start called for process %d", p->pid);
    if (!p)
        return;

retry_start:		
    sr = splhigh();

    /* Check if another timer operation is in progress */
    if (p->timer_operation_in_progress) {
        TRACE_THREAD("WARNING: Timer operation in progress, retrying...");
        spl(sr);
        
        /* Simple retry mechanism with limit */
        if (++retry_count > 5) {
            TRACE_THREAD("WARNING: Failed to start timer after 5 retries");
            return;
        }
        
        /* Small delay before retry */
        {
            volatile int i;
            for (i = 0; i < 100; i++);
        }
        
        goto retry_start;
    }
    TRACE_THREAD("Starting thread timer for process %d", p->pid);
    p->timer_operation_in_progress = 1;
    	
    /* If timer is already enabled, don't add another timeout */
    if (p->p_thread_timer.enabled && p->p_thread_timer.timeout) {
        TRACE_THREAD("Timer already enabled, not adding another timeout");
        p->timer_operation_in_progress = 0;
        spl(sr);
        return;
    }
    
    p->p_thread_timer.thread_id = thread_id;
    p->p_thread_timer.sr = sr;
    
    /* Add the initial timeout */
    p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
    
    TRACE_THREAD("Setting up preemption timer for thread %d", thread_id);

    /* Set enabled flag last to ensure everything is set up */
    TRACE_THREAD("Thread timer started for process %d", p->pid);
    p->p_thread_timer.enabled = 1;
    TRACE_THREAD("thread_timer: enabled set to 1 by %s:%d", __FILE__, __LINE__);
    p->p_thread_timer.in_handler = 0;
    spl(sr);
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

retry_stop:
    sr = splhigh();
    
    /* Check if another timer operation is in progress */
    if (p->timer_operation_in_progress) {
        spl(sr);
        
        /* Simple retry mechanism with limit */
        if (++retry_count > 5) {
            TRACE_THREAD("WARNING: Failed to stop timer after 5 retries (FORCING RESET)");
            return;
        }
        
        /* Introduce a small delay before retrying */
        // delay5(20);
        
        goto retry_stop;
    }
    
    p->timer_operation_in_progress = 1;
    
    /* Check if timer is already disabled */
    TRACE_THREAD("Stopping thread timer for process %d", p->pid);
    if (p->num_threads <= 1) {
        p->p_thread_timer.enabled = 0;
        TRACE_THREAD("thread_timer: enabled set to 0 by %s:%d", __FILE__, __LINE__);
    }
    
    /* Save the timeout pointer locally before canceling */
    if (p->p_thread_timer.timeout) {
        timeout_to_cancel = p->p_thread_timer.timeout;
        p->p_thread_timer.timeout = NULL;
    }

    /* Release the lock before calling canceltimeout */
    p->timer_operation_in_progress = 0;
    spl(sr);
    
    /* Now cancel the timeout if we had one */
    if (timeout_to_cancel) {
        canceltimeout(timeout_to_cancel);
    }
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

/*
 * Thread preemption handler - called by the timeout system
 * This is the core function that will be called periodically
 */
void thread_preempt_handler(PROC *p, long arg)
{
    struct proc *current;
    unsigned short sr;
    
    // TRACE_THREAD("thread_preempt_handler called");
    if (p != curproc) {
        // TRACE_THREAD("thread_preempt_handler: called for non-current process (p->pid=%d, curproc->pid=%d), skipping", p->pid, curproc ? curproc->pid : -1);
        p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        return;
    }
    /* Prevent reentrance */
    if (p->p_thread_timer.in_handler) {
        TRACE_THREAD("(in_handler) Thread timer already in handler, not re-adding timeout");
        /* Re-add the timeout and return */
        if (!p->p_thread_timer.enabled) {
            TRACE_THREAD("(in_handler) Thread timer disabled, not re-adding timeout");
            /* Timer was disabled while we were waiting to enter handler */
            return;
        }
        TRACE_THREAD("(p->p_thread_timer.in_handler == 1) Re-adding timeout for thread %d in process %d", 
                     p->p_thread_timer.thread_id, p->pid);
        p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        return;
    }
    
    TRACE_THREAD("(in_handler) Entering thread preemption handler for process %d", p->pid);
    p->p_thread_timer.in_handler = 1;
    
    /* Clear the timeout pointer since this timeout has fired */
    p->p_thread_timer.timeout = NULL;
    
    /* If timer is not enabled, just return without re-adding the timeout */
	sr = splhigh();
    if (!p->p_thread_timer.enabled) {
        TRACE_THREAD("(thread_preempt_handler) Thread timer disabled, not re-adding timeout");
        p->p_thread_timer.in_handler = 0;
        return;
    }
    
    /* Get current process - IMPORTANT: Don't use the passed-in process pointer */
    current = curproc;
    if (!current) {
        /* No current process, re-add the timeout and return */
        TRACE_THREAD("(thread_preempt_handler) No current process, re-adding timeout");
		spl(sr);
        p->p_thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        p->p_thread_timer.in_handler = 0;
        return;
    }
    
    /* Only preempt if process has multiple threads */
    if (current->num_threads > 1 && current->current_thread) {
        TRACE_THREAD("(thread_preempt_handler) Preempting thread %d in process %d", 
                     current->current_thread->tid, current->pid);
        /* Check if there are threads in the ready queue */
		spl(sr);
        if (!p->ready_queue) {
            TRACE_THREAD("No threads in ready queue, re-adding timeout");
            p->p_thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            p->p_thread_timer.in_handler = 0;
            return;
        }
        
        /* Don't preempt if a thread switch is already in progress */
        if (thread_switch_in_progress) {
            TRACE_THREAD("(thread_preempt_handler) Thread switch in progress, re-adding timeout");
            p->p_thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            p->p_thread_timer.in_handler = 0;
            return;
        }

        /* Get the current thread */
        struct thread *curr_thread = current->current_thread;
        
        TRACE_THREAD("(thread_preempt_handler) Current thread: %d (state %d)", curr_thread->tid, curr_thread->state);
        /* Find next thread to run, skip EXITED threads */
        struct thread *next = current->ready_queue;
        while (next && next->state == THREAD_EXITED) {
            remove_from_ready_queue(next);
            next = current->ready_queue;
        }

        /* Si aucun thread valide, on ne fait rien */
        if (!next || next == curr_thread) {
            TRACE_THREAD("(thread_preempt_handler) No other valid threads to run, re-adding timeout");
            spl(sr);
            p->p_thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            p->p_thread_timer.in_handler = 0;
            return;
        }
        
        /* Remove next thread from ready queue */
        remove_from_ready_queue(next);
        
        /* Add current thread to ready queue uniquement si il est RUNNING et pas EXITED/thread0 */
        if (curr_thread->state == THREAD_RUNNING && curr_thread->tid != 0) {
            TRACE_THREAD("(thread_preempt_handler) Adding current thread %d back to ready queue", curr_thread->tid);
            atomic_thread_state_change(curr_thread, THREAD_READY);
            add_to_ready_queue(curr_thread);
        }
        if (next->state == THREAD_EXITED) {
            TRACE_THREAD("ERROR: Attempt to switch to EXITED thread %d, skipping", next->tid);
            p->p_thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            p->p_thread_timer.in_handler = 0;
            spl(sr);
            return;
        }
        /* Update thread states */
        atomic_thread_state_change(next, THREAD_RUNNING);
        current->current_thread = next;
        TRACE_THREAD("(thread_preempt_handler) Switching to thread %d (state %d)", next->tid, next->state);
        /* Perform context switch */
        thread_switch(curr_thread, next);
        
        spl(sr);
    }
    
    /* Re-add the timeout for next preemption */
    p->p_thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
    p->p_thread_timer.in_handler = 0;
}

// Ready queue management
void add_to_ready_queue(struct thread *t) {
    struct proc *p = t->proc;
    unsigned short sr;
    if (!t || t->state == THREAD_RUNNING || t->state == THREAD_EXITED) return;
    if (t->tid == 0 && t->state == THREAD_EXITED) return;
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

    atomic_thread_state_change(t, THREAD_READY);
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

// Thread context switching
void thread_switch(struct thread *from, struct thread *to) {
    if (!to || from == to || to->state == THREAD_EXITED) {
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
    
    // Save current USP to the current thread's context
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
    
    // Save current context
    if (save_context(&from->ctxt[CURRENT]) == 0) {
        // This is the initial save_context call
        
        // Update thread states - ensure proper state transition
        atomic_thread_state_change(from, THREAD_READY);
        atomic_thread_state_change(to, THREAD_RUNNING);
        
        // Update current thread
        from->proc->current_thread = to;
        
        // Special handling for thread0 - use process USP if needed
        unsigned long usp_to_set;
        if (to->tid == 0) {
            // For thread0, use the process's USP if available
            usp_to_set = to->proc->ctxt[CURRENT].usp ? 
                         to->proc->ctxt[CURRENT].usp : to->ctxt[CURRENT].usp;
        } else {
            // For other threads, use their own USP
            usp_to_set = to->ctxt[CURRENT].usp;
        }
        
        // Set USP for the new thread - IMPORTANT: Do this BEFORE change_context
        TRACE_THREAD("Setting USP to %lx", usp_to_set);
        
        // Use proper assembly to set USP - ensure it's properly aligned
        unsigned long aligned_usp = usp_to_set & ~3L;
        asm volatile (
            "move.l %0,%%a0\n\t"
            "move.l %%a0,%%usp"
            : 
            : "r" (aligned_usp)
            : "a0"
        );
        
        // Clear the in-progress flag before context switch
		// Don't reset state here - will be reset when we return from context switch
        
        // Perform context switch
        change_context(&to->ctxt[CURRENT]);
        
        // We should never reach here
        TRACE_THREAD("ERROR: Returned from change_context!");
    } else {
        // This is the return from context switch
        TRACE_THREAD("Returned to thread %d after context switch", from->tid);
        // Reset thread switch state
        reset_thread_switch_state();
        spl(sr);
    }
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
    if (!t) return -ENOMEM;
    
    TRACE_THREAD("Creating thread: pid=%d, func=%p, arg=%p", p->pid, func, arg);
    
    // Basic initialization
    memset(t, 0, sizeof(struct thread));
    t->tid = p->num_threads++;
    t->proc = p;
    t->priority = p->pri;
    atomic_thread_state_change(t, THREAD_READY);

    /* Initialize signal fields */
    t->t_sigpending = 0;
    t->t_sigmask = p->p_sigmask;  /* Inherit process signal mask */
    t->t_sig_in_progress = 0;
    t->t_sigctx = NULL;

    // Allocate stack
    t->stack = kmalloc(STKSIZE);
    if (!t->stack) {
		p->num_threads--;  // Revert the thread count increment
        kfree(t);
        TRACE_THREAD("Stack allocation failed");
        spl(sr);
        return -ENOMEM;
    }
    
    t->stack_top = (char*)t->stack + STKSIZE;
    t->stack_magic = STACK_MAGIC;
    
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
    if (p->num_threads == 1) {
        atomic_thread_state_change(t, THREAD_RUNNING);
    } else {
        // Add to ready queue
        add_to_ready_queue(t);
        
		#ifdef WITH_THTIMER
        // Start thread timing if this is the second thread
        if (p->num_threads == 2) {
            TRACE_THREAD("(create_thread) Starting thread timer for process %d", p->pid);
            start_thread_timing(t);
        }
		#endif
    }

    // Set as current thread AFTER state is set
    if (p->num_threads == 1) {
        TRACE_THREAD("Setting thread %d as current thread for process %d", t->tid, p->pid);
        p->current_thread = t;
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

// Thread scheduling
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

    while (next && next->state != THREAD_READY) {
        remove_from_ready_queue(next);
        next = p->ready_queue;
    }

    TRACE_THREAD("schedule: next=%d", next ? next->tid : -1);

    /* Check if there's a stuck thread switch and recover if needed */
    if (check_thread_switch_timeout()) {
        TRACE_THREAD("Detected stuck thread switch during schedule, resetting state");
        reset_thread_switch_state();
        /* Continue with scheduling */
    }
				
    if (!next) {
        TRACE_THREAD("No ready threads");
        spl(sr);
        return;
    }
    
    // If there's no current thread, just switch to the next one
    if (!current) {
        TRACE_THREAD("No current thread, switching to thread %d", next->tid);
        
        // Remove next thread from ready queue
        remove_from_ready_queue(next);
        
        // Update thread states
        atomic_thread_state_change(next, THREAD_RUNNING);
        p->current_thread = next;
        
        // Set USP for the new thread
        TRACE_THREAD("Setting USP to %lx", next->ctxt[CURRENT].usp);
        asm volatile (
            "move.l %0,%%a0\n\t"
            "move.l %%a0,%%usp"
            : 
            : "r" (next->ctxt[CURRENT].usp)
            : "a0"
        );
        
        // Perform context switch - this should never return
        change_context(&next->ctxt[CURRENT]);
        
        // We should never reach here
        TRACE_THREAD("ERROR: Returned from change_context!");
        spl(sr);
        return;
    }

    // Check for pending signals in the current thread
    if (current && current->proc && current->proc->p_sigacts && 
        current->proc->p_sigacts->thread_signals) {
        int sig = check_thread_signals(current);
        if (sig) {
            /* Handle the signal before switching */
            handle_thread_signal(current, sig);
        }
    }
    
    // IMPORTANT: Always switch to a different thread if possible
    if (next == current && next->next_ready) {
        next = next->next_ready;
        TRACE_THREAD("Skipping current thread, using next=%d", next->tid);
    }
    
    // If next is still current, don't switch
    if (next == current) {
        TRACE_THREAD("Only current thread is ready, skipping schedule");
        spl(sr);
        return;
    }
    
    TRACE_THREAD("Switching threads: %d -> %d", current->tid, next->tid);
    
    // Remove next thread from ready queue
    remove_from_ready_queue(next);
    
    // Add current thread to ready queue if it's still runnable
    if (current->state == THREAD_RUNNING) {
        // Set state to READY before adding to queue
        atomic_thread_state_change(current, THREAD_READY);
        add_to_ready_queue(current);
        TRACE_THREAD("Added current thread %d to ready queue", current->tid);
    }
    
    // Update thread states
    atomic_thread_state_change(next, THREAD_RUNNING);
    p->current_thread = next;
    
    // Switch to next thread
    thread_switch(current, next);
    
    // Interrupts are restored by thread_switch
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
        volatile int i; for (i = 0; i < 1000; i++);
        thread_exit();
        return;
    }
    thread_exit_in_progress = 1;
    exit_owner_tid = current->tid;

    int tid = current->tid;

    // Marquer EXITED et retirer de la ready queue
    atomic_thread_state_change(current, THREAD_EXITED);
    remove_from_ready_queue(current);

    // Annuler les timers éventuels
    if (current->tid == p->p_thread_timer.thread_id && p->p_thread_timer.enabled) {
        TRACE_THREAD("Stopping thread timer for thread %d", tid);
        thread_timer_stop(p);
        // Si le process a encore plusieurs threads, réarmer le timer sur un autre thread
        if (p->num_threads > 1) {
            struct thread *t = p->threads;
            // Cherche un thread vivant différent du courant
            while (t && (t == current || t->state == THREAD_EXITED)) t = t->next;
            if (t) {
                TRACE_THREAD("Rearming thread timer for thread %d", t->tid);
                thread_timer_start(p, t->tid);
            }
        }
    }
    // Retirer de la liste des threads du process
    struct thread **tp;
    for (tp = &p->threads; *tp; tp = &(*tp)->next) {
        if (*tp == current) {
            *tp = current->next;
            break;
        }
    }
    p->num_threads--;

    // Libérer la stack si ce n'est pas thread0
    if (current->stack && tid != 0) {
        TRACE_THREAD("Freeing stack for thread %d", tid);
        kfree(current->stack);
        current->stack = NULL;
    }

    // Libérer la structure thread
    TRACE_THREAD("Freeing thread %d structure", tid);
    current->magic = 0;
    kfree(current);

    if (tid == 0) {
        // Attendre la fin de tous les threads enfants
        while (p->num_threads > 1) {
            // Vérifie que tous les threads restants sont bien EXITED
            struct thread *t = p->threads;
            int all_exited = 1;
            while (t) {
                if (t->tid != 0 && t->state != THREAD_EXITED) {
                    all_exited = 0;
                    break;
                }
                t = t->next;
            }
            if (all_exited)
                break;

            spl(sr);
            TRACE_THREAD("Thread0 waiting for %d threads to finish", p->num_threads - 1);
            sys_p_sleepthread(40);
            sr = splhigh();
        }
        TRACE_THREAD("Thread0 exiting, returning to process");
        if (p->p_thread_timer.enabled) {
            TRACE_THREAD("Cleaning up thread timer for process %d", p->pid);
            thread_timer_stop(p);
        }        
        p->current_thread = NULL;
        thread_exit_in_progress = 0;
        exit_owner_tid = -1;
        spl(sr);
        return;
    }

    struct thread *next = p->ready_queue;
    if (next) {
        remove_from_ready_queue(next);
        atomic_thread_state_change(next, THREAD_RUNNING);
        p->current_thread = next;
        TRACE_THREAD("Switching to thread %d", next->tid);

        thread_exit_in_progress = 0;
        exit_owner_tid = -1;
        spl(sr);

        if (save_context(&current->ctxt[CURRENT]) == 0) {
            change_context(&next->ctxt[CURRENT]);
            // Ne jamais revenir ici
            TRACE_THREAD("ERROR: Returned from change_context!");
        }
        return;
    }

    // Sinon, retourner au contexte process
    TRACE_THREAD("No other threads to run, returning to main process");
    p->current_thread = NULL;
    thread_exit_in_progress = 0;
    exit_owner_tid = -1;
    spl(sr);

    if (save_context(&current->ctxt[CURRENT]) == 0) {
        change_context(&p->ctxt[CURRENT]);
        TRACE_THREAD("ERROR: Returned from change_context to main process!");
    }
}

void init_thread0(struct proc *p) {
    if (p->current_thread) return; // Already initialized
    
    struct thread *t0 = kmalloc(sizeof(struct thread));
    if (!t0) return;
    
    TRACE_THREAD("Initializing thread0 for process %d", p->pid);
    
    memset(t0, 0, sizeof(struct thread));
    t0->tid = 0;
    t0->proc = p;
    t0->priority = p->pri;
    atomic_thread_state_change(t0, THREAD_RUNNING);
    
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
    
    // Set state to READY before adding to queue
    atomic_thread_state_change(current, THREAD_READY);
    
    // Add to end of ready queue
    add_to_ready_queue(current);
    
    // Find next thread to run
    struct thread *next = p->ready_queue;
    if (!next) {
        // No other threads to run, just return
        TRACE_THREAD("No other threads to run after yield");
        atomic_thread_state_change(current, THREAD_RUNNING);
        spl(sr);
        return 0;
    }
    
    // If next is the same as current, don't switch
    if (next == current) {
        TRACE_THREAD("Next thread is current, not switching");
        remove_from_ready_queue(next);
        atomic_thread_state_change(current, THREAD_RUNNING);
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
            atomic_thread_state_change(current, THREAD_RUNNING);
            spl(sr);
            return -EFAULT;
        }
    }
    
    // Remove next thread from ready queue
    remove_from_ready_queue(next);
    
    // Update thread states
    atomic_thread_state_change(next, THREAD_RUNNING);
    p->current_thread = next;
    
    // Set USP for the new thread
    TRACE_THREAD("Setting USP to %lx", next->ctxt[CURRENT].usp);
    asm volatile (
        "move.l %0,%%a0\n\t"
        "move.l %%a0,%%usp"
        : 
        : "r" (next->ctxt[CURRENT].usp)
        : "a0"
    );
    
    // Save current context and switch to next thread
    if (save_context(&current->ctxt[CURRENT]) == 0) {
        // This is the initial save_context call
        
        // Perform context switch - this should never return
        change_context(&next->ctxt[CURRENT]);
        
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

long _cdecl sys_p_sleepthread(long ms)
{
    TRACE_THREAD("sys_p_sleepthread(%ld)", ms);
    struct proc *p = curproc;
    struct thread *t = p ? p->current_thread : NULL;
    unsigned short sr;
    TIMEOUT *timeout = NULL;

    if (!p || !t)
        return 1; // Timeout par défaut

    sr = splhigh();
    t->sleep_reason = 0; // 0 = réveil normal (signal/autre)

    TRACE_THREAD("sys_p_sleepthread: thread %d going to sleep", t->tid);
    atomic_thread_state_change(t, THREAD_BLOCKED);
    remove_from_ready_queue(t);

    // Programmer le timeout si demandé
    if (ms > 0) {
        int ticks = ms / 20;
        if (ticks < 1) ticks = 1;
        TRACE_THREAD("sys_p_sleepthread: setting timeout for %ld ms (%d ticks)", ms, ticks);
        timeout = addtimeout(p, ticks, thread_timeout_handler);
        timeout->arg = (long)t;
    }

    spl(sr);

    // Laisser la main à un autre thread
    TRACE_THREAD("sys_p_sleepthread: yielding to scheduler");
    schedule();

    // Ici, le thread est réveillé (par signal ou timeout)
    sr = splhigh();
    if (timeout) {
        TRACE_THREAD("sys_p_sleepthread: cancelling timeout");
        canceltimeout(timeout); // Annule le timeout si réveillé autrement
    }

    // Check if the thread was woken up by a signal
    if (t->proc->p_sigacts->thread_signals) {
        int sig = check_thread_signals(t);
        if (sig) {
            // Handle the signal
            handle_thread_signal(t, sig);
            t->t_sig_in_progress = 0;
        }
    }

    spl(sr);
    
    return t->sleep_reason; // 1 = timeout, 0 = autre
}

/**
 * System call implementations for thread signal handling
 */

/*
 * Enable/disable thread-specific signal handling for a process
 */
long _cdecl sys_p_thread_signal_mode(int enable)
{
    if (!curproc || !curproc->p_sigacts)
        return -EINVAL;
        
    if (enable) {
        curproc->p_sigacts->thread_signals = 1;
        curproc->p_sigacts->flags |= SAS_THREADED;
    } else {
        curproc->p_sigacts->thread_signals = 0;
        curproc->p_sigacts->flags &= ~SAS_THREADED;
    }
        
    return 0;
}

/*
 * Set signal mask for current thread
 */
long _cdecl sys_p_thread_sigmask(ulong mask)
{
    if (!curproc || !curproc->current_thread)
        return -EINVAL;
        
    ulong old_mask = curproc->current_thread->t_sigmask;
    
    /* Don't allow masking of certain signals */
    curproc->current_thread->t_sigmask = mask & ~UNMASKABLE;
    
    return old_mask;
}

/*
 * Send a signal to a specific thread
 */
long _cdecl sys_p_kill_thread(struct thread *t, int sig)
{
    struct proc *p;
    
    /* Validate parameters */
    if (!t || sig <= 0 || sig >= NSIG)
        return -EINVAL;
        
    p = t->proc;
    if (!p || !p->p_sigacts)
        return -EINVAL;
        
    /* Check if thread belongs to current process */
    if (p != curproc)
        return -EPERM;
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals)
        return -EINVAL;
        
    /* Deliver signal to thread */
    if (deliver_signal_to_thread(p, t, sig))
        return 0;
        
    return -EINVAL;
}

/*
 * Register a thread-specific signal handler
 */
long _cdecl sys_p_thread_handler(int sig, void (*handler)(int, void*), void *arg)
{
    struct proc *p;
    
    /* Validate parameters */
    if (sig <= 0 || sig >= NSIG)
        return -EINVAL;
        
    p = curproc;
    if (!p || !p->p_sigacts)
        return -EINVAL;
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals)
        return -EINVAL;
        
    /* Set handler */
    p->p_sigacts->thread_handlers[sig].handler = handler;
    p->p_sigacts->thread_handlers[sig].arg = arg;
    p->p_sigacts->thread_handlers[sig].owner = CURTHREAD;
    
    return 0;
}

/*
 * Wait for signals with timeout
 */
long _cdecl sys_p_thread_sigwait(ulong mask, long timeout)
{
    struct thread *t = CURTHREAD;
    int sig;
    
    if (!t)
        return -EINVAL;
        
    /* Wait for a matching signal */
    while (timeout > 0) {
        sig = check_thread_signals(t);
        if (sig && (mask & (1UL << sig)))
            return sig;
            
        /* Sleep and decrement timeout */
        timeout -= 10;
        sys_p_sleepthread(10);
    }
    
    return 0;  /* Timeout */
}


/* End of Threads stuff */

/* Helper functions */

void delay5(long time)
{
    long i;
#ifdef __mcoldfire__
    // Version ColdFire : utiliser une boucle simple ou une autre méthode compatible
    for (i = 0; i < time * 1000; i++) {
        __asm__ __volatile__("nop");
    }
#else
    // Version 68k classique
    for (i = 0; i < time; i++) {
        __asm__ __volatile__(
            "movem.l %%a0/%%d1, -(%%sp)\n\t"
            "move.l #0x4ba.w, %%a0\n\t"
            "move.l (%%a0), %%d1\n\t"
            "1:\n\t"
            "cmp.l (%%a0), %%d1\n\t"
            "beq.s 1b\n\t"
            "movem.l (%%sp)+, %%a0/%%d1\n\t"
            ::: "memory"
        );
    }
#endif
}