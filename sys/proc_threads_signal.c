#include "proc_threads.h"

/* Sets the signal mask for the current thread, returns previous mask */
long _cdecl sys_p_thread_sigmask(ulong mask);
/* Enables or disables thread-specific signal handling for a process */
long _cdecl sys_p_thread_signal_mode(int enable);
/* Sends a signal to a specific thread within the current process */
long _cdecl sys_p_kill_thread(struct thread *t, int sig);
/* Registers a thread-specific signal handler function */
long _cdecl sys_p_thread_handler(int sig, void (*handler)(int, void*), void *arg);
/* Waits for signals with timeout, returns signal number or 0 on timeout */
long _cdecl sys_p_thread_sigwait(ulong mask, long timeout);
/* Sets the signal mask for the current thread, returns previous mask */
long _cdecl sys_p_thread_sigsetmask(ulong mask);
/* Block signals for the current thread (add to mask) */
long _cdecl sys_p_thread_sigblock(ulong mask);
/* Temporarily set signal mask and pause until a signal is received */
long _cdecl sys_p_thread_sigpause(ulong mask);
/* Thread sleep function using signals */
long _cdecl sys_p_thread_sleep(unsigned int n);
long _cdecl sys_p_thread_getid(void);
long _cdecl sys_p_thread_handler_arg(int sig, void *arg);

/* Forward declarations */
static void thread_alarm_handler(PROC *p, long arg);
static long timeout_remaining(TIMEOUT *t);

void init_thread_signals(struct proc *p);
void cleanup_thread_signals(struct thread *t);
void dispatch_thread_signals(struct thread *t);
void init_thread_signal_subsystem(void);

/*
 * Get the current thread's ID
 * Returns the thread ID or -1 on error
 */
long _cdecl sys_p_thread_getid(void)
{
    struct thread *t = CURTHREAD;
    
    if (!t){
        TRACE_THREAD("sys_p_thread_getid: no current thread");
        return -1;
    }
    TRACE_THREAD("sys_p_thread_getid: pid=%d, tid=%d", t->proc->pid, t->tid);
    return t->tid;
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

    if (t->proc != p) {
        TRACE_THREAD("ERROR: Attempt to deliver signal %d to thread %d in wrong process", 
                    sig, t->tid);
        return 0;  /* Don't crash, just fail safely */
    }

    /* Check if this signal should be handled at thread level */
    if ( !p->p_sigacts || (!p->p_sigacts->thread_signals && !IS_THREAD_SIGNAL(sig)) )
        return 0;  /* Process-level signal, don't handle here */
        
    /* Check if signal is blocked by thread */
    if (THREAD_SIGMASK(t) & (1UL << sig))
        return 0;
        
    /* Mark signal as pending for this thread */
    SET_THREAD_SIGPENDING(t, sig);
    
    /* If thread is blocked, wake it up */
    if ((t->state & THREAD_STATE_BLOCKED)) {
        /* Wake up the thread */
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
        TRACE_THREAD("Thread %d woken up by signal %d", t->tid, sig);
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
    ulong pending = THREAD_SIGPENDING(t) & ~THREAD_SIGMASK(t);
    
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
 * Helper function to get the remaining time on a timeout
 */
static long timeout_remaining(TIMEOUT *t)
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
    
    /* Convert from ticks to milliseconds (5ms per tick) */
    return remaining * 5;
}

/*
 * Thread alarm handler
 */
static void thread_alarm_handler(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    
    if (!t || !p) {
        TRACE_THREAD(("thread_alarm_handler: invalid thread or process"));
        return;
    }

    /* Verify thread belongs to this process */
    if (t->proc != p) {
        TRACE_THREAD("thread_alarm_handler: thread %d belongs to different process", 
                    t->tid);
        return;
    }

    /* Clear the thread's alarm timeout */
    t->alarm_timeout = NULL;
    
    /* Deliver SIGALRM specifically to this thread */
    deliver_signal_to_thread(p, t, PTSIG_ALARM);
}

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
        
    ulong old_mask = THREAD_SIGMASK(curproc->current_thread);
    
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
 * Set the argument for a thread-specific signal handler
 */
long _cdecl sys_p_thread_handler_arg(int sig, void *arg)
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
        
    /* Make sure a handler is already registered */
    if (!p->p_sigacts->thread_handlers[sig].handler)
        return -EINVAL;
        
    /* Set handler argument */
    p->p_sigacts->thread_handlers[sig].arg = arg;
    
    return 0;
}

