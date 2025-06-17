/**
 * @file proc_threads_signal.c
 * @brief Kernel Thread Signal Handling
 * 
 * Implements signal management for threaded processes within the kernel.
 * Handles per-thread signal masking, targeted delivery, and sigwait operations.
 * 
 * @author Medour Mehdi
 * @date June 2025
 * @version 1.0
 */

 /**
 * Thread Signal Handling
 * 
 * Implements POSIX-compliant thread signal management with per-thread signal
 * masks, queuing, and handlers. Supports targeted signal delivery, sigwait,
 * and real-time signal management in threaded environments.
 */

#include "proc_threads_signal.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"

/* Delivers a signal to a specific thread, returns 1 if delivered, 0 otherwise */
static int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig);

/* Thread timeout handling */
static void thread_timeout_sighandler(PROC *p, long arg);

/* Forward declarations */
static void thread_signal_alarm_handler(PROC *p, long arg);

/* Trampoline function to call thread signal handlers with proper context management */
static void thread_signal_trampoline(int sig, void *arg);
static void handler_execute(int sig, void *arg);

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
    register unsigned short sr = splhigh();
    if ((t->state & THREAD_STATE_BLOCKED) && (t->wait_type & WAIT_SIGNAL)) {
        t->sleep_reason = 1; // Timeout

        // Remove from any wait queue if present
        remove_thread_from_wait_queues(t);
        t->wait_type &= ~WAIT_SIGNAL;
        
        // Add to ready queue
        atomic_thread_state_change(t, THREAD_STATE_READY);
        add_to_ready_queue(t);
    }
    spl(sr);
}

/*
 * Signal trampoline function to call thread signal handlers
 */
/*
 * Signal trampoline function to call thread signal handlers
 */
static void thread_signal_trampoline(int sig, void *arg)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    
    if (!t || !p || !p->p_sigacts || sig <= 0 || sig >= NSIG)
        return;
    /* Clean up any previous signal stack */
    cleanup_signal_stack(p, (long)t);

    /* Get handler from thread-specific storage */
    void (*handler)(int, void*) = t->sig_handlers[sig].handler;
    void *handler_arg = t->sig_handlers[sig].arg;
    
    if (!handler)
        return;

    /* Mark that we're processing a signal */
    t->t_sig_in_progress = sig;

    /* Save the old signal mask */
    t->old_sigmask = THREAD_SIGMASK(t);
    
    /* Add this signal to the mask to prevent recursive handling */
    THREAD_SIGMASK_ADD_SIGNAL(t, sig);
    
    /* Allocate a new stack for the signal handler */
    t->sig_stack = kmalloc(STKSIZE);
    if (!t->sig_stack) {
        TRACE_THREAD("Failed to allocate signal stack for thread %d", t->tid);
        t->t_sig_in_progress = 0;
        return;
    }
    
    /* Set up stack pointers for signal handler */
    unsigned long ssp = ((unsigned long)t->sig_stack + STKSIZE - 128) & ~3L;
    unsigned long usp = ((unsigned long)t->sig_stack + STKSIZE - 256) & ~3L;
    
    /* Set up signal handler context */
    t->sig_ctx.ssp = ssp;
    t->sig_ctx.usp = usp;
    t->sig_ctx.pc = (unsigned long)handler_execute;
    t->sig_ctx.sr = 0x2000;  /* Supervisor mode */
    
    /* Set up arguments in registers */
    t->sig_ctx.regs[0] = sig;  /* D0 = signal number */
    t->sig_ctx.regs[1] = (unsigned long)handler_arg;  /* D1 = handler argument */
    
    /* Create a proper exception frame for RTE */
    unsigned short *frame_ptr = (unsigned short *)(ssp - 8);
    frame_ptr[0] = 0x0000;  /* Format/Vector */
    frame_ptr[1] = 0x2000;  /* SR (Supervisor mode) */
    frame_ptr[2] = (unsigned short)((unsigned long)handler_execute >> 16);
    frame_ptr[3] = (unsigned short)((unsigned long)handler_execute);
    
    memcpy(&t->ctxt[CURRENT].crp, &t->proc->ctxt[CURRENT].crp, sizeof(t->ctxt[CURRENT].crp));
    memcpy(&t->ctxt[CURRENT].tc, &t->proc->ctxt[CURRENT].tc, sizeof(t->ctxt[CURRENT].tc));
        
    /* Update SSP to point to our exception frame */
    t->sig_ctx.ssp = (unsigned long)frame_ptr;
    
    /* Let the scheduler handle the context switch */
    proc_thread_schedule();
}

