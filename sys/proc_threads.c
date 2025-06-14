#include "proc_threads.h"

#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"
#include "proc_threads_sync.h"
#include "proc_threads_signal.h"
#include "proc_threads_tsd.h"

/* Threads stuff */

static long create_thread(struct proc *p, void *(*func)(void*), void *arg, void *stack_ptr);
static void proc_thread_start(void);
static void init_main_thread_context(struct proc *p);
static void init_thread_context(struct thread *t, void *(*func)(void*), void *arg);
static struct thread* create_idle_thread(struct proc *p);
static void *idle_thread_func(void *arg);

// Thread creation syscall
long _cdecl proc_thread_create(void *(*func)(void*), void *arg, void *stack) {    
    TRACE_THREAD("CREATETHREAD: func=%p arg=%p stack=%p", func, arg, stack);

    init_main_thread_context(curproc);
    return create_thread(curproc, func, arg, stack);
}

static long create_thread(struct proc *p, void *(*func)(void*), void *arg, void* stack_ptr) {
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

    t->is_idle = 0;  // Not an idle thread
    
    /* Map process priority to thread priority (keep positive values) */
    t->priority = MAX(scale_thread_priority(-p->pri), 1);
    t->original_priority = t->priority;
    
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

    /* Initialize thread-specific data */
    t->tsd_data = NULL;
    init_thread_tsd(t);

    /* Initialize signal fields */
    t->t_sigpending = 0;
    THREAD_SIGMASK_SET(t, p->p_sigmask);  /* Inherit process signal mask */
    t->t_sig_in_progress = 0;
    t->alarm_timeout = NULL;
    t->sig_stack = NULL;
    t->old_sigmask = 0;
    for (int i = 0; i < NSIG; i++) {
        t->sig_handlers[i].handler = NULL;
        t->sig_handlers[i].arg = NULL;
    }
    // Allocate stack
    if(stack_ptr != NULL){
        t->stack = stack_ptr;
    } else {
        t->stack = kmalloc(STKSIZE);
    }
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
    // Link into process thread list
    t->next = p->threads;
    p->threads = t;

	t->mutex_wait_obj = NULL;
	t->sem_wait_obj = NULL;
	t->sig_wait_obj = NULL;
	t->cond_wait_obj = NULL;
	t->join_wait_obj = NULL;
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

    TRACE_THREAD_CREATE(t, t->func, t->arg);
    spl(sr);
    return t->tid;
}

static void proc_thread_start(void) {
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
    void* (*func)(void*) = t->func;
    void *arg = t->arg;
    void *result = NULL;
    TRACE_THREAD("START: Thread function=%p, arg=%p", func, arg);
    
    // Call the function
    if (func) {
        TRACE_THREAD("START: Calling thread function");
        result = func(arg);  // Capture return value
        TRACE_THREAD("START: Thread function returned");
    }
    
    if (t && t->magic == CTXT_MAGIC) {
        TRACE_THREAD_EXIT(t, result);
        proc_thread_exit(result);
    }
    
    // Should never reach here
    TRACE_THREAD("START: Thread didn't exit properly!");
    while(1) { /* hang */ }
}

static void init_thread_context(struct thread *t, void *(*func)(void*), void *arg) {
    TRACE_THREAD("INIT CONTEXT: Initializing context for thread %d", t->tid);
    
    // Clear context
    mint_bzero (&t->ctxt[CURRENT], sizeof(t->ctxt[CURRENT]));
    mint_bzero (&t->ctxt[SYSCALL], sizeof(t->ctxt[SYSCALL]));
    
    memcpy(&t->ctxt[CURRENT], &t->proc->ctxt[CURRENT], sizeof(struct context));

    // Set up stack pointers - ensure proper alignment with more space
    unsigned long ssp = ((unsigned long)t->stack_top - 128) & ~MASKBITS;
    unsigned long usp = ((unsigned long)t->stack_top - 256) & ~MASKBITS;
    
    TRACE_THREAD("Stack pointers: SSP=%lx, USP=%lx", ssp, usp);
    
    // Store function and argument in thread structure
    t->func = func;
    t->arg = arg;
    
    // Set up initial context
    t->ctxt[CURRENT].ssp = ssp;
    t->ctxt[CURRENT].usp = usp;
    t->ctxt[CURRENT].pc = (unsigned long)proc_thread_start;
    t->ctxt[CURRENT].sr = 0x2000;  // Supervisor mode (0x2000)
    
    // Store thread pointer in D0 register
    t->ctxt[CURRENT].regs[0] = (unsigned long)t;
    
    // Create a proper exception frame for RTE
    unsigned short *frame_ptr = (unsigned short *)(ssp - 8);
    frame_ptr[0] = 0x0000;  // Format/Vector
    frame_ptr[1] = 0x2000;  // SR (Supervisor mode)
    frame_ptr[2] = (unsigned short)((unsigned long)proc_thread_start >> 16);
    frame_ptr[3] = (unsigned short)((unsigned long)proc_thread_start);
    
    // Update SSP to point to our exception frame
    t->ctxt[CURRENT].ssp = (unsigned long)frame_ptr;
    
    // Copy to SYSCALL context
    memcpy(&t->ctxt[SYSCALL], &t->ctxt[CURRENT], sizeof(struct context));

    memcpy(&t->ctxt[SYSCALL].crp, &t->proc->ctxt[SYSCALL].crp, sizeof(t->ctxt[SYSCALL].crp));
    memcpy(&t->ctxt[SYSCALL].tc, &t->proc->ctxt[SYSCALL].tc, sizeof(t->ctxt[SYSCALL].tc));    

    TRACE_THREAD("INIT CONTEXT: Thread id. %d initialized for process %d",t->tid, t->proc->pid);
    TRACE_THREAD("Exception frame created at %p:", frame_ptr);
    TRACE_THREAD("  SR = %04x (supervisor mode)", frame_ptr[1]);
    TRACE_THREAD("  PC = %04x%04x", frame_ptr[2], frame_ptr[3]);
    TRACE_THREAD("  Thread pointer stored in D0 = %p", t);     
}

