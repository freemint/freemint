#include "proc_threads_signal.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"

/* Checks for pending signals in a thread, returns signal number or 0 */
static int check_thread_signals(struct thread *t);
/* Delivers a signal to a specific thread, returns 1 if delivered, 0 otherwise */
static int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig);

/* Thread timeout handling */
static void thread_timeout_sighandler(PROC *p, long arg);

/* Forward declarations */
static void thread_signal_alarm_handler(PROC *p, long arg);

/* Not implemented yet */
// static void init_thread_signals(struct proc *p);
// static void dispatch_thread_signals(struct thread *t);
// static void handle_thread_signal(struct thread *t, int sig);
// static void cleanup_thread_signals(struct thread *t);
/* Trampoline function to call thread signal handlers with proper context management */
// static void thread_signal_trampoline(int sig, void *arg);
/* Saves thread execution context before signal handling */
// static void save_thread_signal_context(struct thread *t, struct thread_sigcontext *ctx);
/* Restores thread execution context after signal handling */
// static void restore_thread_signal_context(struct thread *t, struct thread_sigcontext *ctx);

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
static void thread_timeout_sighandler(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    TRACE_THREAD("thread_timeout_sighandler: thread %d timeout", t ? t->tid : -1);
    if(!t) return;
    unsigned short sr = splhigh();
    if ((t->state & THREAD_STATE_BLOCKED) && t->wait_type == WAIT_SIGNAL) {
        t->sleep_reason = 1; // Timeout

        // Remove from any wait queue if present
        remove_thread_from_wait_queues(t);
        t->wait_type = WAIT_NONE;
        
        // Add to ready queue
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
    }
    spl(sr);
}

/*
 * Save thread execution context before signal handling
 */
// static void save_thread_signal_context(struct thread *t, struct thread_sigcontext *ctx)
// {
//     if (!t || !ctx)
//         return;
    
//     /* Copy the entire context structure */
//     memcpy(&ctx->sigcontext, &t->ctxt[CURRENT], sizeof(CONTEXT));
    
//     /* Save additional information */
//     ctx->sc_thread = t;
//     ctx->sc_sigmask_save = THREAD_SIGMASK(t);
    
//     /* sc_sig and sc_handler_arg are filled in by the trampoline */
// }

/*
 * Restore thread execution context after signal handling
 */
// static void restore_thread_signal_context(struct thread *t, struct thread_sigcontext *ctx)
// {
//     if (!t || !ctx || ctx->sc_thread != t)
//         return;
    
//     /* Copy the context back */
//     memcpy(&t->ctxt[CURRENT], &ctx->sigcontext, sizeof(CONTEXT));
    
//     /* Restore signal mask */
//     THREAD_SIGMASK_SET(t, ctx->sc_sigmask_save);
// }

/*
 * Signal trampoline function to call thread signal handlers
 */
// static void thread_signal_trampoline(int sig, void *arg)
// {
//     struct thread *t = CURTHREAD;
//     struct proc *p = curproc;
//     struct thread_sigcontext *ctx;
    
//     if (!t || !p || !p->p_sigacts || sig <= 0 || sig >= NSIG)
//         return;
        
//     /* Get handler */
//     void (*handler)(int, void*) = p->p_sigacts->thread_handlers[sig].handler;
//     void *handler_arg = p->p_sigacts->thread_handlers[sig].arg;
    
//     if (!handler)
//         return;

//    /* Mark that we're processing a signal */
//    t->t_sig_in_progress = sig;

//     /* Save context */
//     unsigned short sr = splhigh();
//     ctx = (struct thread_sigcontext *)kmalloc(sizeof(struct thread_sigcontext));
//     if (!ctx){
//         TRACE_THREAD("Failed to allocate context for thread %d", t->tid);
//         /* Clear signal in progress flag before returning */
//         t->t_sig_in_progress = 0;        
//         spl(sr);
//         return;
//     }

//     TRACE_THREAD("KMALLOC: Allocated thread signal context at %p", ctx);
//     save_thread_signal_context(t, ctx);
//     spl(sr);

//     /* Store context and signal info */
//     ctx->sc_sig = sig;
//     ctx->sc_handler_arg = handler_arg;
//     t->t_sigctx = ctx;
    
//     /* Call handler */
//     (*handler)(sig, handler_arg);
    
//     /* Restore context and free it */
//     sr = splhigh();
//     restore_thread_signal_context(t, ctx);
//     t->t_sigctx = NULL;
//     TRACE_THREAD("KFREE: Freeing thread signal context for thread %d", t->tid);
//     kfree(ctx);
    
