/**
 * @file proc_threads_cancel.c
 * @brief Thread Cancellation Implementation
 * 
 * Implements POSIX-compliant thread cancellation with both asynchronous
 * and deferred cancellation modes. Handles cancellation points and
 * cleanup handlers.
 * 
 * @author Medour Mehdi
 * @date June 2025
 * @version 1.0
 */

#include "proc_threads_cancel.h"
#include "proc_threads_helper.h"
#include "proc_threads_queue.h"
#include "proc_threads_scheduler.h"
#include "proc_threads_signal.h"

/**
 * Check if a thread should be cancelled and perform the cancellation
 * This is the core cancellation checking function that should be called
 * from various points in the kernel.
 * 
 * @param t Thread to check for cancellation
 */
void check_thread_cancellation(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC) return;
    
    // Only check if cancellation is enabled
    if (t->cancel_state != PTHREAD_CANCEL_ENABLE) return;
    
    // Only proceed if cancellation is pending
    if (!t->cancel_pending) return;
    
    register unsigned short sr = splhigh();
    
    // Check if thread is already exiting/exited
    if (t->state & (THREAD_STATE_EXITED | THREAD_STATE_ZOMBIE)) {
        spl(sr);
        return;
    }
    
    TRACE_THREAD("CANCEL: Checking cancellation for thread %d (type=%s)", 
                t->tid, 
                (t->cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) ? "ASYNC" : "DEFERRED");
    
    // For asynchronous cancellation, cancel immediately
    if (t->cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) {
        spl(sr);
        TRACE_THREAD("CANCEL: Asynchronous cancellation of thread %d", t->tid);
        
        // If this is the current thread, exit directly
        if (t == CURTHREAD) {
            proc_thread_exit(PTHREAD_CANCELED, NULL);
        } else {
            // For other threads, mark them for immediate cancellation
            t->cancel_requested = 1;
            
            // Wake them up if sleeping/waiting so they can process the cancellation
            if (t->state & THREAD_STATE_BLOCKED) {
                // Remove from any wait queues
                remove_thread_from_wait_queues(t);
                
                // Clear wait types
                t->wait_type = WAIT_NONE;
                
                // Cancel any timeouts
                if (t->sleep_timeout) {
                    canceltimeout(t->sleep_timeout);
                    t->sleep_timeout = NULL;
                }
                
                // Wake up the thread
                atomic_thread_state_change(t, THREAD_STATE_READY);
                add_to_ready_queue(t);
                
                TRACE_THREAD("CANCEL: Woke up thread %d for async cancellation", t->tid);
            }
        }
        return;
    }
    
    // For deferred cancellation, only cancel at explicit cancellation points
    // Mark that cancellation is pending and let cancellation points handle it
    t->cancel_requested = 1;
    
    spl(sr);
}

/**
 * Cancellation point - should be called at points where cancellation
 * can safely occur (like sleep, wait, etc.)
 * 
 * @param t Thread to check (if NULL, uses current thread)
 */
void pthread_testcancel_internal(struct thread *t) {
    if (!t) t = CURTHREAD;
    if (!t || t->magic != CTXT_MAGIC) return;
    
    // Only proceed if cancellation is enabled and pending
    if (t->cancel_state != PTHREAD_CANCEL_ENABLE || !t->cancel_pending) {
        return;
    }
    
    TRACE_THREAD("CANCEL: Cancellation point reached for thread %d", t->tid);
    
    // Exit with cancellation status
    proc_thread_exit(PTHREAD_CANCELED, NULL);
}

/**
 * Set thread cancellation state
 * 
 * @param state New cancellation state (PTHREAD_CANCEL_ENABLE/DISABLE)
 * @param oldstate Pointer to store old state (can be NULL)
 * @return 0 on success, error code on failure
 */
long _cdecl proc_thread_setcancelstate(int state, int *oldstate) {
    struct thread *t = CURTHREAD;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return EINVAL;
    }
    
    if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE) {
        return EINVAL;
    }
    
    register unsigned short sr = splhigh();
    
    if (oldstate) {
        *oldstate = t->cancel_state;
    }
    
    t->cancel_state = state;
    
    // If enabling cancellation and there's a pending cancellation, check it
    if (state == PTHREAD_CANCEL_ENABLE && t->cancel_pending) {
        spl(sr);
        check_thread_cancellation(t);
        return 0;
    }
    
    spl(sr);
    return 0;
}

/**
 * Set thread cancellation type
 * 
 * @param type New cancellation type (PTHREAD_CANCEL_DEFERRED/ASYNCHRONOUS)
 * @param oldtype Pointer to store old type (can be NULL)
 * @return 0 on success, error code on failure
 */
long _cdecl proc_thread_setcanceltype(int type, int *oldtype) {
    struct thread *t = CURTHREAD;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return EINVAL;
    }
    
    if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS) {
        return EINVAL;
    }
    
    register unsigned short sr = splhigh();
    
    if (oldtype) {
        *oldtype = t->cancel_type;
    }
    
    t->cancel_type = type;
    
    // If switching to asynchronous and there's a pending cancellation, handle it
    if (type == PTHREAD_CANCEL_ASYNCHRONOUS && t->cancel_pending && 
        t->cancel_state == PTHREAD_CANCEL_ENABLE) {
        spl(sr);
        check_thread_cancellation(t);
        return 0;
    }
    
    spl(sr);
    return 0;
}

/**
 * Cancel a thread
 * 
 * @param t Thread to cancel
 * @return 0 on success, error code on failure
 */
long _cdecl proc_thread_cancel(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC) {
        return EINVAL;
    }
    
    // Check if thread belongs to current process
    if (t->proc != curproc) {
        return EPERM;
    }
    
    register unsigned short sr = splhigh();
    
    // Check if thread is already exited
    if (t->state & (THREAD_STATE_EXITED | THREAD_STATE_ZOMBIE)) {
        spl(sr);
        return ESRCH;
    }
    
    // Mark cancellation as pending
    t->cancel_pending = 1;
    
    TRACE_THREAD("CANCEL: Cancellation requested for thread %d", t->tid);
    
    spl(sr);
    
    // Check if cancellation should be processed immediately
    check_thread_cancellation(t);
    
    return 0;
}

/**
 * Test for pending cancellation (explicit cancellation point)
 */
long _cdecl proc_thread_testcancel(void) {
    pthread_testcancel_internal(CURTHREAD);
    return 0;  // If we reach here, no cancellation occurred
}

/**
 * Initialize cancellation state for a new thread
 * 
 * @param t Thread to initialize
 */
void init_thread_cancellation(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC) return;
    
    t->cancel_state = PTHREAD_CANCEL_ENABLE;
    t->cancel_type = PTHREAD_CANCEL_DEFERRED;
    t->cancel_pending = 0;
    t->cancel_requested = 0;
    
    TRACE_THREAD("CANCEL: Initialized cancellation for thread %d", t->tid);
}

/**
 * Cleanup cancellation state when a thread exits
 * 
 * @param t Thread being destroyed
 */
void cleanup_thread_cancellation(struct thread *t) {
    if (!t || t->magic != CTXT_MAGIC) return;
    
    // Clear cancellation state
    t->cancel_state = PTHREAD_CANCEL_DISABLE;
    t->cancel_pending = 0;
    t->cancel_requested = 0;
    
    TRACE_THREAD("CANCEL: Cleaned up cancellation for thread %d", t->tid);
}