/*
 * Wait for signals with timeout
 */
long _cdecl sys_p_thread_sigwait(ulong mask, long timeout)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    int sig;
    TIMEOUT *wait_timeout = NULL;
    
    if (!t || !p)
        return -EINVAL;
        
    /* Check for pending signals first */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))){
        TRACE_THREAD("sys_p_thread_sigwait: returning immediately with signal %d", sig);
        return sig;
    }
        
    /* If timeout is 0, just check and return */
    if (timeout == 0){
        TRACE_THREAD("sys_p_thread_sigwait: timeout is 0, returning immediately");
        return 0;
    }
        
    /* Set up a timeout if requested */
    if (timeout > 0) {
        /* Convert milliseconds to ticks (200Hz clock = 5ms per tick) */
        long ticks = (timeout + 4) / 5;
        if (ticks < 1) ticks = 1;
        
        wait_timeout = addtimeout(p, ticks, thread_timeout_handler);
        if (wait_timeout) {
            wait_timeout->arg = (long)t;
        }
    }
    
    /* Mark thread as sleeping */
    TRACE_THREAD("sys_p_thread_sigwait: thread %d going to sleep with mask %lx", t->tid, mask);
    unsigned short sr = splhigh();
    t->sleep_reason = 0;
    atomic_thread_state_change(t, THREAD_STATE_SLEEPING);
    remove_from_ready_queue(t);
    spl(sr);
    
    /* Schedule other threads */
    TRACE_THREAD("sys_p_thread_sigwait: scheduling other threads");
    schedule();
    
    /* When we wake up, cancel the timeout if it's still active */
    if (wait_timeout) {
        TRACE_THREAD("sys_p_thread_sigwait: cancelling timeout after wakeup");
        canceltimeout(wait_timeout);
    }
    
    /* Check if we woke up due to a signal */
    TRACE_THREAD("sys_p_thread_sigwait: checking for signals after wakeup");
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))){
        TRACE_THREAD("sys_p_thread_sigwait: returning with signal %d", sig);
        return sig;
    }
        
    return 0;  /* Timeout or no matching signal */
}

/*
 * Block signals for the current thread (add to mask)
 */
long _cdecl sys_p_thread_sigblock(ulong mask)
{
    struct thread *t = CURTHREAD;
    
    if (!t)
        return -EINVAL;
        
    ulong old_mask = THREAD_SIGMASK(t);
    
    /* Add signals to mask, but don't allow blocking unmaskable signals */
    t->t_sigmask |= (mask & ~UNMASKABLE);
    
    return old_mask;
}

/*
 * Set the signal mask for the current thread
 */
long _cdecl sys_p_thread_sigsetmask(ulong mask)
{
    return sys_p_thread_sigmask(mask);
}

/*
 * Temporarily set signal mask and pause until a signal is received
 */
long _cdecl sys_p_thread_sigpause(ulong mask)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    ulong old_mask;
    int sig;
    
    if (!t || !p)
        return -EINVAL;
        
    /* Save old mask */
    old_mask = THREAD_SIGMASK(t);
    
    /* Set new mask */
    t->t_sigmask = mask & ~UNMASKABLE;
    
    /* Wait for any signal */
    sig = sys_p_thread_sigwait(~0UL, -1);
    
    /* Restore old mask */
    t->t_sigmask = old_mask;
    
    return sig;
}