static void handler_execute(int sig, void *arg)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    
    if (!t || !p || !p->p_sigacts || sig <= 0 || sig >= NSIG)
        return;
        
    /* Get handler from thread-specific storage */
    void (*handler)(int, void*) = t->sig_handlers[sig].handler;
    // void *handler_arg = t->sig_handlers[sig].arg;
    
    if (!handler)
        return;
    
    /* Call the handler */
    (*handler)(sig, t->sig_handlers[sig].arg);
    
    /* Restore signal mask */
    THREAD_SIGMASK_SET(t, t->old_sigmask);
    
    /* Clear signal in progress flag */
    t->t_sig_in_progress = 0;
    
    /* Schedule cleanup of signal stack */
    addtimeout(p, 1, cleanup_signal_stack);
    addtimeout(p, 1, cleanup_signal_stack)->arg = (long)t;
}

/*
 * Process thread signal
 */
void handle_thread_signal(struct thread *t, int sig)
{
    if (!t || sig <= 0 || sig >= NSIG){
        TRACE_THREAD("handle_thread_signal: invalid parameters (t=%p, sig=%d)", t, sig);
        return;
    }
        
    /* If already handling a signal for this thread, exit */
    if (t->t_sig_in_progress) {
        TRACE_THREAD("Signal %d ignored - thread %d already handling signal %d",
                    sig, t->tid, t->t_sig_in_progress);
        return;
    }

    /* Check if there's a thread-specific handler */
    if (t->sig_handlers[sig].handler) {
        /* Execute handler in thread context */
        thread_signal_trampoline(sig, t->sig_handlers[sig].arg);
    } else {
        /* No handler registered yet, keep the signal pending for the thread */
        TRACE_THREAD("handle_thread_signal: no thread handler for signal %d, keeping it pending", sig);
        SET_THREAD_SIGPENDING(t, sig);
    }
}

/*
 * Deliver a signal to a specific thread
 * Returns 1 if signal was delivered, 0 otherwise
 */