//     /* Clear signal in progress flag */
//     t->t_sig_in_progress = 0;
//     spl(sr);
// }

/*
 * Process thread signal
 */
// void handle_thread_signal(struct thread *t, int sig)
// {
//     struct proc *p;
    
//     if (!t || !t->proc || sig <= 0 || sig >= NSIG){
//         TRACE_THREAD("handle_thread_signal: invalid parameters (t=%p, sig=%d)", t, sig);
//         return;
//     }
        
//     p = t->proc;

//     /* Si déjà en train de traiter un signal pour ce thread, on sort */
//     if (t->t_sig_in_progress) {
//         TRACE_THREAD("Signal %d ignored - thread %d already handling signal %d",
//                     sig, t->tid, t->t_sig_in_progress);
//         return;
//     }

//     /* Check if there's a thread-specific handler */
//     if (p->p_sigacts && p->p_sigacts->thread_handlers[sig].handler) {
//         /* Execute handler in thread context */
//         thread_signal_trampoline(sig, p->p_sigacts->thread_handlers[sig].arg);
//     } else {
//         TRACE_THREAD("handle_thread_signal: no thread handler for signal %d, falling back to process handler", sig);
//         /* Fall back to process-level signal handling */
//         /* This will use the standard signal dispatcher */
//         p->sigpending |= (1UL << sig);
//     }
// }

/*
 * Deliver a signal to a specific thread
 * Returns 1 if signal was delivered, 0 otherwise
 */
static int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig) {
    unsigned short sr;
    if (!p || !t || sig <= 0 || sig >= NSIG){
        TRACE_THREAD("ERROR: Invalid parameters for deliver_signal_to_thread");
        return 0;
    }
    sr = splhigh();  // Protect access to thread state
    if (t->proc != p) {
        TRACE_THREAD("ERROR: Attempt to deliver signal %d to thread %d in wrong process", 
                    sig, t->tid);
        spl(sr);
        return 0;  /* Don't crash, just fail safely */
    }

    /* Check if this signal should be handled at thread level */
   if ( !p->p_sigacts || !p->p_sigacts->thread_signals ) {
        TRACE_THREAD("ERROR: Attempt to deliver signal %d to thread %d but no thread handlers are set",
            sig, t->tid);
        /* Thread-specific signal handling is disabled */
        spl(sr);
        return 0;  /* Process-level signal, don't handle here */
    }

    /* For non-thread signals, only handle if explicitly allowed */
    if (!IS_THREAD_USER_SIGNAL(sig) && !(p->p_sigacts->flags & SAS_THREADED)) {
        TRACE_THREAD("ERROR: Attempt to deliver non-thread signal %d to thread %d",
                    sig, t->tid);
        spl(sr);
        return 0;  /* Not a thread signal and thread handling not enabled for all signals */
    }

    /* Check if signal is blocked by thread */
    if (THREAD_SIGMASK(t) & (1UL << sig)){
        TRACE_THREAD("ERROR: Attempt to deliver signal %d to blocked thread %d",
                    sig, t->tid);
        spl(sr);
        return 0;
    }

    TRACE_THREAD("Delivering signal %d to thread %d", sig, t->tid);
    /* Mark signal as pending for this thread */
    SET_THREAD_SIGPENDING(t, sig);
    
    /* If thread is blocked, wake it up */
    if ((t->state & THREAD_STATE_BLOCKED) && (t->wait_type == WAIT_SIGNAL)) {

        TRACE_THREAD("Waking up thread %d due to signal %d", t->tid, sig);
        t->sleep_reason = 0; // Signal

        // Remove from any wait queue if present
        remove_thread_from_wait_queues(t);
        t->wait_type = WAIT_NONE;

        // Add to ready queue if not already there
        atomic_thread_state_change(t, THREAD_STATE_READY);
        if (!is_in_ready_queue(t)) {
            add_to_ready_queue(t);
        }
    }

    spl(sr);
    return 1;
}

/*
 * Modified version of raise() that's thread-aware
 */
