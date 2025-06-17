/**
 * @file proc_threads_syscall.c
 * @brief Kernel Thread System Call Dispatcher
 * 
 * Routes pthread-related system calls to appropriate kernel subsystems.
 * Handles synchronization, signaling, scheduling, and cleanup operations.
 * 
 * @author Medour Mehdi
 * @date June 2025
 * @version 1.0
 */

#include "proc_threads.h"

#include "proc_threads_policy.h"
#include "proc_threads_signal.h"
#include "proc_threads_sync.h"
#include "proc_threads_scheduler.h"
#include "proc_threads_signal.h"
#include "proc_threads_sleep_yield.h"
#include "proc_threads_helper.h"
#include "proc_threads_tsd.h"
#include "proc_threads_cleanup.h"

long _cdecl sys_p_thread_ctrl(long mode, long arg1, long arg2) {
    switch (mode) {
        case THREAD_CTRL_EXIT: // Exit thread
            TRACE_THREAD("EXIT: sys_p_thread_ctrl called with exit mode");
            proc_thread_exit((void*)arg1);  // Use arg1 as the return value
            return 0;  // Should never reach here

        case THREAD_CTRL_STATUS:
            // TRACE_THREAD("STATUS: proc_thread_status called for tid=%ld", arg1);
            // Get thread status
            return proc_thread_status(arg1);

        case THREAD_CTRL_GETID:
            return sys_p_thread_getid();

        default:
            TRACE_THREAD("ERROR: sys_p_thread_ctrl called with invalid mode %d", mode);
            return EINVAL;
    }
}

long _cdecl sys_p_thread_signal(long func, long arg1, long arg2) {
    
    TRACE_THREAD("sys_p_thread_signal: func=%ld, arg1=%ld, arg2=%ld", func, arg1, arg2);
    
    switch (func) {
        case PTSIG_MODE:
            TRACE_THREAD("proc_thread_signal_mode: %s thread signals", 
                        arg1 ? "enabling" : "disabling");
            return proc_thread_signal_mode((int)arg1);
        case PTSIG_KILL:
            {        
            TRACE_THREAD("PTSIG_KILL: arg1=%ld, arg2=%ld", arg1, arg2);
            /* Send signal to specific thread */
            struct thread *target = NULL;
            struct proc *p = curproc;
            
            /* Find thread by ID */
            register unsigned short sr = splhigh();
            struct thread *t;
            for (t = p->threads; t != NULL; t = t->next) {
                if (t->tid == arg1) {
                    target = t;
                    break;
                }
            }
            spl(sr);
            
            if (!target) {
                TRACE_THREAD("PTSIG_KILL: Thread with ID %ld not found", arg1);
                return ESRCH;
            }
            
            /* Send signal to target thread */
            TRACE_THREAD("PTSIG_KILL: Sending signal %ld to thread %d", arg2, target->tid);
            return proc_thread_signal_kill(target, (int)arg2);
            }
        case PTSIG_GETMASK:
            return proc_thread_signal_sigmask(0);
            
        case PTSIG_SETMASK:
            return proc_thread_signal_sigmask(arg1);
            
        case PTSIG_BLOCK:
            return proc_thread_signal_sigblock((ulong)arg1);
            
        case PTSIG_UNBLOCK:
            {
                struct thread *t = CURTHREAD;
                if (!t) return EINVAL;
                ulong old_mask = t->t_sigmask;
                t->t_sigmask &= ~(arg1 & ~UNMASKABLE);
                return old_mask;
            }
            
        case PTSIG_WAIT:
            return proc_thread_signal_sigwait(arg1, arg2);
            
        case PTSIG_HANDLER:
            TRACE_THREAD("sys_p_thread_signal -> proc_thread_signal_sighandler: PROC ID %d, THREAD ID %d, SIG %ld, HANDLER %lx, ARG %p", 
                        curproc ? curproc->pid : -1, 
                        CURTHREAD ? CURTHREAD->tid : -1,
                        arg1, arg2, NULL);
            return proc_thread_signal_sighandler((int)arg1, (void (*)(int, void*))arg2, NULL);
            
        case PTSIG_HANDLER_ARG:
            TRACE_THREAD("sys_p_thread_signal -> proc_thread_signal_sighandler_arg: PROC ID %d, THREAD ID %d, SIG %ld, ARG %ld", 
                        curproc ? curproc->pid : -1, 
                        CURTHREAD ? CURTHREAD->tid : -1,
                        arg1, arg2);
            return proc_thread_signal_sighandler_arg((int)arg1, (void*)arg2);

        case PTSIG_PENDING:
           {
               struct thread *t = CURTHREAD;
               if (!t) return EINVAL;
               return t->t_sigpending;
           }            
        case PTSIG_ALARM:
            return proc_thread_signal_sigalrm(CURTHREAD, arg1);

        case PTSIG_BROADCAST:
            return proc_thread_signal_broadcast(arg1);

        default:
            if (func > 0 && func < NSIG) {
                /* Direct signal to thread */
                TRACE_THREAD("Sending signal %ld to thread ID %ld", func, arg1);
                struct thread *target = NULL;
                struct proc *p = curproc;
                
                if (arg1 == 0) {
                    /* Signal current thread */
                    target = CURTHREAD;
                    TRACE_THREAD("Using current thread (ID %d)", target ? target->tid : -1);
                } else {
                    /* Find thread by ID */
                    register unsigned short sr = splhigh();
                    struct thread *t;
                    for (t = p->threads; t != NULL; t = t->next) {
                        if (t->tid == arg1) {
                            target = t;
                            TRACE_THREAD("Found thread with ID %ld", arg1);
                            break;
                        }
                    }
                    spl(sr);
                    
                    if (!target) {
                        TRACE_THREAD("Thread with ID %ld not found", arg1);
                        return ESRCH;
                    }
                }
                
                /* Now deliver the signal to the target thread */
                if (target) {
                    TRACE_THREAD("Sending signal %ld to thread ID %ld", func, target->tid);
                    return proc_thread_signal_kill(target, (int)func);
                }
            }
            
            TRACE_THREAD("Invalid function code: %ld", func);
            return EINVAL;
    }
}