static int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig) {
    register unsigned short sr;
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
    if ((t->state & THREAD_STATE_BLOCKED) && (t->wait_type & WAIT_SIGNAL)) {
        TRACE_THREAD("Thread id %d: state is %d and wait_type is %d", t->tid, t->state, t->wait_type);
        TRACE_THREAD("Waking up thread %d due to signal %d", t->tid, sig);
        t->sleep_reason = 0; // Signal

        // Remove from any wait queue if present
        remove_thread_from_wait_queues(t);
        t->wait_type &= ~WAIT_SIGNAL;

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
/*
 * Modified version of raise() that's thread-aware
 */
int proc_thread_signal_aware_raise(struct proc *p, int sig)
{
    if (!p || sig <= 0 || sig >= NSIG)
        return EINVAL;
        
    /* Check if this process has thread-specific signal handling enabled */
    if (p->p_sigacts && p->p_sigacts->thread_signals) {
        /* Try to deliver to threads - prioritize based on signal type and waiting threads */
        if (IS_THREAD_USER_SIGNAL(sig) || (p->p_sigacts->flags & SAS_THREADED)) {
            TRACE_THREAD("Attempting thread-aware delivery for signal %d", sig);
            int delivered = 0;
            
            /* Try to deliver to current thread first */
            if (p->current_thread && p->current_thread->tid != 0) {
                delivered |= deliver_signal_to_thread(p, p->current_thread, sig);
            }
            
            /* First pass: prioritize threads explicitly waiting for this signal in sigwait */
            struct thread *t;
            for (t = p->threads; t != NULL; t = t->next) {
                /* Skip thread0 and current thread */
                if (t->tid == 0 || t == p->current_thread)
                    continue;

                /* Check if thread is explicitly waiting for this signal in sigwait */
                if ((t->state & THREAD_STATE_BLOCKED) && 
                    (t->wait_type & WAIT_SIGNAL) && 
                    t->sig_wait_obj) {
                    ulong wait_mask = (ulong)t->sig_wait_obj;
                    if (wait_mask & (1UL << sig)) {
                        TRACE_THREAD("Thread %d is waiting for signal %d in sigwait (mask=%lx)", 
                                   t->tid, sig, wait_mask);
                        if (deliver_signal_to_thread(p, t, sig)) {
                            delivered = 1;
                            TRACE_THREAD("Signal %d delivered to waiting thread %d", sig, t->tid);
                            break;  /* Deliver to only one waiting thread for efficiency */
                        }
                    }
                }
            }
            
            /* Second pass: if no thread was specifically waiting, try other eligible threads */
            if (!delivered) {
                for (t = p->threads; t != NULL; t = t->next) {
                    /* Skip thread0, current thread, and threads we already checked */
                    if (t->tid == 0 || t == p->current_thread)
                        continue;                    
                    
                delivered |= deliver_signal_to_thread(p, t, sig);
            }
            
            if (delivered)
                return 0;
            }
        }
    }
    
    /* Default: deliver to process as before */
    p->sigpending |= (1UL << sig);
    
    /* Find thread0 to handle process signals */
    struct thread *thread0 = NULL;
    for (struct thread *t = p->threads; t != NULL; t = t->next) {
        if (t->tid == 0) {
            thread0 = t;
            break;
        }
    }
    
    /* Wake up thread0 if it's blocked */
    if (thread0 && (thread0->state & THREAD_STATE_BLOCKED) && thread0 != p->current_thread) {
        TRACE_THREAD("Waking up thread0 to handle process signal %d", sig);
        remove_thread_from_wait_queues(thread0);
        thread0->wait_type &= ~WAIT_SIGNAL;
        atomic_thread_state_change(thread0, THREAD_STATE_READY);
        add_to_ready_queue(thread0);
    }
    
    return 0;
}

/*
 * Check for pending signals in a thread
 * Returns signal number if a signal is pending, 0 otherwise
 */
/*
 * Check for pending signals in a thread
 * Returns signal number if a signal is pending, 0 otherwise
 */
int check_thread_signals(struct thread *t)
{
    int sig;
    register unsigned short sr;
    
    if (!t || !t->proc)
        return 0;
    
    sr = splhigh();  // Protect access to thread signal state
        
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
            /* Clear pending flag */
            t->t_sigpending &= ~(1UL << sig);
            
            /* Return the signal number */
            TRACE_THREAD("check_thread_signals: thread %d has pending signal %d", 
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

    /* Verify thread is still valid and not being destroyed */
    if (t->state == THREAD_STATE_ZOMBIE) {
        TRACE_THREAD("thread_signal_alarm_handler: thread %d is zombie, ignoring alarm", t->tid);
        return;
    }

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
        return EINVAL;
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
        return EINVAL;
        
    ulong old_mask = THREAD_SIGMASK(curproc->current_thread);
    
    /* Ensure we're not masking unmaskable signals */
    if (mask & UNMASKABLE) {
        TRACE_THREAD("proc_thread_signal_sigmask: attempt to mask unmaskable signals");
        /* Remove unmaskable signals from the mask */
        mask &= ~UNMASKABLE;
    }

    /* No need to validate mask bits if NSIG is 32 and ulong is 32 bits */
    #if 0
    /* Create mask for valid signals 1-31 */
    #define SIG_VALID_MASK (((1UL << (NSIG-1)) - 1) << 1)

    /* Validate mask only contains valid signal bits */

    if (mask & ~SIG_VALID_MASK) {
        TRACE_THREAD("proc_thread_signal_sigmask: invalid signal bits in mask %lx", mask);
        mask &= SIG_VALID_MASK;
    }
    #endif
    
    /* Directly set new mask while excluding unmaskable signals */
    THREAD_SIGMASK_SET(curproc->current_thread, mask);
    
    return old_mask;
}

/*
 * Enhanced sigwait with better signal mask management
 * This version temporarily adjusts the thread's signal mask to ensure
 * proper signal delivery during the wait period
 */
long _cdecl proc_thread_signal_sigwait_enhanced(ulong mask, long timeout)
{
    int sig;
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    register unsigned short sr;
    ulong old_mask;
    
    if (!t || !p) {
        TRACE_THREAD("proc_thread_sigwait_enhanced: invalid thread or process");
        return EINVAL;
    }
    
    if (!p->p_sigacts || !p->p_sigacts->thread_signals) {
        TRACE_THREAD("proc_thread_sigwait_enhanced: thread signals not enabled");
        return EINVAL;
    }
    
    TRACE_THREAD("proc_thread_sigwait_enhanced: Thread %d waiting for signals in mask %lx, timeout %ld", 
                t->tid, mask, timeout);
    
    /* Validate mask */
    if (mask & UNMASKABLE) {
        TRACE_THREAD("proc_thread_sigwait_enhanced: removing unmaskable signals from wait mask");
        mask &= ~UNMASKABLE;
    }
    
    /* Save current signal mask and temporarily unblock signals we're waiting for */
    sr = splhigh();
    old_mask = THREAD_SIGMASK(t);
    
    /* Set mask to block all signals except those we're waiting for */
    THREAD_SIGMASK_SET(t, old_mask & ~mask);
    spl(sr);
    
    /* Check for already pending signals that match our wait mask */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))) {
        /* Found a pending signal we're waiting for */
        sr = splhigh();
        THREAD_SIGMASK_SET(t, old_mask);
        spl(sr);
        TRACE_THREAD("proc_thread_sigwait_enhanced: returning immediately with pending signal %d", sig);
        return sig;
    }
    
    /* Call standard sigwait with our adjusted mask */
    sig = proc_thread_signal_sigwait(mask, timeout);
    
    /* Restore original signal mask */
    sr = splhigh();
    THREAD_SIGMASK_SET(t, old_mask);
    spl(sr);
    
    TRACE_THREAD("proc_thread_sigwait_enhanced: returning with signal %d", sig);
    return sig;
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
        return EINVAL;
    }

    p = t->proc;

    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_kill: invalid process");
        return EINVAL;
    }

    /* Check if thread belongs to current process */
    if (p != curproc){
        TRACE_THREAD("proc_thread_signal_kill: thread does not belong to current process");
        return EPERM;
    }

    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_kill: thread-specific signals are disabled");
        return EINVAL;
    }
    /* Deliver signal to thread */
    if (!deliver_signal_to_thread(p, t, sig)){
        TRACE_THREAD("proc_thread_signal_kill: failed to deliver signal to thread");
        return EINVAL;
    }
    TRACE_THREAD("proc_thread_signal_kill: signal %d delivered to thread %d", sig, t->tid);
    return 0;
}