int proc_thread_signal_aware_raise(struct proc *p, int sig)
{
    if (!p || sig <= 0 || sig >= NSIG)
        return -EINVAL;
        
    /* Check if this process has thread-specific signal handling enabled */
    if (p->p_sigacts && p->p_sigacts->thread_signals) {
        /* For certain signals, deliver to specific thread */
        if (IS_THREAD_USER_SIGNAL(sig)) {
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
    unsigned short sr;
    
    if (!t || !t->proc)
        return 0;
    
    sr = splhigh();  // Protect access to thread signal state

    /* Don't process signals if we're already handling one */
    if (t->t_sig_in_progress){
        TRACE_THREAD("check_thread_signals: thread %d already processing signal %d", 
                    t->tid, t->t_sig_in_progress);
        spl(sr);
        return 0;
    }
        
    /* Check for pending signals that aren't masked */
    ulong pending = THREAD_SIGPENDING(t) & ~THREAD_SIGMASK(t);
    
    if (!pending){
        TRACE_THREAD("check_thread_signals: no pending signals for thread %d", t->tid);
        spl(sr);
        return 0;
    }
        
    /* Find the first pending signal */
    for (sig = 1; sig < NSIG; sig++) {
        if (pending & (1UL << sig)) {
            // /* Clear pending flag */
            t->t_sigpending &= ~(1UL << sig);
            
            /* Mark that we're processing a signal */
            t->t_sig_in_progress = sig;
            TRACE_THREAD("check_thread_signals: thread %d processing signal %d", 
                        t->tid, sig);
            spl(sr);
            return sig;
        }
    }
    TRACE_THREAD("check_thread_signals: no valid pending signals for thread %d", t->tid);
    spl(sr);
    return 0;
}

/*
 * Thread alarm handler
 */
static void thread_signal_alarm_handler(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    
    if (!t || !p) {
        TRACE_THREAD(("thread_signal_alarm_handler: invalid thread or process"));
        return;
    }

    /* Verify thread belongs to this process */
    if (t->proc != p) {
        TRACE_THREAD("thread_signal_alarm_handler: thread %d belongs to different process", 
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
long _cdecl proc_thread_signal_mode(int enable)
{
    if (!curproc || !curproc->p_sigacts){
        TRACE_THREAD("proc_thread_signal_mode: invalid process");
        return -EINVAL;
    }
        
    if (enable) {
        TRACE_THREAD("proc_thread_signal_mode: enabling thread signals");
        curproc->p_sigacts->thread_signals = 1;
        curproc->p_sigacts->flags |= SAS_THREADED;
    } else {
        TRACE_THREAD("proc_thread_signal_mode: disabling thread signals");
        curproc->p_sigacts->thread_signals = 0;
        curproc->p_sigacts->flags &= ~SAS_THREADED;
    }
        
    return 0;
}

/*
 * Set signal mask for current thread
 */
long _cdecl proc_thread_signal_sigmask(ulong mask)
{
    if (!curproc || !curproc->current_thread)
        return -EINVAL;
        
    ulong old_mask = THREAD_SIGMASK(curproc->current_thread);
    
    /* Ensure we're not masking unmaskable signals */
    if (mask & UNMASKABLE) {
        TRACE_THREAD("proc_thread_signal_sigmask: attempt to mask unmaskable signals");
    }
    
    /* Directly set new mask while excluding unmaskable signals */
    curproc->current_thread->t_sigmask = mask & ~UNMASKABLE;
    
    return old_mask;
}

/*
 * Send a signal to a specific thread
 */
long _cdecl proc_thread_signal_kill(struct thread *t, int sig)
{
    struct proc *p;
    
    /* Validate parameters */
    if (!t || sig <= 0 || sig >= NSIG){
        TRACE_THREAD("proc_thread_signal_kill: invalid parameters (t=%p, sig=%d)", t, sig);
        return -EINVAL;
    }

    p = t->proc;

    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_kill: invalid process");
        return -EINVAL;
    }

    /* Check if thread belongs to current process */
    if (p != curproc){
        TRACE_THREAD("proc_thread_signal_kill: thread does not belong to current process");
        return -EPERM;
    }

    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_kill: thread-specific signals are disabled");
        return -EINVAL;
    }
    /* Deliver signal to thread */
    if (deliver_signal_to_thread(p, t, sig)){
        TRACE_THREAD("proc_thread_signal_kill: failed to deliver signal to thread");
        return 0;
    }
    TRACE_THREAD("proc_thread_signal_kill: signal %d delivered to thread %d", sig, t->tid);
    return -EINVAL;
}

/*
 * Register a thread-specific signal handler
 */
long _cdecl proc_thread_signal_sighandler(int sig, void (*handler)(int, void*), void *arg)
{
    struct proc *p;
    
    /* Validate parameters */
    if (sig <= 0 || sig >= NSIG){
        TRACE_THREAD("proc_thread_signal_sighandler: invalid signal %d", sig);
        return -EINVAL;
    }
        
    p = curproc;

    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_sighandler: invalid process");
        return -EINVAL;
    }
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_sighandler: thread-specific signals are disabled");
        return -EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sighandler: PROC ID %d, THREAD ID %d, SIG %d, HANDLER %p, ARG %p",
                 p->pid, CURTHREAD->tid, sig, handler, arg);

    /* Set handler */
    p->p_sigacts->thread_handlers[sig].handler = handler;
    p->p_sigacts->thread_handlers[sig].arg = arg;
    p->p_sigacts->thread_handlers[sig].owner = CURTHREAD;
    
    return 0;
}

/*
 * Set the argument for a thread-specific signal handler
 */
long _cdecl proc_thread_signal_sighandler_arg(int sig, void *arg)
{
    struct proc *p = curproc;
    
    /* Validate parameters */
    if (sig <= 0 || sig >= NSIG){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: invalid signal %d", sig);
        return -EINVAL;
    }
        
    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: invalid process");
        return -EINVAL;
    }
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: thread-specific signals are disabled");
        return -EINVAL;
    }
        
    /* Make sure a handler is already registered */
    if (!p->p_sigacts->thread_handlers[sig].handler){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: no handler registered for signal %d", sig);
        return -EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sighandler_arg: PROC ID %d, THREAD ID %d, SIG %d, ARG %p",
                 p->pid, CURTHREAD->tid, sig, arg);
    /* Set handler argument */
    p->p_sigacts->thread_handlers[sig].arg = arg;
    
    return 0;
}

