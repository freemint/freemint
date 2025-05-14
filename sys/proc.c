/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * routines for handling processes
 *
 */

# include "proc.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/credentials.h"
# include "mint/filedesc.h"
# include "mint/basepage.h"
# include "mint/resource.h"
# include "mint/signal.h"

# include "arch/context.h"	/* save_context, change_context */
# include "arch/kernel.h"
# include "arch/mprot.h"
# include "arch/tosbind.h"
# include "arch/user_things.h"	/* trampoline */

# include "bios.h"
# include "cookie.h"
# include "dosfile.h"
# include "filesys.h"
# include "k_exit.h"
# include "kmemory.h"
# include "memory.h"
# include "proc_help.h"
# include "proc_wakeup.h"
# include "random.h"
# include "signal.h"
# include "time.h"
# include "timeout.h"
# include "random.h"
# include "xbios.h"


/*
 * We initialize proc_clock to a very large value so that we don't have
 * to worry about unexpected process switches while starting up
 */
unsigned short proc_clock = 0x7fff;

struct proc_queue sysq[NUM_QUEUES] = { { NULL } };


/* global process variables */
struct proc *proclist = NULL;		/* list of all active processes */
struct proc *curproc  = NULL;	/* current process		*/
struct proc *rootproc = NULL;		/* pid 0 -- MiNT itself		*/

/* default; actual value comes from mint.cnf */
short time_slice = 2;

struct proc *_cdecl get_curproc(void) { return curproc; }

/* Threads stuff */

// #define WITH_THTIMER 1

void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);
void thread_switch(struct thread *from, struct thread *to);
long create_thread(struct proc *p, void (*func)(void*), void *arg);
void init_thread_context(struct thread *t, void (*func)(void*), void *arg);
void thread_start(void);
void schedule(void);
void thread_exit(void);
void init_thread0(struct proc *p);
void thread_timer_init(void);
void thread_preempt_handler(PROC *p, long arg);
void thread_timer_start(struct proc *p, int thread_id);
void thread_timer_stop(void);
void start_thread_timing(struct thread *t);

void thread_switch_timeout_handler(PROC *p, long arg);
void reset_thread_switch_state(void);
int check_thread_switch_timeout(void);

void atomic_thread_state_change(struct thread *t, int new_state);

struct thread *ready_queue = NULL;
static int thread_switch_in_progress = 0;
static int timer_operation_in_progress = 0;  /* Flag to prevent concurrent timer operations */
static TIMEOUT *thread_switch_timeout = NULL;
static struct thread *switching_from = NULL;
static struct thread *switching_to = NULL;
static unsigned long switch_ticks = 0;

#define THREAD_SWITCH_TIMEOUT_MS 500  /* 500ms timeout for thread switches */
#define MAX_SWITCH_RETRIES 3

/* Thread timer state */
static struct {
    struct proc *proc;               /* Process being timed */
    int thread_id;                   /* Current thread ID */
    int enabled;                     /* Timer enabled flag */
    TIMEOUT *timeout;                /* Current timeout handle */
    unsigned short sr;               /* Saved interrupt state */
    int in_handler;                  /* Flag to prevent reentrance */
} thread_timer = { NULL, 0, 0, NULL, 0, 0 };

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
        
    sr = splhigh();
    TRACE_THREAD("Thread %d state change: %d -> %d", t->tid, t->state, new_state);
    t->state = new_state;
    spl(sr);
}

/*
 * Initialize the thread timer
 * Called during system initialization
 */
void thread_timer_init(void)
{
    thread_timer.proc = NULL;
    thread_timer.thread_id = 0;
    thread_timer.enabled = 0;
    thread_timer.timeout = NULL;
    thread_timer.in_handler = 0;
    
	timer_operation_in_progress = 0;
    DEBUG(("Thread timer initialized"));
}

/*
 * Start timing a specific process/thread
 */