/*
 * Register a thread-specific signal handler
 */
long _cdecl proc_thread_signal_sighandler(int sig, void (*handler)(int, void*), void *arg)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    
    /* Validate parameters */
    if (!t || sig <= 0 || sig >= NSIG){
        TRACE_THREAD("proc_thread_signal_sighandler: invalid signal %d", sig);
        return EINVAL;
    }
        
    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_sighandler: invalid process");
        return EINVAL;
    }
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_sighandler: thread-specific signals are disabled");
        return EINVAL;
    }

    /* Don't allow thread0 to set thread-specific handlers */
    if (t->tid == 0) {
        TRACE_THREAD("proc_thread_signal_sighandler: thread0 cannot set thread-specific handlers");
        return EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sighandler: PROC ID %d, THREAD ID %d, SIG %d, HANDLER %p, ARG %p",
                 p->pid, t->tid, sig, handler, arg);

    /* Store handler in thread-specific storage */
    t->sig_handlers[sig].handler = handler;
    t->sig_handlers[sig].arg = arg;
    
    return 0;
}

/*
 * Set the argument for a thread-specific signal handler
 */
long _cdecl proc_thread_signal_sighandler_arg(int sig, void *arg)
{
    struct thread *t = CURTHREAD;
    struct proc *p = curproc;
    
    /* Validate parameters */
    if (!t || sig <= 0 || sig >= NSIG){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: invalid signal %d", sig);
        return EINVAL;
    }
        
    if (!p || !p->p_sigacts){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: invalid process");
        return EINVAL;
    }
        
    /* Make sure thread-specific signals are enabled */
    if (!p->p_sigacts->thread_signals){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: thread-specific signals are disabled");
        return EINVAL;
    }
        
    /* Make sure a handler is already registered */
    if (!t->sig_handlers[sig].handler){
        TRACE_THREAD("proc_thread_signal_sighandler_arg: no handler registered for signal %d", sig);
        return EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sighandler_arg: PROC ID %d, THREAD ID %d, SIG %d, ARG %p",
                 p->pid, t->tid, sig, arg);
    
    /* Set handler argument in thread-specific storage */
    t->sig_handlers[sig].arg = arg;
    
    return 0;
}

/*
 * Wait for signals with timeout
 */