/*
 * Wait for signals with timeout
 */
long _cdecl proc_thread_signal_sigwait(ulong mask, long timeout)
{
    unsigned short sr;
    int sig;

    struct proc *p = curproc;
    struct thread *t = CURTHREAD;
    
    TIMEOUT *wait_timeout = NULL;
    
    if (!t || !p){
        TRACE_THREAD("proc_thread_signal_sigwait: invalid thread or process");
        return -EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sigwait: PROC ID %d, THREAD ID %d, mask %lx, timeout %ld", 
                 p->pid, t->tid, mask, timeout);

    /* Check for pending signals first */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))) {
        TRACE_THREAD("proc_thread_signal_sigwait: returning immediately with signal %d", sig);
        return sig;
    }
        
    /* If timeout is 0, just check and return */
    if (timeout == 0) {
        TRACE_THREAD("proc_thread_signal_sigwait: timeout is 0, returning immediately");
        return 0;
    }
        
    /* Set up a timeout if requested */
    if (timeout > 0) {
        long ticks = (timeout + 4) / MS_PER_TICK;
        if (ticks < 1) ticks = 1;
        
        // wait_timeout = addtimeout(p, ticks, thread_timeout_sighandler);
        wait_timeout = addtimeout(p, timeout, thread_timeout_sighandler);
        if (wait_timeout) {
            wait_timeout->arg = (long)t;
        }
    }

    /* Mark thread as waiting for signal */
    sr = splhigh();
    t->sleep_reason = 0;
    atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
    t->wait_type = WAIT_SIGNAL;

    // Add to signal wait queue
    t->next_sigwait = NULL;
    if (p->signal_wait_queue) {
        struct thread *last = p->signal_wait_queue;
        while (last->next_sigwait)
            last = last->next_sigwait;
        last->next_sigwait = t;
    } else {
        p->signal_wait_queue = t;
    }
    t->wait_obj = (void*)mask;

    remove_from_ready_queue(t);
    spl(sr);
    
    /* Schedule other threads */
    proc_thread_schedule();

    if (CURTHREAD != t) {
        TRACE_THREAD("proc_thread_signal_sigwait: wrong thread after schedule -> THREAD id is %ld instead of wanted %ld - EAGAIN, please retry", CURTHREAD->tid, t->tid);
        return -EAGAIN;
    }

    /* Cancel timeout if it exists */
    sr = splhigh();
    if (wait_timeout) {
        canceltimeout(wait_timeout);
        wait_timeout = NULL;
    }

    /* Check why we woke up */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))) {
        spl(sr);
        TRACE_THREAD("proc_thread_signal_sigwait: returning with signal %d", sig);
        return sig;
    }

    /* Check for timeout */
    if (t->sleep_reason == 1) {
        spl(sr);
        TRACE_THREAD("proc_thread_signal_sigwait: returning due to timeout");
        return 0;
    }

    spl(sr);
    return -EINTR;  // Interruption, le thread appelant doit réessayer
}

/*
 * Block signals for the current thread (add to mask)
 */