void thread_timer_start(struct proc *p, int thread_id)
{
    unsigned short sr, retry_count = 0;
    
    if (!p)
        return;

retry_start:		
    sr = splhigh();

    /* Check if another timer operation is in progress */
    if (timer_operation_in_progress) {
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
    
    timer_operation_in_progress = 1;
    	
    /* If timer is already enabled, don't add another timeout */
    if (thread_timer.enabled && thread_timer.timeout) {
		timer_operation_in_progress = 0;
        spl(sr);
        return;
    }
    
    thread_timer.proc = p;
    thread_timer.thread_id = thread_id;
    thread_timer.sr = sr;
    
    /* Add the initial timeout */
    thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
    
    TRACE_THREAD("Setting up preemption timer for thread %d", thread_id);

    /* Set enabled flag last to ensure everything is set up */
    thread_timer.enabled = 1;	
    spl(sr);
}

/*
 * Stop the thread timer
 */
void thread_timer_stop(void)
{
    unsigned short sr, retry_count = 0;
    TIMEOUT *timeout_to_cancel = NULL;
		 
retry_stop:
    sr = splhigh();
    
    /* Check if another timer operation is in progress */
    if (timer_operation_in_progress) {
        spl(sr);
        
        /* Simple retry mechanism with limit */
        if (++retry_count > 5) {
            TRACE_THREAD("WARNING: Failed to stop timer after 5 retries");
            return;
        }
        
        /* Small delay before retry */
        {
            volatile int i;
            for (i = 0; i < 100; i++);
        }
        
        goto retry_stop;
    }
    
    timer_operation_in_progress = 1;
    
    /* Check if timer is already disabled */
    
    thread_timer.enabled = 0;
    
    /* Save the timeout pointer locally before canceling */
    if (thread_timer.timeout) {
        timeout_to_cancel = thread_timer.timeout;
        thread_timer.timeout = NULL;
    }

    /* Release the lock before calling canceltimeout */
    timer_operation_in_progress = 0;
    spl(sr);
    
    /* Now cancel the timeout if we had one */
    if (timeout_to_cancel) {
        canceltimeout(timeout_to_cancel);
    }

    spl(sr);
}

/*
 * Function to start timing a newly created thread
 * Call this after thread creation
 */
void start_thread_timing(struct thread *t)
{
    if (!t || !t->proc)
        return;
        
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
    
    /* Prevent reentrance */
    if (thread_timer.in_handler) {
        /* Re-add the timeout and return */
        if (!thread_timer.enabled) {
            /* Timer was disabled while we were waiting to enter handler */
            return;
        }		
        thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        return;
    }
    
    thread_timer.in_handler = 1;
    
    /* Clear the timeout pointer since this timeout has fired */
    thread_timer.timeout = NULL;
    
    /* If timer is not enabled, just return without re-adding the timeout */
	sr = splhigh();
    if (!thread_timer.enabled) {
        thread_timer.in_handler = 0;
        return;
    }
    
    /* Get current process - IMPORTANT: Don't use the passed-in process pointer */
    current = curproc;
    if (!current) {
        /* No current process, re-add the timeout and return */
		spl(sr);
        thread_timer.timeout = addtimeout(p, 20, thread_preempt_handler);
        thread_timer.in_handler = 0;
        return;
    }
    
    /* Only preempt if process has multiple threads */
    if (current->num_threads > 1 && current->current_thread) {
        /* Check if there are threads in the ready queue */
		spl(sr);
        if (!ready_queue) {
            thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            thread_timer.in_handler = 0;
            return;
        }
        
        /* Don't preempt if a thread switch is already in progress */
        if (thread_switch_in_progress) {
			spl(sr);
            thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            thread_timer.in_handler = 0;
            return;
        }
        
        /* Get the current thread */
        struct thread *curr_thread = current->current_thread;
        
        /* Find next thread to run */
        struct thread *next = ready_queue;
        
        /* If next is the same as current, try to find another thread */
        if (next == curr_thread && next->next_ready) {
            next = next->next_ready;
        }
        
        if (next == curr_thread) {
            spl(sr);
            thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
            thread_timer.in_handler = 0;
            return;
        }
        
        /* Remove next thread from ready queue */
        remove_from_ready_queue(next);
        
        /* Add current thread to ready queue */
        atomic_thread_state_change(curr_thread, THREAD_READY);
        add_to_ready_queue(curr_thread);
        
        /* Update thread states */
        atomic_thread_state_change(next, THREAD_RUNNING);
        current->current_thread = next;
        
        /* Perform context switch */
        thread_switch(curr_thread, next);
        
        spl(sr);
    }
    
    /* Re-add the timeout for next preemption */
    thread_timer.timeout = addtimeout(current, 20, thread_preempt_handler);
    thread_timer.in_handler = 0;
}

// Ready queue management
void add_to_ready_queue(struct thread *t) {
    unsigned short sr;
    
    if (!t || t->state == THREAD_RUNNING) return;
    
    sr = splhigh();
    
    // Check if already in queue
    struct thread *curr = ready_queue;
    while (curr) {
        if (curr == t) {
            TRACE_THREAD("Thread %d already in ready queue", t->tid);
            spl(sr);
            return;  // Already in queue, don't add again
        }
        curr = curr->next_ready;
    }
    
    atomic_thread_state_change(t, THREAD_READY);
    TRACE_THREAD("ADD_TO_READY_Q: Thread %d (pri %d, state %d)", 
                t->tid, t->priority, t->state);
    
    // Add to end of queue to maintain fairness
    if (!ready_queue) {
        ready_queue = t;
        t->next_ready = NULL;
    } else {
        struct thread *last = ready_queue;
        while (last->next_ready) {
            last = last->next_ready;
        }
        last->next_ready = t;
        t->next_ready = NULL;
    }
    
    spl(sr);
}

void remove_from_ready_queue(struct thread *t) {
    unsigned short sr;
    
    if (!t || !ready_queue) return;
    
    sr = splhigh();
    
    TRACE_THREAD("REMOVE_FROM_READY_Q: Thread %d", t->tid);
    
    // Use a double pointer to traverse the list
    // This allows us to modify the pointer that points to the current node
    struct thread **pp = &ready_queue;
    struct thread *curr = ready_queue;
    int found = 0;
    
    // Search through the entire queue
    while (curr) {
        if (curr == t) {
            // Found the thread, remove it from the queue
            *pp = curr->next_ready;
            curr->next_ready = NULL;
            found = 1;
            TRACE_THREAD("Removed thread %d from ready queue", t->tid);
            break;
        }
        // Move to next node
        pp = &(curr->next_ready);
        curr = curr->next_ready;
    }
    
    if (!found) {
        // Thread wasn't in the queue
        TRACE_THREAD("Thread %d not found in ready queue", t->tid);
    }
    
    spl(sr);
}

// Thread context switching
void thread_switch(struct thread *from, struct thread *to) {
    if (!to || from == to) {
        TRACE_THREAD("Invalid thread switch request");
        return;
    }
    
    // Disable interrupts during context switch
    unsigned short sr = splhigh();
    
    // Check if another thread switch is in progress
    if (thread_switch_in_progress) {
        TRACE_THREAD("Thread switch already in progress, aborting");
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
    struct thread *next = ready_queue;
    
    TRACE_THREAD("schedule: current=%d ready_queue=%p", 
                current ? current->tid : -1, ready_queue);

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
    
    TRACE_THREAD("Thread %d exiting", current->tid);
    
    unsigned short sr = splhigh();
    
    // Prevent other threads from exiting simultaneously
    static int thread_exit_in_progress = 0;
    static int exit_owner_tid = -1;
    
    // If another thread is exiting, wait unless it's us (recursive call)
    if (thread_exit_in_progress && exit_owner_tid != current->tid) {
        // Wait briefly and retry
        spl(sr);
        TRACE_THREAD("Thread exit already in progress by thread %d, waiting", exit_owner_tid);
        
        // Simple delay
        volatile int i;
        for (i = 0; i < 1000; i++);
        
        // Try again after delay
        thread_exit();
        return;
    }
    
    // Mark this thread as the one performing exit
    thread_exit_in_progress = 1;
    exit_owner_tid = current->tid;
    
    // Save the thread ID for logging
    int tid = current->tid;
    
    // Mark thread as exited and remove from ready queue
    atomic_thread_state_change(current, THREAD_EXITED);
    remove_from_ready_queue(current);
    
    // Cancel any timeouts associated with this thread
    if (current->tid == thread_timer.thread_id && thread_timer.enabled) {
        thread_timer_stop();
    }
    
    // Find another thread to run
    struct thread *next = ready_queue;
    
    if (next) {
        // Save context information
        CONTEXT saved_context;
        memcpy(&saved_context, &next->ctxt[CURRENT], sizeof(CONTEXT));
        
        // Remove from ready queue
        remove_from_ready_queue(next);
        
        // Update thread states
        atomic_thread_state_change(next, THREAD_RUNNING);
        p->current_thread = next;
        
        TRACE_THREAD("Switching to thread %d", next->tid);
        
        // Set USP for the new thread
        TRACE_THREAD("Setting USP to %lx", next->ctxt[CURRENT].usp);
        asm volatile (
            "move.l %0,%%a0\n\t"
            "move.l %%a0,%%usp"
            : 
            : "r" (next->ctxt[CURRENT].usp)
            : "a0"
        );
        
        // Detach thread from process's thread list
        struct thread **tp;
        for (tp = &p->threads; *tp; tp = &(*tp)->next) {
            if (*tp == current) {
                *tp = current->next;
                break;
            }
        }
        
        // Decrement thread count
        p->num_threads--;
        
        // Free thread resources
        TRACE_THREAD("Freeing stack for thread %d", tid);
        if (current->stack && tid != 0) {  // Don't free thread0's stack
            kfree(current->stack);
            current->stack = NULL;  // Prevent double-free
        }
        
        // Free the thread structure
        TRACE_THREAD("Freeing thread %d structure", tid);
        current->magic = 0;  // Clear magic to prevent reuse
        kfree(current);
        
        // Reset exit in progress flag
        thread_exit_in_progress = 0;
        exit_owner_tid = -1;
        
        // Perform context switch - this should never return
        change_context(&saved_context);
        
        // If we somehow get here, something went wrong
        TRACE_THREAD("ERROR: Returned from change_context!");
        spl(sr);
        return;
    } else {
        // No other threads to run
        TRACE_THREAD("No other threads to run after thread exit");
        
        // If this is thread0, we need to handle it specially
        if (tid == 0) {
            TRACE_THREAD("Thread0 exiting, returning to process");
            
            // Detach thread from process's thread list
            struct thread **tp;
            for (tp = &p->threads; *tp; tp = &(*tp)->next) {
                if (*tp == current) {
                    *tp = current->next;
                    break;
                }
            }
            
            // Decrement thread count
            p->num_threads--;
            
            // Clear current thread
            p->current_thread = NULL;
            
            // Free the thread structure
            TRACE_THREAD("Freeing thread %d structure", tid);
            current->magic = 0;  // Clear magic to prevent reuse
            kfree(current);
            
            // Reset exit in progress flag
            thread_exit_in_progress = 0;
            exit_owner_tid = -1;
            
            spl(sr);
            return;
        }
        
        // For other threads, we need to return to the main process
        TRACE_THREAD("No other threads to run, returning to main process");
        
        // Save the process context
        CONTEXT saved_proc_context;
        memcpy(&saved_proc_context, &p->ctxt[CURRENT], sizeof(CONTEXT));
        
        // Detach thread from process's thread list
        struct thread **tp;
        for (tp = &p->threads; *tp; tp = &(*tp)->next) {
            if (*tp == current) {
                *tp = current->next;
                break;
            }
        }
        
        // Decrement thread count
        p->num_threads--;
        
        // Clear current thread
        p->current_thread = NULL;
        
        // Free thread resources
        TRACE_THREAD("Freeing stack for thread %d", tid);
        if (current->stack && tid != 0) {  // Don't free thread0's stack
            kfree(current->stack);
            current->stack = NULL;  // Prevent double-free
        }
        
        // Free the thread structure
        TRACE_THREAD("Freeing thread %d structure", tid);
        current->magic = 0;  // Clear magic to prevent reuse
        kfree(current);
        
        // Reset exit in progress flag
        thread_exit_in_progress = 0;
        exit_owner_tid = -1;
        
        // Return to the main process context
        TRACE_THREAD("Returning to main process context");
        change_context(&saved_proc_context);
        
        // If we somehow get here, something went wrong
        TRACE_THREAD("ERROR: Returned from change_context to main process!");
    }
    
    // Reset exit in progress flag in case we somehow get here
    thread_exit_in_progress = 0;
    exit_owner_tid = -1;
    
    spl(sr);
    
    // We should never reach here
    TRACE_THREAD("ERROR: Thread %d returned after exit!", tid);
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
    if (!ready_queue) {
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
    struct thread *next = ready_queue;
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
        next = ready_queue;
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

/* End of Threads stuff */

/*
 * initialize the process table
 */
void
init_proc(void)
{
	static DTABUF dta;

	static struct proc	rootproc0;
	static struct memspace	mem0;
	static struct ucred	ucred0;
	static struct pcred	pcred0;
	static struct filedesc	fd0;
	static struct cwd	cwd0;
	static struct sigacts	sigacts0;
	static struct plimit	limits0;
	
	#ifdef WITH_THTIMER
	// Initialize thread timer
	thread_timer_init();
	#endif

	mint_bzero(&sysq, sizeof(sysq));

	/* XXX */
	mint_bzero(&rootproc0, sizeof(rootproc0));
	mint_bzero(&mem0, sizeof(mem0));
	mint_bzero(&ucred0, sizeof(ucred0));
	mint_bzero(&pcred0, sizeof(pcred0));
	mint_bzero(&fd0, sizeof(fd0));
	mint_bzero(&cwd0, sizeof(cwd0));
	mint_bzero(&sigacts0, sizeof(sigacts0));
	mint_bzero(&limits0, sizeof(limits0)); 

	pcred0.ucr = &ucred0;			ucred0.links = 1;

	rootproc0.p_mem		= &mem0;	mem0.links = 1;
	rootproc0.p_cred	= &pcred0;	pcred0.links = 1;
	rootproc0.p_fd		= &fd0;		fd0.links = 1;
	rootproc0.p_cwd		= &cwd0;	cwd0.links = 1;
	rootproc0.p_sigacts	= &sigacts0;	sigacts0.links = 1;
//	rootproc0.p_limits	= &limits0;	limits0.links = 1;

	fd0.ofiles = fd0.dfiles;
	fd0.ofileflags = (unsigned char *)fd0.dfileflags;
	fd0.nfiles = NDFILE;

	DEBUG(("init_proc() inf : %p, %p, %p, %p, %p, %p, %p",
		&rootproc0, &mem0, &pcred0, &ucred0, &fd0, &cwd0, &sigacts0));

	curproc = rootproc = &rootproc0;
	rootproc0.links = 1;
	
	
	/* set the stack barrier */
	rootproc->stack_magic = STACK_MAGIC;

	rootproc->ppid = -1;		/* no parent */
	rootproc->p_flag = P_FLAG_SYS;
	rootproc->domain = DOM_TOS;	/* TOS domain */
	rootproc->sysstack = (long)(rootproc->stack + STKSIZE - 12);
	rootproc->magic = CTXT_MAGIC;

	((long *) rootproc->sysstack)[1] = FRAME_MAGIC;
	((long *) rootproc->sysstack)[2] = 0;
	((long *) rootproc->sysstack)[3] = 0;

	rootproc->p_fd->dta = &dta;	/* looks ugly */
	strcpy(rootproc->name, "MiNT");
	strcpy(rootproc->fname, "MiNT");
	strcpy(rootproc->cmdlin, "MiNT");

	/* get some memory */
	rootproc->p_mem->memflags = F_PROT_S; /* default prot mode: super-only */
	rootproc->p_mem->num_reg = NUM_REGIONS;
	{
		union { char *c; void *v; } ptr;
		unsigned long size = rootproc->p_mem->num_reg * sizeof(void *);
		ptr.v = kmalloc(size * 2);
		/* make sure kmalloc was successful */
		assert(ptr.v);
		rootproc->p_mem->mem = ptr.v;
		rootproc->p_mem->addr = (void *)(ptr.c + size);
		/* make sure it's filled with zeros */
		mint_bzero(ptr.c, size * 2L); 
	}
	rootproc->p_mem->base = _base;
	
	/* init trampoline things */
	rootproc->p_mem->tp_ptr = &kernel_things;
	rootproc->p_mem->tp_reg = NULL;

	/* init page table for curproc */
	init_page_table_ptr(rootproc->p_mem);
	init_page_table(rootproc, rootproc->p_mem);

	/* get root and current directories for all drives */
	{
		FILESYS *fs;
		int i;

		for (i = 0; i < NUM_DRIVES; i++)
		{
			fcookie dir;

			fs = drives[i];
			if (fs && xfs_root(fs, i, &dir) == E_OK)
			{
				rootproc->p_cwd->root[i] = dir;
				dup_cookie(&rootproc->p_cwd->curdir[i], &dir);
			}
			else
			{
				rootproc->p_cwd->root[i].fs = rootproc->p_cwd->curdir[i].fs = 0;
				rootproc->p_cwd->root[i].dev = rootproc->p_cwd->curdir[i].dev = i;
			}
		}
	}

	/* Set the correct drive. The current directory we
	 * set later, after all file systems have been loaded.
	 */
	rootproc->p_cwd->curdrv = TRAP_Dgetdrv();
	proclist = rootproc;

	rootproc->p_cwd->cmask = 0;

	/* some more protection against job control; unless these signals are
	 * re-activated by a shell that knows about job control, they'll have
	 * no effect
	 */
	SIGACTION(rootproc, SIGTTIN).sa_handler = SIG_IGN;
	SIGACTION(rootproc, SIGTTOU).sa_handler = SIG_IGN;
	SIGACTION(rootproc, SIGTSTP).sa_handler = SIG_IGN;

	/* set up some more per-process variables */
	rootproc->started = xtime;

	if (has_bconmap)
		/* init_xbios not happened yet */
		rootproc->p_fd->bconmap = (int) TRAP_Bconmap(-1);
	else
		rootproc->p_fd->bconmap = 1;
	rootproc->logbase = (void *) TRAP_Logbase();
	rootproc->criticerr = *((long _cdecl (**)(long)) 0x404L);

	/* Initialize thread-related fields for root process */
	rootproc->current_thread = NULL;
	rootproc->threads = NULL;
	rootproc->num_threads = 0;
}

/* remaining_proc_time():
 *
 * this function returns the numer of milliseconds remaining to
 * the normal preemption. It may be useful for drivers of devices,
 * which do not generate interrupts (Falcon IDE for example).
 * Such a device must give the CPU up from time to time, while
 * looping.
 *
 * Actually reading the proc_clock directly would be much simpler,
 * but doing it so we retain compatibility if we ever resize the
 * proc_clock variable to long or increase its granularity
 * (its actually 50 Hz).
 *
 */
unsigned long _cdecl
remaining_proc_time(void)
{
	unsigned long proc_ms = proc_clock;

	proc_ms *= 20; /* one tick is 20 ms */

	return proc_ms;
}

/* reset_priorities():
 *
 * reset all process priorities to their base level
 * called once per second, so that cpu hogs can get _some_ time
 * slices :-).
 */
void
reset_priorities(void)
{
	struct proc *p;

	for (p = proclist; p; p = p->gl_next)
	{
		if (p->slices >= 0)
		{
			p->curpri = p->pri;
			p->slices = SLICES(p->curpri);
		}
	}
}

/* run_next(p, slices):
 *
 * schedule process "p" to run next, with "slices" initial time slices;
 * "p" does not actually start running until the next context switch
 */
void
run_next(struct proc *p, int slices)
{
	register unsigned short sr = splhigh();

	p->slices = -slices;
	p->curpri = MAX_NICE;
	p->wait_q = READY_Q;
	p->q_next = sysq[READY_Q].head;
	sysq[READY_Q].head = p;
	if (!p->q_next)
		sysq[READY_Q].tail = p;
	else
		p->q_next->q_prev = p;
	p->q_prev = NULL;

	spl(sr);
}

/* fresh_slices(slices):
 *
 * give the current process "slices" more slices in which to run
 */
void
fresh_slices(int slices)
{
	reset_priorities();
	curproc->slices = 0;
	curproc->curpri = MAX_NICE + 1;
	proc_clock = time_slice + slices;
}

/*
 * add a process to a wait (or ready) queue.
 *
 * processes go onto a queue in first in-first out order
 */
void
add_q(int que, struct proc *proc)
{
	/* "proc" should not already be on a list */
	assert(proc->wait_q == 0);
	assert(proc->q_next == 0);

	if (sysq[que].tail) {
		proc->q_prev = sysq[que].tail;
		sysq[que].tail->q_next = proc;
	} else {
		proc->q_prev = NULL;
		sysq[que].head = proc;
	}
	sysq[que].tail = proc;
	proc->wait_q = que;
	if (que != READY_Q && proc->slices >= 0) {
		proc->curpri = proc->pri;	/* reward the process */
		proc->slices = SLICES(proc->curpri);
	}
}

/*
 * remove a process from a queue
 */
void
rm_q(int que, struct proc *proc)
{
	assert(proc->wait_q == que);

	if (proc->q_prev)
		proc->q_prev->q_next = proc->q_next;
	else
		sysq[que].head = proc->q_next;

	if (proc->q_next)
		proc->q_next->q_prev = proc->q_prev;
	else {
		if ((sysq[que].tail = proc->q_prev))
			proc->q_prev->q_next = NULL;
	}
	proc->wait_q = 0;
	proc->q_next = proc->q_prev = NULL;
}

/*
 * preempt(): called by the vbl routine and/or the trap handlers when
 * they detect that a process has exceeded its time slice and hasn't
 * yielded gracefully. For now, it just does sleep(READY_Q); later,
 * we might want to keep track of statistics or something.
 */

void _cdecl
preempt(void)
{
	assert(!(curproc->p_flag & P_FLAG_SYS));

	if (bconbsiz)
	{
		bflush();
	}
	else
	{
		/* punish the pre-empted process */
		if (curproc->curpri >= MIN_NICE)
			curproc->curpri -= 1;
	}

	sleep(READY_Q, curproc->wait_cond);
}

/*
 * swap_in_curproc(): for all memory regions of the current process swaps
 * in the contents of those regions that have been saved in a shadow region
 */

static void
swap_in_curproc(void)
{
	struct memspace *mem = curproc->p_mem;
	long txtsize = curproc->p_mem->txtsize;
	MEMREGION *m, *shdw, *save;
	int i;

	assert(mem && mem->mem);

	for (i = 0; i < mem->num_reg; i++)
	{
		m = mem->mem[i];
		if (m && m->save)
		{
			save = m->save;
			for (shdw = m->shadow; shdw->save; shdw = shdw->shadow)
				assert (shdw != m);

			assert (m->loc == shdw->loc);

			shdw->save = save;
			m->save = 0;
			if (i != 1 || txtsize == 0)
			{
				quickswap((char *)m->loc, (char *)save->loc, m->len);
			}
			else
			{
				quickswap((char *)m->loc, (char *)save->loc, 256);
				quickswap((char *)m->loc + (txtsize+256), (char *)save->loc + 256, m->len - (txtsize+256));
			}
		}
	}
}

/*
 * sleep(que, cond): put the current process on the given queue, then switch
 * contexts. Before a new process runs, give it a fresh time slice. "cond"
 * is the condition for which the process is waiting, and is placed in
 * curproc->wait_cond
 */

static void
do_wakeup_things(short sr, int newslice, long cond)
{
	/*
	 * check for stack underflow, just in case
	 */
	auto int foo;
	struct proc *p;

	p = curproc;

	if ((sr & 0x700) < 0x500)
	{
		/* skip all this if int level is too high */

		if (p->pid && ((long) &foo) < (long) p->stack + ISTKSIZE + 512)
		{
			ALERT("stack underflow");
			handle_sig(SIGBUS);
		}

		/* see if process' time limit has been exceeded */
		if (p->maxcpu)
		{
			if (p->maxcpu <= p->systime + p->usrtime)
			{
				DEBUG(("cpu limit exceeded"));
				raise(SIGXCPU);
			}
		}

		/* check for alarms and similar time out stuff */
		checkalarms();

		if (p->sigpending && cond != (long) sys_pwaitpid)
			/* check for signals */
			check_sigs();

		/* check for proc specific wakeup things */
		checkprocwakeup(p);
	}

	/* Kludge: restore the cookie jar pointer. If this to be restored,
	 * this means that the process has changed it directly, not through
	 * Setexc(). We don't like that.
	 */
# ifdef JAR_PRIVATE
	*CJAR = p->p_mem->tp_ptr->user_jar_p;
# endif

	if (newslice)
	{
		if (p->slices >= 0)
		{
			/* get a fresh time slice */
			proc_clock = time_slice;
		}
		else
		{
			/* slices set by run_next */
			proc_clock = time_slice - p->slices;
			p->curpri = p->pri;
		}

		p->slices = SLICES(p->curpri);
	}
}

static long sleepcond, iwakecond;

/*
 * sleep: returns 1 if no signals have happened since our last sleep, 0
 * if some have
 */

int _cdecl
sleep(int _que, long cond)
{
	struct proc *p;
	unsigned short sr;
	short que = _que & 0xff;
	unsigned long onsigs = curproc->nsigs;
	int newslice = 1;

	/* save condition, checkbttys may just wake() it right away ...
	 * note this assumes the condition will never be waked from interrupts
	 * or other than thru wake() before we really went to sleep, otherwise
	 * use the 0x100 bit like select
	 */
	sleepcond = cond;

	/* if there have been keyboard interrupts since our last sleep,
	 * check for special keys like CTRL-ALT-Fx
	 */
	sr = splhigh();
	if ((sr & 0x700) < 0x500)
	{
		/* can't call checkkeys if sleep was called
		 * with interrupts off  -nox
		 */
		spl(sr);
		checkbttys();
		if (kintr)
		{
			checkkeys();
			kintr = 0;
		}

# ifdef DEV_RANDOM
		/* Wake processes waiting for random bytes */
		checkrandom();
# endif

		sr = splhigh();
		if ((curproc->sigpending & ~(curproc->p_sigmask))
			&& curproc->pid && que != ZOMBIE_Q && que != TSR_Q)
		{
			spl(sr);
			check_sigs();
			sleepcond = 0;	/* possibly handled a signal, return */
			sr = splhigh();
		}
	}

	/* kay: If _que & 0x100 != 0 then take curproc->wait_cond != cond as
	 * an indicatation that the wakeup has already happend before we
	 * actually go to sleep and return immediatly.
	 */
	if ((que == READY_Q && !sysq[READY_Q].head)
	    || ((sleepcond != cond || (iwakecond == cond && cond) || (_que & 0x100 && curproc->wait_cond != cond))
		&& (!sysq[READY_Q].head || (newslice = 0, proc_clock))))
	{
		/* we're just going to wake up again right away! */
		iwakecond = 0;

		spl(sr);
		do_wakeup_things(sr, newslice, cond);

		return (onsigs != curproc->nsigs);
	}

	/* unless our time slice has expired (proc_clock == 0) and other
	 * processes are ready...
	 */
	iwakecond = 0;
	if (!newslice)
		que = READY_Q;
	else
		curproc->wait_cond = cond;

	add_q(que, curproc);

	/* alright curproc is on que now... maybe there's an
	 * interrupt pending that will wakeselect or signal someone
	 */
	spl(sr);

	if (!sysq[READY_Q].head)
	{
		/* hmm, no-one is ready to run. might be a deadlock, might not.
		 * first, try waking up any napping processes;
		 * if that doesn't work, run the root process,
		 * just so we have someone to charge time to.
		 */
		wake(SELECT_Q, (long) nap);

		if (!sysq[READY_Q].head)
		{
			sr = splhigh();
			p = rootproc;		/* pid 0 */
			rm_q(p->wait_q, p);
			add_q(READY_Q, p);
			spl(sr);
		}
	}

	/*
	 * Walk through the ready list, to find what process should run next.
	 * Lower priority processes don't get to run every time through this
	 * loop; if "p->slices" is positive, it's the number of times that
	 * they will have to miss a turn before getting to run again
	 *
	 * Loop structure:
	 *	while (we haven't picked anybody)
	 *	{
	 *		for (each process)
	 *		{
	 *			if (sleeping off a penalty)
	 *			{
	 *				decrement penalty counter
	 *			}
	 *			else
	 *			{
	 *				pick this one and break out of
	 *				both loops
	 *			}
	 *		}
	 *	}
	 */

	sr = splhigh();
	p = 0;
	while (!p)
	{
		for (p = sysq[READY_Q].head; p; p = p->q_next)
		{
			if (p->slices > 0)
				p->slices--;
			else
				break;
		}
	}
	/* p is our victim */
	rm_q(READY_Q, p);
	spl(sr);

	if (save_context(&(curproc->ctxt[CURRENT])))
	{
		/*
		 * restore per-process variables here
		 */
		swap_in_curproc();
		do_wakeup_things(sr, 1, cond);

		return (onsigs != curproc->nsigs);
	}

	/*
	 * save per-process variables here
	 */
	curproc->ctxt[CURRENT].regs[0] = 1;
	curproc = p;

	proc_clock = time_slice;			/* fresh time */

	if ((p->ctxt[CURRENT].sr & 0x2000) == 0)	/* user mode? */
		leave_kernel();

	assert(p->magic == CTXT_MAGIC);
	change_context(&(p->ctxt[CURRENT]));

	/* not reached */
	return 0;
}

/*
 * wake(que, cond): wake up all processes on the given queue that are waiting
 * for the indicated condition
 */

INLINE void
do_wake(int que, long cond)
{
	struct proc *p;

top:
	p = sysq[que].head;

	while (p)
	{
		register unsigned short s = splhigh();

		/* check if p is still on the right queue,
		 * maybe an interrupt just woke it...
		 */
		if (p->wait_q != que)
		{
			spl(s);
			goto top;
		}

		/* move to ready queue */
		{
			struct proc *q = p;

			p = p->q_next;

			if (q->wait_cond == cond)
			{
				rm_q(que, q);
				add_q(READY_Q, q);
			}
		}

		spl(s);
	}
}

void _cdecl
wake(int que, long cond)
{
	if (que == READY_Q)
	{
		ALERT("wake: why wake up ready processes??");
		return;
	}

	if (sleepcond == cond)
		sleepcond = 0;

	do_wake(que, cond);
}

/*
 * iwake(que, cond, pid): special version of wake() for IO interrupt
 * handlers and such.  the normal wake() would lose when its
 * interrupt goes off just before a process is calling sleep() on the
 * same condition (similar problem like with wakeselect...)
 *
 * use like this:
 *	static ipid = -1;
 *	static volatile sleepers = 0;	(optional, to save useless calls)
 *	...
 *	device_read(...)
 *	{
 *		ipid = curproc->pid;	(p_getpid() for device drivers...)
 *		while (++sleepers, (not ready for IO...)) {
 *			sleep(IO_Q, cond);
 *			if (--sleepers < 0)
 *				sleepers = 0;
 *		}
 *		if (--sleepers < 0)
 *			sleepers = 0;
 *		ipid = -1;
 *		...
 *	}
 *
 * and in the interrupt handler:
 *	if (sleepers > 0)
 *	{
 *		sleepers = 0;
 *		iwake (IO_Q, cond, ipid);
 *	}
 *
 * caller is responsible for not trying to wake READY_Q or other nonsense :)
 * and making sure the passed pid is always -1 when curproc is calling
 * sleep() for another than the waked que/condition.
 */

void _cdecl
iwake(int que, long cond, short pid)
{
	if (pid >= 0)
	{
		register unsigned short s = splhigh();

		if (iwakecond == cond)
		{
			spl(s);
			return;
		}

		if (curproc->pid == pid && !curproc->wait_q)
			iwakecond = cond;

		spl(s);
	}

	do_wake(que, cond);
}

/*
 * wakeselect(p): wake process p from a select() system call
 * may be called by an interrupt handler or whatever
 */

void _cdecl
wakeselect(struct proc *p)
{
	unsigned short s = splhigh();

	if (p->wait_cond == (long) wakeselect
		|| p->wait_cond == (long) &select_coll)
	{
		p->wait_cond = 0;
	}

	if (p->wait_q == SELECT_Q)
	{
		rm_q(SELECT_Q, p);
		add_q(READY_Q, p);
	}

	spl(s);
}

/*
 * dump out information about processes
 */

/*
 * kludge alert! In order to get the right pid printed by FORCE, we use
 * curproc as the loop variable.
 *
 * I have changed this function so it is more useful to a user, less to
 * somebody debugging MiNT.  I haven't had any stack problems in MiNT
 * at all, so I consider all that stack info wasted space.  -- AKP
 */

# ifdef DEBUG_INFO
static const char *qstring[] =
{
	"run", "ready", "wait", "iowait", "zombie", "tsr", "stop", "select"
};

/* UNSAFE macro for qname, evaluates x 1, 2, or 3 times */
# define qname(x) ((x >= 0 && x < NUM_QUEUES) ? qstring[x] : "unkn")
# endif

unsigned long uptime = 0;
unsigned long avenrun[3] = { 0, 0, 0 };
unsigned short uptimetick = 200;

static unsigned short number_running;

void
DUMPPROC(void)
{
#ifdef DEBUG_INFO
	struct proc *p = curproc;

	FORCE("Uptime: %ld seconds Loads: %ld %ld %ld Processes running: %d",
		uptime,
		(avenrun[0] * 100) / 2048 , (avenrun[1] * 100) / 2048, (avenrun[2] * 100 / 2048),
 		number_running);

	for (curproc = proclist; curproc; curproc = curproc->gl_next)
	{
		FORCE("state %s sys %s, PC: %lx/%lx BP: %p (pgrp %i)",
			qname(curproc->wait_q),
			curproc->p_flag & P_FLAG_SYS ? "yes":" no",
			curproc->ctxt[CURRENT].pc, curproc->ctxt[SYSCALL].pc,
			curproc->p_mem ? curproc->p_mem->base : NULL,
			curproc->pgrp);
	}
	curproc = p;	/* restore the real curproc */
# endif
}

INLINE unsigned long
gen_average(unsigned long *sum, unsigned char *load_ptr, unsigned long max_size)
{
	register long old_load = (long) *load_ptr;
	register long new_load = number_running;

	*load_ptr = (unsigned char) new_load;

	*sum += (new_load - old_load) * LOAD_SCALE;

	return (*sum / max_size);
}

void
calc_load_average(void)
{
	static unsigned char one_min [SAMPS_PER_MIN];
	static unsigned char five_min [SAMPS_PER_5MIN];
	static unsigned char fifteen_min [SAMPS_PER_15MIN];

	static unsigned short one_min_ptr = 0;
	static unsigned short five_min_ptr = 0;
	static unsigned short fifteen_min_ptr = 0;

	static unsigned long sum1 = 0;
	static unsigned long sum5 = 0;
	static unsigned long sum15 = 0;

	register struct proc *p;

# if 0	/* moved to intr.spp */
	uptime++;
	uptimetick += 200;

	if (uptime % 5) return;
# endif

	number_running = 0;

	for (p = proclist; p; p = p->gl_next)
	{
		if (p != rootproc)
		{
			if ((p->wait_q == CURPROC_Q) || (p->wait_q == READY_Q))
				number_running++;
		}

		/* Check the stack magic here, to ensure the system/interrupt
		 * stack hasn't grown too much. Most noticeably, NVDI 5's new
		 * bitmap conversion (vr_transfer_bits()) seems to eat _a lot_
		 * of supervisor stack, that's why the values in proc.h have
		 * been increased.
		 */
		if (p->stack_magic != STACK_MAGIC)
			FATAL("proc %lx has invalid stack_magic %lx", (long) p, p->stack_magic);
	}

	if (one_min_ptr == SAMPS_PER_MIN)
		one_min_ptr = 0;

	avenrun [0] = gen_average(&sum1, &one_min [one_min_ptr++], SAMPS_PER_MIN);

	if (five_min_ptr == SAMPS_PER_5MIN)
		five_min_ptr = 0;

	avenrun [1] = gen_average(&sum5, &five_min [five_min_ptr++], SAMPS_PER_5MIN);

	if (fifteen_min_ptr == SAMPS_PER_15MIN)
		fifteen_min_ptr = 0;

	avenrun [2] = gen_average(&sum15, &fifteen_min [fifteen_min_ptr++], SAMPS_PER_15MIN);
}