/*
 * Thread sleep function using signals
 */
long _cdecl sys_p_thread_sleep(unsigned int n)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    TIMEOUT *sleep_timeout = NULL;
    
    if (!t || !p)
        return -EINVAL;
        
    /* Set up a timeout */
    long ticks = (n + 4) / 5;  /* Convert milliseconds to ticks */
    if (ticks < 1) ticks = 1;
    
    sleep_timeout = addtimeout(p, ticks, thread_timeout_handler);
    if (sleep_timeout) {
        sleep_timeout->arg = (long)t;
    }
    
    /* Mark thread as sleeping */
    unsigned short sr = splhigh();
    t->sleep_reason = 0;
    atomic_thread_state_change(t, THREAD_STATE_SLEEPING);
    remove_from_ready_queue(t);
    spl(sr);
    
    /* Schedule other threads */
    schedule();
    
    /* When we wake up, cancel the timeout if it's still active */
    if (sleep_timeout) {
        canceltimeout(sleep_timeout);
    }
    
    return 0;
}

/*
 * Initialize thread signal handling for a process
 */
void init_thread_signals(struct proc *p)
{
    if (!p || !p->p_sigacts)
        return;
        
    /* Initialize thread signal handling */
    p->p_sigacts->thread_signals = 0;
    
    /* Initialize thread signal handlers */
    for (int i = 0; i < NSIG; i++) {
        p->p_sigacts->thread_handlers[i].handler = NULL;
        p->p_sigacts->thread_handlers[i].arg = NULL;
        p->p_sigacts->thread_handlers[i].owner = NULL;
    }
}

/*
 * Clean up thread signal handlers for a thread
 */
void cleanup_thread_signals(struct thread *t)
{
    struct proc *p;
    
    if (!t || !t->proc || !t->proc->p_sigacts)
        return;
        
    p = t->proc;
    
    /* Clean up any signal handlers registered by this thread */
    for (int i = 0; i < NSIG; i++) {
        if (p->p_sigacts->thread_handlers[i].owner == t) {
            p->p_sigacts->thread_handlers[i].handler = NULL;
            p->p_sigacts->thread_handlers[i].arg = NULL;
            p->p_sigacts->thread_handlers[i].owner = NULL;
        }
    }
    
    /* Cancel any pending alarm */
    if (t->alarm_timeout) {
        canceltimeout(t->alarm_timeout);
        t->alarm_timeout = NULL;
    }
    
    /* Free signal context if one exists */
    if (t->t_sigctx) {
        kfree(t->t_sigctx);
        t->t_sigctx = NULL;
    }
}

/*
 * Dispatch thread signals
 * Called during context switches and when threads wake up
 */
void dispatch_thread_signals(struct thread *t)
{
    int sig;
    
    if (!t || !t->proc || !t->proc->p_sigacts || !t->proc->p_sigacts->thread_signals)
        return;
        
    /* Check for pending signals */
    sig = check_thread_signals(t);
    if (sig) {
        /* Handle the signal */
        handle_thread_signal(t, sig);
    }
}

/*
 * Main entry point for thread signal system calls
 */
/*
 * Main entry point for thread signal system calls
 */