static void init_main_thread_context(struct proc *p) {
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
    t0->priority = 1;
    t0->original_priority = 1;

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

    for (int i = 0; i < NSIG; i++) {
        t0->sig_handlers[i].handler = NULL;
        t0->sig_handlers[i].arg = NULL;
    }
	t0->mutex_wait_obj = NULL;
	t0->sem_wait_obj = NULL;
	t0->sig_wait_obj = NULL;
	t0->cond_wait_obj = NULL;
	t0->join_wait_obj = NULL;
    t0->wait_type = WAIT_NONE;  // Not waiting for anything initially
    t0->sleep_reason = 0;  // No sleep reason initially
    
    // Initialize join-related fields
    t0->retval = NULL;
    t0->joiner = NULL;
    t0->detached = 0;  // Default is joinable
    t0->joined = 0;

    /* Use process TSD data for thread0 */
    t0->tsd_data = p->proc_tsd_data;

    p->threads = t0;
    p->current_thread = t0;
    p->num_threads = 1;
    p->total_threads = 1;

    atomic_thread_state_change(t0, THREAD_STATE_RUNNING);
    TRACE_THREAD("INIT CONTEXT: Thread id. %d initialized for process %d",t0->tid, p->pid);
    TRACE_THREAD("INIT CONTEXT: Starting thread timer for process %d", p->pid);
    thread_timer_start(t0->proc, t0->tid);
}

CONTEXT* get_thread_context(struct thread *t) {
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

    // If thread is handling a signal, return the signal context
    if (t->t_sig_in_progress) {
        TRACE_THREAD("GET_CTX: Using signal context for thread %d", t->tid);
        return &t->sig_ctx;
    }

    TRACE_THREAD("GET_CTX: Using current context for thread %d", t->tid);
    return &t->ctxt[CURRENT];
}

void proc_thread_cleanup_process(struct proc *pcurproc) {
    /* Clean up threads if any exist */
    if (pcurproc->threads) {
        TRACE(("terminate: cleaning up threads for pid=%d", pcurproc->pid));

        /* 1. FIRST: Stop the thread timer to prevent scheduling during cleanup */
        if (pcurproc->p_thread_timer.enabled) {
            TRACE(("terminate: stopping thread timer"));
            thread_timer_stop(pcurproc);
        }

        /* 2. Cancel all timeouts for this process EARLY */
        TIMEOUT *timelist, *next_timelist;
        for (timelist = tlist; timelist; timelist = next_timelist) {
            next_timelist = timelist->next;
            if (timelist->proc == pcurproc) {
                TRACE(("terminate: cancelling timeout for pid=%d", pcurproc->pid));
                canceltimeout(timelist);
            }
        }

        /* 3. Remove all threads from queues BEFORE clearing sync states */
        struct thread *t;
        for (t = pcurproc->threads; t; t = t->next) {
            if (t->magic == CTXT_MAGIC) {
                TRACE(("terminate: removing thread %d from queues", t->tid));
                remove_thread_from_wait_queues(t);
                remove_from_ready_queue(t);
                /* Mark as exited but don't free yet */
                t->state |= THREAD_STATE_EXITED;
            }
        }
        // Explicitly clean up idle thread
        if (pcurproc->idle_thread) {
            TRACE(("terminate: cleaning up idle thread"));
            cleanup_thread_resources(pcurproc, pcurproc->idle_thread, pcurproc->idle_thread->tid);
            pcurproc->idle_thread = NULL;
        }
        
        /* 4. NOW clean up sync states (threads are out of queues) */
        cleanup_thread_sync_states(pcurproc);

        /* 5. Free individual threads */
        t = pcurproc->threads;
        struct thread *next;
        while (t) {
            next = t->next;
            
            if (t->magic == CTXT_MAGIC) {
                TRACE(("terminate: freeing thread %d resources", t->tid));
                /* Free thread resources (includes individual TSD) */
                cleanup_thread_resources(pcurproc, t, t->tid);
            }
            t = next;
        }

        /* 6. Clean up process-wide thread state */
        pcurproc->current_thread = NULL;
        pcurproc->num_threads = 0;
        pcurproc->total_threads = 0;
        pcurproc->threads = NULL;

        /* 7. LAST: Clean up process-wide TSD */
        cleanup_proc_tsd(pcurproc);
    }
}