long _cdecl proc_thread_signal_sigwait(ulong mask, long timeout)
{
    register unsigned short sr;
    int sig;

    struct proc *p = curproc;
    struct thread *t = CURTHREAD;
    
    TIMEOUT *wait_timeout = NULL;
    
    if (!t || !p){
        TRACE_THREAD("proc_thread_signal_sigwait: invalid thread or process");
        return EINVAL;
    }

    TRACE_THREAD("proc_thread_signal_sigwait: PROC ID %d, THREAD ID %d, mask %lx, timeout %ld", 
                 p->pid, t->tid, mask, timeout);

    /* Check for pending signals first */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))) {
        TRACE_THREAD("proc_thread_signal_sigwait: returning immediately with signal %d", sig);
        return sig;
    }

    /* Enhanced validation for wait mask */
    if (!mask) {
        TRACE_THREAD("proc_thread_signal_sigwait: empty wait mask");
        return EINVAL;
    }
    
    /* Remove unmaskable signals from wait mask */
    if (mask & UNMASKABLE) {
        TRACE_THREAD("proc_thread_signal_sigwait: removing unmaskable signals from wait mask");
        mask &= ~UNMASKABLE;
        if (!mask) {
            TRACE_THREAD("proc_thread_signal_sigwait: wait mask became empty after removing unmaskable signals");
            return EINVAL;
        }
    }
    
    /* Store the wait mask in wait_obj for signal routing */
    sr = splhigh();
    t->sig_wait_obj = (void*)mask;
    spl(sr);

    /* If timeout is 0, just check and return */
    if (timeout == 0) {
        TRACE_THREAD("proc_thread_signal_sigwait: timeout is 0, returning immediately");
        return 0;
    }
        
    /* Set up a timeout if requested */
    if (timeout > 0) {
        wait_timeout = addtimeout(p, timeout, thread_timeout_sighandler);
        if (wait_timeout) {
            wait_timeout->arg = (long)t;
        }
    }


    TRACE_THREAD("proc_thread_signal_sigwait: Mark thread as waiting for signal - PROC ID %d, THREAD ID %d, mask %lx, timeout %ld",
                p->pid, t->tid, mask, timeout);
    /* Mark thread as waiting for signal */
    sr = splhigh();
    t->sleep_reason = 0;
    atomic_thread_state_change(t, THREAD_STATE_BLOCKED);
    t->wait_type |= WAIT_SIGNAL;

    TRACE_THREAD("proc_thread_signal_sigwait: Adding to signal_wait_queue - PROC ID %d, THREAD ID %d, mask %lx, timeout %ld",
                p->pid, t->tid, mask, timeout);

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
    t->sig_wait_obj = (void*)mask;
    TRACE_THREAD("proc_thread_signal_sigwait: remove_from_ready_queue - PROC ID %d, THREAD ID %d, mask %lx, timeout %ld",
                p->pid, t->tid, mask, timeout);
    remove_from_ready_queue(t);
    spl(sr);
    
    /* Schedule other threads */
    TRACE_THREAD("proc_thread_signal_sigwait: Schedule other threads - PROC ID %d, THREAD ID %d, mask %lx, timeout %ld",
                p->pid, t->tid, mask, timeout);    
    proc_thread_schedule();

    if (CURTHREAD != t) {
        TRACE_THREAD("proc_thread_signal_sigwait: wrong thread after schedule -> THREAD id is %ld instead of wanted %ld - EAGAIN, please retry", CURTHREAD->tid, t->tid);
        return EAGAIN;
    }

    TRACE_THREAD("proc_thread_signal_sigwait: Returned from scheduler - PROC ID %d, THREAD ID %d, mask %lx, timeout %ld",
            p->pid, t->tid, mask, timeout);  
    /* Cancel timeout if it exists */
    sr = splhigh();
    if (wait_timeout) {
        canceltimeout(wait_timeout);
        wait_timeout = NULL;
    }

    /* Clear wait object */
    sr = splhigh();
    t->sig_wait_obj = NULL;
    spl(sr);

    /* Check why we woke up */
    sig = check_thread_signals(t);
    if (sig && (mask & (1UL << sig))) {
        spl(sr);
        TRACE_THREAD("proc_thread_signal_sigwait: returning with signal %d", sig);
        
        /* If there's a handler for this signal, call it before returning */
        if (t->sig_handlers[sig].handler) {
            thread_signal_trampoline(sig, t->sig_handlers[sig].arg);
        }
        
        return sig;
    }

    /* Check for timeout */
    if (t->sleep_reason == 1) {
        spl(sr);
        TRACE_THREAD("proc_thread_signal_sigwait: returning due to timeout");
        return 0;
    }

    spl(sr);
    return EINTR;  // Interruption, le thread appelant doit réessayer
}