long _cdecl sys_p_threadsignal(long func, long arg1, long arg2)
{
    TRACE_THREAD("IN KERNEL:sys_p_threadsignal: func=%ld, arg1=%ld, arg2=%ld", func, arg1, arg2);
    
    switch (func) {
        case PTSIG_MODE:
            return sys_p_thread_signal_mode((int)arg1);
            
        case PTSIG_GETMASK:
            return sys_p_thread_sigmask(0);
            
        case PTSIG_SETMASK:
            return sys_p_thread_sigmask(arg1);
            
        case PTSIG_BLOCK:
            return sys_p_thread_sigblock((ulong)arg1);
            
        case PTSIG_UNBLOCK:
            {
                struct thread *t = CURTHREAD;
                if (!t) return -EINVAL;
                ulong old_mask = t->t_sigmask;
                t->t_sigmask &= ~(arg1 & ~UNMASKABLE);
                return old_mask;
            }
            
        case PTSIG_WAIT:
            return sys_p_thread_sigwait(arg1, arg2);
            
        case PTSIG_HANDLER:
            /* Fix: Use arg1 for signal number and arg2 for handler function */
            return sys_p_thread_handler((int)arg1, (void (*)(int, void*))arg2, NULL);
            
        case PTSIG_HANDLER_ARG:
            /* New operation: Set the argument for a previously registered handler */
            return sys_p_thread_handler_arg((int)arg1, (void*)arg2);
            
        case PTSIG_GETID:
            return sys_p_thread_getid();

        case PTSIG_ALARM:
            /* Set alarm for current thread */
            return sys_p_thread_alarm(CURTHREAD, arg1);
            
        case PTSIG_ALARM_THREAD:
            /* Set alarm for specific thread */
            {
                struct thread *t = NULL;
                
                /* Find thread by ID */
                if (arg1 > 0) {
                    struct thread *curr;
                    for (curr = curproc->threads; curr; curr = curr->next) {
                        if (curr->tid == arg1) {
                            t = curr;
                            break;
                        }
                    }
                    
                    if (!t) {
                        TRACE_THREAD("Thread with ID %ld not found", arg1);
                        return -ESRCH;
                    }
                } else {
                    /* Use current thread */
                    t = CURTHREAD;
                }
                
                return sys_p_thread_alarm(t, arg2);
            }

        default:
            if (func > 0 && func < NSIG) {
                /* Direct signal to thread */
                struct thread *target = NULL;
                
                if (arg1 == 0) {
                    /* Signal current thread */
                    target = CURTHREAD;
                } else if (arg1 > 0 && arg1 < 1000) {
                    /* Treat arg1 as a thread ID */
                    struct proc *p = curproc;
                    struct thread *t;
                    
                    for (t = p->threads; t != NULL; t = t->next) {
                        if (t->tid == arg1) {
                            target = t;
                            break;
                        }
                    }
                    
                    if (!target) {
                        TRACE_THREAD("Thread with ID %ld not found", arg1);
                        return -ESRCH;
                    }
                } else {
                    /* Treat arg1 as a thread pointer */
                    target = (struct thread*)arg1;
                }
                
                return sys_p_kill_thread(target, (int)func);
            }
            
            return -EINVAL;
    }
}

/*
 * Initialize the thread signal subsystem
 */
void init_thread_signal_subsystem(void)
{
    /* Nothing to do here for now */
    TRACE_THREAD(("Thread signal subsystem initialized"));
}

/*
 * Set an alarm for the current thread
 * Returns the number of milliseconds remaining in the previous alarm, or 0 if none
 * 
 * This function allows each thread to have its own independent alarm timer.
 * When the alarm expires, the thread receives a SIGALRM signal.
 * If a previous alarm was set, it returns the remaining time in milliseconds.
 */
long _cdecl sys_p_thread_alarm(struct thread *t, long ms)
{
    long remaining = 0;

    if (!t)
        return -EINVAL;

    struct proc *p = t->proc;

    /* Calculate remaining time on current alarm */
    if (t->alarm_timeout) {
        remaining = timeout_remaining(t->alarm_timeout);
        canceltimeout(t->alarm_timeout);
        t->alarm_timeout = NULL;
    }
    
    /* Set new alarm if requested */
    if (ms > 0) {
        /* Convert milliseconds to ticks (200Hz clock = 5ms per tick) */
        long ticks = (ms + 4) / 5;
        if (ticks < 1) ticks = 1;
        
        t->alarm_timeout = addtimeout(p, ticks, thread_alarm_handler);
        if (t->alarm_timeout) {
            t->alarm_timeout->arg = (long)t;
        }
    }
    
    return remaining;
}