long _cdecl sys_p_thread_sync(long operator, long arg1, long arg2) {
    
    TRACE_THREAD("sys_p_thread_sync(OP = %ld, arg1 = %ld, arg2= %ld)", operator, arg1, arg2);
    
    switch (operator) {
        case THREAD_SYNC_SEM_WAIT:
            TRACE_THREAD("THREAD_SYNC_SEM_WAIT");
            return thread_semaphore_down((struct semaphore *)arg1);
            
        case THREAD_SYNC_SEM_POST:
            TRACE_THREAD("THREAD_SYNC_SEM_POST");
            return thread_semaphore_up((struct semaphore *)arg1);
            
        case THREAD_SYNC_MUTEX_LOCK:
            TRACE_THREAD("THREAD_SYNC_MUTEX_LOCK");
            return thread_mutex_lock((struct mutex *)arg1);
            
        case THREAD_SYNC_MUTEX_UNLOCK:
            TRACE_THREAD("THREAD_SYNC_MUTEX_UNLOCK");
            return thread_mutex_unlock((struct mutex *)arg1);
            
        case THREAD_SYNC_MUTEX_INIT:
            TRACE_THREAD("THREAD_SYNC_MUTEX_INIT");
            return thread_mutex_init((struct mutex *)arg1);
            
        case THREAD_SYNC_SEM_INIT: {
            struct semaphore *sem = (struct semaphore *)arg1;
            TRACE_THREAD("THREAD_SYNC_SEM_INIT: count=%d", sem->count);
            return thread_semaphore_init(sem, sem->count);
        }            
        case THREAD_SYNC_JOIN: // Join thread
            TRACE_THREAD("JOIN: proc_thread_join called for tid=%ld", arg1);
            return proc_thread_join(arg1, (void**)arg2);
            
        case THREAD_SYNC_DETACH: // Detach thread
            TRACE_THREAD("DETACH: proc_thread_detach called for tid=%ld", arg1);
            return proc_thread_detach(arg1);

        case THREAD_SYNC_TRYJOIN:
            TRACE_THREAD("TRYJOIN: proc_thread_tryjoin called for tid=%ld", arg1);
            // New non-blocking join
            return proc_thread_tryjoin(arg1, (void **)arg2);
        case THREAD_SYNC_SLEEP:
            return proc_thread_sleep((long)arg1);
        case THREAD_SYNC_YIELD:
            return proc_thread_yield();
        case THREAD_SYNC_COND_INIT:
            TRACE_THREAD("THREAD_SYNC_COND_INIT");
            return proc_thread_condvar_init((struct condvar *)arg1);
            
        case THREAD_SYNC_COND_DESTROY:
            TRACE_THREAD("THREAD_SYNC_COND_DESTROY");
            return proc_thread_condvar_destroy((struct condvar *)arg1);
            
        case THREAD_SYNC_COND_WAIT:
            TRACE_THREAD("THREAD_SYNC_COND_WAIT");
            return proc_thread_condvar_wait((struct condvar *)arg1, (struct mutex *)arg2);
            
        case THREAD_SYNC_COND_TIMEDWAIT:
            TRACE_THREAD("THREAD_SYNC_COND_TIMEDWAIT");
            return proc_thread_condvar_timedwait((struct condvar *)arg1, (struct mutex *)arg2, 
                                          ((struct condvar *)arg1)->timeout_ms);
            
        case THREAD_SYNC_COND_SIGNAL:
            TRACE_THREAD("THREAD_SYNC_COND_SIGNAL");
            return proc_thread_condvar_signal((struct condvar *)arg1);
            
        case THREAD_SYNC_COND_BROADCAST:
            TRACE_THREAD("THREAD_SYNC_COND_BROADCAST");
            return proc_thread_condvar_broadcast((struct condvar *)arg1);

        case THREAD_SYNC_CLEANUP_PUSH:
            TRACE_THREAD("THREAD_SYNC_CLEANUP_PUSH: routine=%p, arg=%p", (void*)arg1, (void*)arg2);
            return thread_cleanup_push((void (*)(void*))arg1, (void*)arg2);
            
        case THREAD_SYNC_CLEANUP_POP:
            TRACE_THREAD("THREAD_SYNC_CLEANUP_POP: routine_ptr=%p, arg_ptr=%p", (void*)arg1, (void*)arg2);
            return thread_cleanup_pop((void (**)(void*))arg1, (void**)arg2);

        case THREAD_SYNC_CLEANUP_GET:
            TRACE_THREAD("THREAD_SYNC_CLEANUP_GET: handlers=%p, max_handlers=%ld", (void*)arg1, arg2);
            return get_cleanup_handlers(CURTHREAD, (struct cleanup_info*)arg1, (int)arg2);

        default:
            TRACE_THREAD("THREAD_SYNC_UNKNOWN: %d", operator);
            return EINVAL;
    }
}