long proc_thread_status(long tid) {
    struct proc *p = curproc;
    struct thread *target = NULL;
    unsigned short sr;
    
    if (!p)
        return -EINVAL;
    
    // Find target thread
    sr = splhigh();
    for (target = p->threads; target; target = target->next) {
        if (target->tid == tid) {
            break;
        }
    }
    
    if (!target) {
        spl(sr);
        return -ESRCH;  // Thread not found
    }
    
    long status = target->state;
    spl(sr);
    
    return status;
}

/*
 * Idle thread function - just loops forever
 */
static void *idle_thread_func(void *arg) {
    struct proc *p = (struct proc *)arg;
    
    if (!p || p->magic != CTXT_MAGIC) {
        TRACE_THREAD("IDLE: Invalid process pointer");
        return NULL;
    }
    p->pri = p->pri + 1;

    while (1) {
        asm volatile("nop");
        proc_thread_schedule();
    }

    // volatile unsigned long current_time = get_system_ticks();
    // volatile unsigned long end_time = current_time + 50;

    // while (current_time < end_time) {
    //     asm volatile("nop");
    //     current_time = get_system_ticks();
    // }

    /* Restore original process priority */
    p->pri = p->pri - 1;

    return NULL;
}

static struct thread* create_idle_thread(struct proc *p) {
    // Create a new idle thread
    struct thread *idle = kmalloc(sizeof(struct thread));
    if (!idle) return NULL;
    
    // Initialize the idle thread
    mint_bzero(idle, sizeof(*idle));

    idle->tid = -128;  // Negative tid to indicate idle thread
    // Don't increment p->num_threads for idle thread
    idle->proc = p;
    idle->priority = 0;  // Lowest possible priority
    idle->original_priority = 0;
    idle->is_idle = 1;      // Mark as idle thread
    idle->magic = CTXT_MAGIC;

    // Allocate stack
    idle->stack = kmalloc(STKSIZE);
    if (!idle->stack) {
        kfree(idle);
        return NULL;
    }
    idle->stack_top = (char*)idle->stack + STKSIZE;
    idle->stack_magic = STACK_MAGIC;
    
    idle->policy = DEFAULT_SCHED_POLICY;
    idle->timeslice = p->thread_default_timeslice;
    idle->remaining_timeslice = p->thread_default_timeslice;
    idle->last_scheduled = 0;
    idle->priority_boost = 0;

    idle->tsd_data = NULL;

    idle->t_sigpending = 0;
    THREAD_SIGMASK_SET(idle, p->p_sigmask);  /* Inherit process signal mask */
    idle->t_sig_in_progress = 0;
    idle->alarm_timeout = NULL;
    idle->sig_stack = NULL;
    idle->old_sigmask = 0;
    for (int i = 0; i < NSIG; i++) {
        idle->sig_handlers[i].handler = NULL;
        idle->sig_handlers[i].arg = NULL;
    }
    
    idle->wakeup_time = 0;  // No wakeup time initially
    idle->next_sleeping = NULL;  // Not in sleep queue initially
    idle->next_ready = NULL;  // Not in ready queue initially

	idle->mutex_wait_obj = NULL;
	idle->sem_wait_obj = NULL;
	idle->sig_wait_obj = NULL;
	idle->cond_wait_obj = NULL;
	idle->join_wait_obj = NULL;
    idle->wait_type = WAIT_NONE;  // Not waiting for anything initially
    idle->sleep_reason = 0;  // No sleep reason initially
    
    // Initialize join-related fields
    idle->retval = NULL;
    idle->joiner = NULL;
    idle->detached = 1;
    idle->joined = 0;

    // Initialize context
    init_thread_context(idle, idle_thread_func, (void *)p);
    
    /* Set one idle thread per process */
    p->idle_thread = idle;

    // Link into process thread list
    idle->next = p->threads;
    p->threads = idle;

    atomic_thread_state_change(idle, THREAD_STATE_READY);
    add_to_ready_queue(idle);

    TRACE_THREAD("IDLE: Created idle thread with tid %d", idle->tid);

    return idle;
};

struct thread* get_idle_thread(struct proc *p) {
    if (!p) return NULL;
    
    if(!p->idle_thread)
        return create_idle_thread(p);
        
    return p->idle_thread;
}

struct thread* get_main_thread(struct proc *p) {
    struct thread *thread0 = NULL;
    for (struct thread *t = p->threads; t != NULL; t = t->next) {
        if (t->tid == 0 && t->magic == CTXT_MAGIC && !(t->state & THREAD_STATE_EXITED)) {
            thread0 = t;
            break;
        }
    }
    return thread0;
}
/* End of Threads stuff */