/*
 * Block signals for the current thread (add to mask)
 */
long _cdecl proc_thread_signal_sigblock(ulong mask)
{
    struct thread *t = CURTHREAD;
    
    if (!t)
        return EINVAL;
        
    ulong old_mask = THREAD_SIGMASK(t);
    
   /* Check if any unmaskable signals were attempted to be blocked */
   if (mask & UNMASKABLE) {
       TRACE_THREAD("proc_thread_signal_sigblock: attempt to block unmaskable signals");
   }
   
   /* Merge assignment to add signals to mask, excluding unmaskable ones */
   THREAD_SIGMASK_ADD(t, mask);
    
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
        return EINVAL;
        
    /* Save old mask */
    old_mask = THREAD_SIGMASK(t);
    
    /* Set new mask */
    THREAD_SIGMASK_SET(t, mask);
    
    /* Wait for any signal */
    sig = proc_thread_signal_sigwait(~0UL, -1);
    
    /* Restore old mask */
    t->t_sigmask = old_mask;
    
    return sig;
}

/*
 * Clean up thread signal handlers for a thread
 */
void cleanup_thread_signals(struct thread *t)
{
    
    if (!t || !t->proc || !t->proc->p_sigacts)
        return;
    
    register unsigned short sr = splhigh();
    
    /* Clean up any signal handlers registered by this thread */
    if (t->magic == CTXT_MAGIC) {
        for (int i = 0; i < NSIG; i++) {
            t->sig_handlers[i].handler = NULL;
            t->sig_handlers[i].arg = NULL;
        }
    }

    /* Cancel any pending alarm for this thread */
    if (t->alarm_timeout) {
        canceltimeout(t->alarm_timeout);
        t->alarm_timeout = NULL;
    }
    
    /* Clear signal-related state */
    t->t_sig_in_progress = 0;
    t->t_sigpending = 0;
    t->sig_wait_obj = NULL;
    
    /* Free signal stack if allocated */
    if (t->sig_stack) {
        kfree(t->sig_stack);
        t->sig_stack = NULL;
    }

    spl(sr);
}

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
        return EINVAL;

    struct proc *p = t->proc;

    /* Calculate remaining time on current alarm */
    if (t->alarm_timeout) {
        remaining = timeout_remaining(t->alarm_timeout);
        register unsigned short sr = splhigh();
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

/* Thread signal handling implementation */

/*
 * Initialize thread signal handling for a process
 */
void init_thread_signals(struct proc *p)
{
    if (!p || !p->p_sigacts)
        return;
        
    /* Initialize thread signal handling */
    p->p_sigacts->thread_signals = 0;
    
    struct thread *t;
    for (t = p->threads; t != NULL; t = t->next) {
        if (t->magic == CTXT_MAGIC) {
            for (int i = 0; i < NSIG; i++) {
                t->sig_handlers[i].handler = NULL;
                t->sig_handlers[i].arg = NULL;
            }
        }
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
    /* Special handling for thread0 - it handles process signals */
    if (t->tid == 0) {
        /* No need to check for thread signals on thread0 */
        return;
    }        
    /* Check for pending signals */
    sig = check_thread_signals(t);
    if (sig) {
        /* Handle the signal */
        handle_thread_signal(t, sig);
    }
}

/* Clean up signal stack if needed */
void cleanup_signal_stack(PROC *p, long arg)
{
    struct thread *t = (struct thread *)arg;
    
    if (!t || t->magic != CTXT_MAGIC) {
        TRACE_THREAD("cleanup_signal_stack: Invalid thread pointer");
        return;
    }
        
    if (t->t_sig_in_progress == 0 && t->sig_stack) {
        void *stack_to_free = t->sig_stack;
        t->sig_stack = NULL;
        kfree(stack_to_free);
        TRACE_THREAD("Freed signal stack for thread %d", t->tid);
    }
}

/*
 * Broadcast a signal to all threads in a process (except thread0)
 * Useful for implementing process-wide notifications
 */
long _cdecl proc_thread_signal_broadcast(int sig)
{
    struct proc *p = curproc;
    if (!p || !p->p_sigacts || !p->p_sigacts->thread_signals)
        return EINVAL;
        
    /* Use the enhanced raise function which handles broadcasting */
    return proc_thread_signal_aware_raise(p, sig);
}