long _cdecl proc_thread_signal_sigblock(ulong mask)
{
    struct thread *t = CURTHREAD;
    
    if (!t)
        return -EINVAL;
        
    ulong old_mask = THREAD_SIGMASK(t);
    
   /* Check if any unmaskable signals were attempted to be blocked */
   if (mask & UNMASKABLE) {
       TRACE_THREAD("proc_thread_signal_sigblock: attempt to block unmaskable signals");
   }
   
   /* Merge assignment to add signals to mask, excluding unmaskable ones */
   t->t_sigmask |= (mask & ~UNMASKABLE);
    
    return old_mask;
}

/*
 * Set the signal mask for the current thread
 */
long _cdecl sys_p_thread_sigsetmask(ulong mask)
{
    return proc_thread_signal_sigmask(mask);
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
    sig = proc_thread_signal_sigwait(~0UL, -1);
    
    /* Restore old mask */
    t->t_sigmask = old_mask;
    
    return sig;
}

/*
 * Clean up thread signal handlers for a thread
 */
// static void cleanup_thread_signals(struct thread *t)
// {
//     struct proc *p;
    
//     if (!t || !t->proc || !t->proc->p_sigacts)
//         return;
    
//     unsigned short sr = splhigh();
//     p = t->proc;
    
//     /* Clean up any signal handlers registered by this thread */
//     for (int i = 0; i < NSIG; i++) {
//         if (p->p_sigacts->thread_handlers[i].owner == t) {
//             p->p_sigacts->thread_handlers[i].handler = NULL;
//             p->p_sigacts->thread_handlers[i].arg = NULL;
//             p->p_sigacts->thread_handlers[i].owner = NULL;
//         }
//     }
    
//     /* Cancel any pending alarm */
//     if (t->alarm_timeout) {
//         canceltimeout(t->alarm_timeout);
//         t->alarm_timeout = NULL;
//     }
    
//     /* Free signal context if one exists */
//     if (t->t_sigctx) {
//         TRACE_THREAD("KFREE: Freeing thread signal context for thread %d", t->tid);
//         kfree(t->t_sigctx);
//         t->t_sigctx = NULL;
//     }

//     /* Clear thread signal state */
//     t->t_sigpending = 0;
//     t->t_sigmask = p->p_sigmask;  /* Inherit process signal mask */
//     t->t_sig_in_progress = 0;
//     spl(sr);
// }

/*
 * Set an alarm for the current thread
 * Returns the number of milliseconds remaining in the previous alarm, or 0 if none
 * 
 * This function allows each thread to have its own independent alarm timer.
 * When the alarm expires, the thread receives a SIGALRM signal.
 * If a previous alarm was set, it returns the remaining time in milliseconds.
 */
long _cdecl proc_thread_signal_sigalrm(struct thread *t, long ms)
{
    long remaining = 0;

    if (!t)
        return -EINVAL;

    struct proc *p = t->proc;

    /* Calculate remaining time on current alarm */
    if (t->alarm_timeout) {
        remaining = timeout_remaining(t->alarm_timeout);
        unsigned short sr = splhigh();
        canceltimeout(t->alarm_timeout);
        t->alarm_timeout = NULL;
        spl(sr);
    }
    
    /* Set new alarm if requested */
    if (ms > 0) {
        t->alarm_timeout = addtimeout(p, ms, thread_signal_alarm_handler);
        if (t->alarm_timeout) {
            t->alarm_timeout->arg = (long)t;
        }
    }
    
    return remaining;
}

/* Not implemented yet */

/*
 * Initialize thread signal handling for a process
 */
// static void init_thread_signals(struct proc *p)
// {
//     if (!p || !p->p_sigacts)
//         return;
        
//     /* Initialize thread signal handling */
//     p->p_sigacts->thread_signals = 0;
    
//     /* Initialize thread signal handlers */
//     for (int i = 0; i < NSIG; i++) {
//         p->p_sigacts->thread_handlers[i].handler = NULL;
//         p->p_sigacts->thread_handlers[i].arg = NULL;
//         p->p_sigacts->thread_handlers[i].owner = NULL;
//     }
// }

/*
 * Dispatch thread signals
 * Called during context switches and when threads wake up
 */
// static void dispatch_thread_signals(struct thread *t)
// {
//     int sig;
    
//     if (!t || !t->proc || !t->proc->p_sigacts || !t->proc->p_sigacts->thread_signals)
//         return;
        
//     /* Check for pending signals */
//     sig = check_thread_signals(t);
//     if (sig) {
//         /* Handle the signal */
//         handle_thread_signal(t, sig);
//     }
// }