long _cdecl sys_p_thread_sched_policy(long func, long arg1, long arg2, long arg3) {
    
    TRACE_THREAD("IN KERNEL: sys_p_thread_sched_policy: func=%ld, arg1=%ld, arg2=%ld, arg3=%ld", 
                func, arg1, arg2, arg3);
    
    switch (func) {
        case PSCHED_SETPARAM:
            return proc_thread_set_schedparam(arg1, arg2, arg3);
            
        case PSCHED_GETPARAM:
            return proc_thread_get_schedparam(arg1, (long*)arg2, (long*)arg3);
            
        case PSCHED_GETRRINTERVAL:
            return proc_thread_get_rrinterval(arg1, (long*)arg2);
            
        case PSCHED_SET_TIMESLICE:
            return proc_thread_set_timeslice(arg1, arg2);
            
        case PSCHED_GET_TIMESLICE:
            return proc_thread_get_timeslice(arg1, (long*)arg2, (long*)arg3);
            
        default:
            return EINVAL;
    }
}

/**
 * System call handler for thread-specific data operations
 * 
 * @param op Operation code (THREAD_TSD_*)
 * @param arg1 First argument (key or destructor)
 * @param arg2 Second argument (value)
 * @return Operation-specific return value
 */
long _cdecl sys_p_thread_tsd(long op, long arg1, long arg2) {
    TRACE_THREAD("sys_p_thread_tsd: op=%ld, arg1=%ld, arg2=%ld", op, arg1, arg2);
    
    switch (op) {
        case THREAD_TSD_CREATE_KEY:
            return thread_key_create((void (*)(void*))arg1);
            
        case THREAD_TSD_DELETE_KEY:
            return thread_key_delete(arg1);
            
        case THREAD_TSD_GET_SPECIFIC:
            return (long)thread_getspecific(arg1);
            
        case THREAD_TSD_SET_SPECIFIC:
            return thread_setspecific(arg1, (void*)arg2);
            
        default:
            return EINVAL;
    }
}
