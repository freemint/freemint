/**
 * mint_pthread.h - Comprehensive POSIX threads implementation for FreeMiNT
 *
 * This header provides a complete pthread implementation that maps to
 * FreeMiNT's native thread API. All functions are implemented inline
 * for ease of use and efficiency.
 */

#ifndef _MINT_PTHREAD_H
#define _MINT_PTHREAD_H

#include <mint/mintbind.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MiNT system call numbers */
#define P_TREAD_SHED     0x185
#define P_THREAD_SYNC      0x18D
#define P_THREAD_CTRL          0x18A
#define P_THREAD_SIGNAL  0x18E

/* Define the PE_THREAD mode for Pexec */
#define PE_THREAD       107

/* Thread operation codes */
#define THREAD_SYNC_SEM_WAIT      1
#define THREAD_SYNC_SEM_POST      2
#define THREAD_SYNC_MUTEX_LOCK    3
#define THREAD_SYNC_MUTEX_UNLOCK  4
#define THREAD_SYNC_MUTEX_INIT    5
#define THREAD_SYNC_SEM_INIT      6
#define THREAD_SYNC_JOIN     7   /* Join a thread and wait for it to terminate */
#define THREAD_SYNC_DETACH   8   /* Detach a thread, making it unjoinable */
#define THREAD_SYNC_TRYJOIN 9   /* Non-blocking join (new) */
#define THREAD_SYNC_SLEEP			10  /* Sleep for a specified number of milliseconds */
#define THREAD_SYNC_YIELD			11  /* Yield the current thread */

/* Thread operation modes for sys_p_thread_ctrl */
#define THREAD_CTRL_EXIT     0   /* Exit the current thread */
#define THREAD_CTRL_STATUS   4   /* Get thread status */
#define THREAD_CTRL_GETID    5	/* Get thread ID */

#define THREAD_STATE_RUNNING    0x0001
#define THREAD_STATE_READY      0x0002
#define THREAD_STATE_BLOCKED	0x0004
#define THREAD_STATE_STOPPED	0x0008
#define THREAD_STATE_ZOMBIE		0x0010
#define THREAD_STATE_DEAD		0x0020
#define THREAD_STATE_EXITED     (THREAD_STATE_ZOMBIE | THREAD_STATE_DEAD)
#define THREAD_STATE_LIVE       (THREAD_STATE_RUNNING | THREAD_STATE_READY)

/* Thread scheduling policies */
#define SCHED_OTHER     0
#define SCHED_FIFO      1
#define SCHED_RR        2

/* Thread scheduling operations */
#define PSCHED_SETPARAM       0
#define PSCHED_GETPARAM       1
#define PSCHED_SETSCHEDULER   2
#define PSCHED_GETSCHEDULER   3
#define PSCHED_GETRRINTERVAL  5
#define PSCHED_SET_TIMESLICE  6
#define PSCHED_GET_TIMESLICE  7

/* Operation codes for proc_thread_signal */
typedef enum {
    PTSIG_SETMASK        = 1,  /* Set thread signal mask */
    PTSIG_GETMASK        = 2,  /* Get thread signal mask (handler=0) */
    PTSIG_MODE           = 3,  /* Set thread signal mode (enable/disable) */
    PTSIG_KILL           = 4,  /* Send signal to thread */
    PTSIG_WAIT           = 5,  /* Wait for signals */
    PTSIG_BLOCK          = 6,  /* Block signals (add to mask) */
    PTSIG_UNBLOCK        = 7,  /* Unblock signals (remove from mask) */
    PTSIG_PAUSE          = 8,  /* Pause with specified mask */
    PTSIG_ALARM          = 9,  /* Set thread alarm */
    PTSIG_PENDING        = 11, /* Get pending signals */
    PTSIG_HANDLER        = 12, /* Register thread signal handler */
    PTSIG_HANDLER_ARG    = 14, /* Set argument for thread signal handler */
    PTSIG_ALARM_THREAD   = 16  /* Set alarm for specific thread */
} ptsig_op_t;


/* Thread ID type */
typedef long pthread_t;

#define PTHREAD_BARRIER_SERIAL_THREAD 1
#define CLOCK_THREAD_CPUTIME_ID 1

/* Define sched_param structure */
struct sched_param {
    int sched_priority;
};

/* Thread attribute type */
typedef struct {
    int detachstate;
    size_t stacksize;
    int policy;
    int priority;
    void *stackaddr;
    size_t guardsize;
} pthread_attr_t;

/* Internal structures */
struct thread {
    long id;
    void *stack;
};

struct semaphore {
    volatile unsigned short count;
    struct thread *wait_queue;
};

struct mutex {
    volatile short locked;
    struct thread *owner;
    struct thread *wait_queue;
};

/* Mutex type definitions */
typedef struct mutex pthread_mutex_t;

typedef struct {
    int type;
} pthread_mutexattr_t;

/* Semaphore type definitions */
typedef struct semaphore sem_t;

/* Thread-specific data key */
typedef unsigned int pthread_key_t;

/* Thread once control */
typedef int pthread_once_t;

/* Read-write lock */
typedef struct {
    pthread_mutex_t mutex;
    int readers;
    int writers;
    int writer_waiting;
} pthread_rwlock_t;

typedef struct {
    int type;
} pthread_rwlockattr_t;

/* Condition variable */
typedef struct {
    pthread_mutex_t *mutex;
    struct thread *wait_queue;
} pthread_cond_t;

typedef struct {
    int type;
} pthread_condattr_t;

/* Constants */
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_RECURSIVE  1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

#define PTHREAD_ONCE_INIT        0

#define PTHREAD_MUTEX_INITIALIZER {0, 0, NULL}
#define PTHREAD_RWLOCK_INITIALIZER {PTHREAD_MUTEX_INITIALIZER, 0, 0, 0}
#define PTHREAD_COND_INITIALIZER {NULL, NULL}

/* MiNT system call wrappers */

static inline long sys_p_thread_sync(long op, long arg1, long arg2) {
    return trap_1_wlll(P_THREAD_SYNC, (long)op, (long)arg1, (long)arg2);
}

static inline long proc_thread_signal(int op, long arg1, long arg2) {
    return trap_1_wlll(P_THREAD_SIGNAL, op, arg1, arg2);
}

static inline long proc_thread_sleep(long ms) {
    return sys_p_thread_sync(THREAD_SYNC_SLEEP, ms, 0);
}

static inline long sys_p_thread_ctrl(short mode, long arg1, long arg2) {
    return trap_1_wlll(P_THREAD_CTRL, mode, arg1, arg2);
}

static inline long sys_p_thread_sched_policy(long func, long arg1, long arg2, long arg3) {
    return trap_1_wllll(P_TREAD_SHED, func, arg1, arg2, arg3);
}

static inline long proc_thread_status(long arg1) {
    return sys_p_thread_ctrl(THREAD_CTRL_STATUS, arg1, 0);
}

/**
 * Creates a new thread
 * 
 * @param thread Pointer to store the thread ID
 * @param attr Thread attributes (can be NULL)
 * @param start_routine Function to execute in the new thread
 * @param arg Argument to pass to start_routine
 * @return 0 on success, error code on failure
 */
static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start_routine)(void*), void *arg)
{
    long tid;
    
    if (!thread || !start_routine)
        return EINVAL;
    
    tid = Pexec(PE_THREAD, start_routine, arg, NULL);
    
    if (tid < 0) {
        switch (tid) {
            case -ENOMEM:
                return EAGAIN;
            case -EINVAL:
                return EINVAL;
            default:
                return EAGAIN;
        }
    }
    
    *thread = (pthread_t)tid;
    
    // If detached state is requested, detach the thread
    if (attr && attr->detachstate == PTHREAD_CREATE_DETACHED) {
        sys_p_thread_sync(THREAD_SYNC_DETACH, tid, 0);
    }
    
    return 0;
}

/**
 * Terminates the calling thread
 * 
 * @param retval Return value to be returned to the joiner
 */
static inline void pthread_exit(void *retval)
{
    sys_p_thread_ctrl(THREAD_CTRL_EXIT, (long)retval, 0);
    /* Should never reach here */
    while(1);
}

/**
 * Causes the calling thread to relinquish the CPU
 */
static inline int pthread_yield(void)
{
    return (int)sys_p_thread_sync(THREAD_SYNC_YIELD, 0, 0);
}

/**
 * Waits for a thread to terminate
 * 
 * @param thread Thread ID to join
 * @param retval Pointer to store the thread's return value (can be NULL)
 * @return 0 on success, error code on failure
 */

int pthread_tryjoin(pthread_t thread, void **retval)
{
    while (1) {
        long status = proc_thread_status(thread);
        if (status == -ESRCH) {
            return (status == -ESRCH) ? ESRCH : EINVAL;
        }
        if (status & THREAD_STATE_EXITED) {
            // Thread exited, now do the actual join to get return value
            long result = sys_p_thread_sync(THREAD_SYNC_TRYJOIN, thread, (long)retval);
            if (result == 0 || result != -EAGAIN) {
                return (result == 0) ? 0 : EINVAL;
            }
        }
        // pthread_yield();
        usleep(20000); // Sleep for 20ms before checking again
    }
}

static inline int pthread_join(pthread_t thread, void **retval)
{
    long result = sys_p_thread_sync(THREAD_SYNC_JOIN, thread, (long)retval);

    if (result < 0) {
        switch (result) {
            case -ESRCH:
                return ESRCH;  // No such thread
            case -EINVAL:
                return EINVAL; // Thread is detached or already joined
            case -EDEADLK:
                return EDEADLK; // Deadlock detected
            default:
                printf("result: %ld\n", result);
                return EINVAL;
        }
    } else {
        printf("result: %ld\n", result);
    }
    
    return 0;
}

/**
 * Marks a thread as detached
 * 
 * @param thread Thread ID to detach
 * @return 0 on success, error code on failure
 */
static inline int pthread_detach(pthread_t thread)
{
    long result = sys_p_thread_ctrl(THREAD_SYNC_DETACH, thread, 0);
    
    if (result < 0) {
        switch (result) {
            case -ESRCH:
                return ESRCH;  // No such thread
            case -EINVAL:
                return EINVAL; // Thread is already joined
            default:
                return EINVAL;
        }
    }
    
    return 0;
}

/**
 * Returns the thread ID of the calling thread
 */
static inline pthread_t pthread_self(void)
{
    return (pthread_t)sys_p_thread_ctrl(THREAD_CTRL_GETID, 0, 0);
}

/**
 * Compares two thread IDs
 * 
 * @return Non-zero if equal, 0 if not equal
 */
static inline int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1 == t2;
}

/**
 * Initialize thread attributes
 */
static inline int pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr)
        return EINVAL;
    
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    attr->stacksize = 0;  // Default stack size
    attr->policy = SCHED_OTHER;
    attr->priority = 0;
    attr->stackaddr = NULL;
    attr->guardsize = 0;
    
    return 0;
}

/**
 * Destroy thread attributes
 */
static inline int pthread_attr_destroy(pthread_attr_t *attr)
{
    if (!attr)
        return EINVAL;
    
    // Nothing to do for now
    return 0;
}

/**
 * Set detach state in thread attributes
 */
static inline int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if (!attr || (detachstate != PTHREAD_CREATE_JOINABLE && 
                 detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;
    
    attr->detachstate = detachstate;
    return 0;
}

/**
 * Get detach state from thread attributes
 */
static inline int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    if (!attr || !detachstate)
        return EINVAL;
    
    *detachstate = attr->detachstate;
    return 0;
}

/**
 * Set stack size in thread attributes
 */
static inline int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (!attr || stacksize < 16384)  // Minimum 16K stack
        return EINVAL;
    
    attr->stacksize = stacksize;
    return 0;
}

/**
 * Get stack size from thread attributes
 */
static inline int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    if (!attr || !stacksize)
        return EINVAL;
    
    *stacksize = attr->stacksize;
    return 0;
}

/**
 * Set scheduling policy in thread attributes
 */
static inline int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
    if (!attr || (policy != SCHED_OTHER && policy != SCHED_FIFO && policy != SCHED_RR))
        return EINVAL;
    
    attr->policy = policy;
    return 0;
}

/**
 * Get scheduling policy from thread attributes
 */
static inline int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
    if (!attr || !policy)
        return EINVAL;
    
    *policy = attr->policy;
    return 0;
}

/**
 * Set scheduling priority in thread attributes
 */
static inline int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
    if (!attr || !param)
        return EINVAL;
    
    attr->priority = param->sched_priority;
    return 0;
}

/**
 * Get scheduling priority from thread attributes
 */
static inline int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
    if (!attr || !param)
        return EINVAL;
    
    param->sched_priority = attr->priority;
    return 0;
}

/* Mutex functions */

/**
 * Initialize a mutex
 */
static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex init */
    long result = sys_p_thread_sync(THREAD_SYNC_MUTEX_INIT, (long)mutex, 0);
    
    return (result < 0) ? -result : 0;
}

/**
 * Lock a mutex
 */
static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex lock */
    long result = sys_p_thread_sync(THREAD_SYNC_MUTEX_LOCK, (long)mutex, 0);
    
    return (result < 0) ? -result : 0;
}

/**
 * Try to lock a mutex (non-blocking)
 */
static inline int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    /* Check if already locked */
    if (mutex->locked)
        return EBUSY;
    
    return pthread_mutex_lock(mutex);
}

/**
 * Unlock a mutex
 */
static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex unlock */
    long result = sys_p_thread_sync(THREAD_SYNC_MUTEX_UNLOCK, (long)mutex, 0);
    
    return (result < 0) ? -result : 0;
}

/**
 * Destroy a mutex
 */
static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    /* Check if mutex is locked */
    if (mutex->locked)
        return EBUSY;
    
    /* Reset all fields */
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->wait_queue = NULL;
    
    return 0;
}

/**
 * Initialize mutex attributes
 */
static inline int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr)
        return EINVAL;
    
    attr->type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

/**
 * Destroy mutex attributes
 */
static inline int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    if (!attr)
        return EINVAL;
    
    // Nothing to do
    return 0;
}

/**
 * Set mutex type in attributes
 */
static inline int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    if (!attr || (type != PTHREAD_MUTEX_NORMAL && 
                 type != PTHREAD_MUTEX_RECURSIVE && 
                 type != PTHREAD_MUTEX_ERRORCHECK))
        return EINVAL;
    
    attr->type = type;
    return 0;
}

/**
 * Get mutex type from attributes
 */
static inline int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    if (!attr || !type)
        return EINVAL;
    
    *type = attr->type;
    return 0;
}

/* Semaphore functions */

/**
 * Initialize a semaphore
 */
static inline int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (!sem || pshared != 0)  // pshared not supported
        return -1;
    
    sem->count = value;
    
    /* Use MiNT thread operation system call for semaphore init */
    long result = sys_p_thread_sync(THREAD_SYNC_SEM_INIT, (long)sem, 0);
    
    return (result < 0) ? -1 : 0;
}

/**
 * Destroy a semaphore
 */
static inline int sem_destroy(sem_t *sem)
{
    if (!sem)
        return -1;
    
    /* Check if semaphore has waiters */
    if (sem->wait_queue)
        return -1;
    
    /* Reset all fields */
    sem->count = 0;
    sem->wait_queue = NULL;
    
    return 0;
}

/**
 * Wait on a semaphore
 */
static inline int sem_wait(sem_t *sem)
{
    if (!sem)
        return -1;
    
    /* Use MiNT thread operation system call for semaphore wait */
    long result = sys_p_thread_sync(THREAD_SYNC_SEM_WAIT, (long)sem, 0);
    
    return (result < 0) ? -1 : 0;
}

/**
 * Try to wait on a semaphore (non-blocking)
 */
static inline int sem_trywait(sem_t *sem)
{
    if (!sem)
        return -1;
    
    /* Check if semaphore has available count */
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (sem->count > 0) {
        sem->count--;
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        return 0;
    }
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    errno = EAGAIN;
    return -1;
}

/**
 * Post to a semaphore
 */
static inline int sem_post(sem_t *sem)
{
    if (!sem)
        return -1;
    
    /* Use MiNT thread operation system call for semaphore post */
    long result = sys_p_thread_sync(THREAD_SYNC_SEM_POST, (long)sem, 0);
    
    return (result < 0) ? -1 : 0;
}

/**
 * Get semaphore value
 */
static inline int sem_getvalue(sem_t *sem, int *value)
{
    if (!sem || !value)
        return -1;
    
    *value = sem->count;
    return 0;
}

/* Thread-specific data functions */

/* Internal TSD implementation */
#define MAX_KEYS 128

static pthread_key_t next_key = 0;
static void (*destructors[MAX_KEYS])(void*) = {NULL};
static void *tsd_values[MAX_KEYS] = {NULL};

/**
 * Create a thread-specific data key
 */
static inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    if (!key)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (next_key >= MAX_KEYS) {
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        return EAGAIN;
    }
    
    *key = next_key++;
    destructors[*key] = destructor;
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return 0;
}

/**
 * Delete a thread-specific data key
 */
static inline int pthread_key_delete(pthread_key_t key)
{
    if (key >= next_key)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    destructors[key] = NULL;
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return 0;
}

/**
 * Set thread-specific data
 */
static inline int pthread_setspecific(pthread_key_t key, const void *value)
{
    if (key >= next_key)
        return EINVAL;
    
    tsd_values[key] = (void*)value;
    return 0;
}

/**
 * Get thread-specific data
 */
static inline void *pthread_getspecific(pthread_key_t key)
{
    if (key >= next_key)
        return NULL;
    
    return tsd_values[key];
}

/* Thread once control */

/**
 * Execute a function once per process
 */
static inline int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (!once_control || !init_routine)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (*once_control == 0) {
        *once_control = 1;
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        init_routine();
    } else {
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    }
    
    return 0;
}

/* Read-write lock functions */

/**
 * Initialize a read-write lock
 */
static inline int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_init(&rwlock->mutex, NULL);
    if (result != 0)
        return result;
    
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->writer_waiting = 0;
    
    return 0;
}

/**
 * Destroy a read-write lock
 */
static inline int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    if (rwlock->readers > 0 || rwlock->writers > 0)
        return EBUSY;
    
    return pthread_mutex_destroy(&rwlock->mutex);
}

/**
 * Acquire a read lock
 */
static inline int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_lock(&rwlock->mutex);
    if (result != 0)
        return result;
    
    /* Wait while there are writers or a writer is waiting */
    while (rwlock->writers > 0 || rwlock->writer_waiting > 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        proc_thread_sleep(1);  // Sleep for 1ms
        result = pthread_mutex_lock(&rwlock->mutex);
        if (result != 0)
            return result;
    }
    
    rwlock->readers++;
    
    return pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Try to acquire a read lock (non-blocking)
 */
static inline int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_lock(&rwlock->mutex);
    if (result != 0)
        return result;
    
    /* Check if there are writers or a writer is waiting */
    if (rwlock->writers > 0 || rwlock->writer_waiting > 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        return EBUSY;
    }
    
    rwlock->readers++;
    
    return pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Acquire a write lock
 */
static inline int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_lock(&rwlock->mutex);
    if (result != 0)
        return result;
    
    rwlock->writer_waiting++;
    
    /* Wait while there are readers or writers */
    while (rwlock->readers > 0 || rwlock->writers > 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        proc_thread_sleep(1);  // Sleep for 1ms
        result = pthread_mutex_lock(&rwlock->mutex);
        if (result != 0) {
            rwlock->writer_waiting--;
            return result;
        }
    }
    
    rwlock->writer_waiting--;
    rwlock->writers++;
    
    return pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Try to acquire a write lock (non-blocking)
 */
static inline int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_lock(&rwlock->mutex);
    if (result != 0)
        return result;
    
    /* Check if there are readers or writers */
    if (rwlock->readers > 0 || rwlock->writers > 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        return EBUSY;
    }
    
    rwlock->writers++;
    
    return pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Release a read-write lock
 */
static inline int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    
    int result = pthread_mutex_lock(&rwlock->mutex);
    if (result != 0)
        return result;
    
    if (rwlock->readers > 0)
        rwlock->readers--;
    else if (rwlock->writers > 0)
        rwlock->writers--;
    else {
        pthread_mutex_unlock(&rwlock->mutex);
        return EPERM;  // Not locked
    }
    
    return pthread_mutex_unlock(&rwlock->mutex);
}

/* Condition variable functions */

/**
 * Initialize a condition variable
 */
static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    if (!cond)
        return EINVAL;
    
    cond->mutex = NULL;
    cond->wait_queue = NULL;
    
    return 0;
}

/**
 * Destroy a condition variable
 */
static inline int pthread_cond_destroy(pthread_cond_t *cond)
{
    if (!cond)
        return EINVAL;
    
    if (cond->wait_queue)
        return EBUSY;
    
    cond->mutex = NULL;
    cond->wait_queue = NULL;
    
    return 0;
}

/**
 * Wait on a condition variable
 */
static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!cond || !mutex)
        return EINVAL;
    
    /* Store the mutex for later use */
    cond->mutex = mutex;
    
    /* Unlock the mutex */
    pthread_mutex_unlock(mutex);
    
    /* Sleep until signaled */
    proc_thread_sleep(0xFFFFFFFF);  // Sleep indefinitely
    
    /* Reacquire the mutex */
    return pthread_mutex_lock(mutex);
}

/**
 * Wait on a condition variable with timeout
 */
static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    if (!cond || !mutex || !abstime)
        return EINVAL;
    
    /* Calculate relative timeout in milliseconds */
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    
    long ms = (abstime->tv_sec - now.tv_sec) * 1000;
    ms += (abstime->tv_nsec - now.tv_nsec) / 1000000;
    
    if (ms <= 0)
        return ETIMEDOUT;
    
    /* Store the mutex for later use */
    cond->mutex = mutex;
    
    /* Unlock the mutex */
    pthread_mutex_unlock(mutex);
    
    /* Sleep until signaled or timeout */
    long result = proc_thread_sleep(ms);
    
    /* Reacquire the mutex */
    pthread_mutex_lock(mutex);
    
    return (result == -ETIMEDOUT) ? ETIMEDOUT : 0;
}

/**
 * Signal a condition variable (wake one waiter)
 */
static inline int pthread_cond_signal(pthread_cond_t *cond)
{
    if (!cond)
        return EINVAL;
    
    /* Wake up one thread */
    /* This is a simplified implementation - in a real implementation,
       we would need to wake up a specific thread from the wait queue */
    
    return 0;
}

/**
 * Broadcast a condition variable (wake all waiters)
 */
static inline int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!cond)
        return EINVAL;
    
    /* Wake up all threads */
    /* This is a simplified implementation - in a real implementation,
       we would need to wake up all threads from the wait queue */
    
    return 0;
}

/* Thread scheduling functions */

/**
 * Set scheduling parameters for a thread
 */
static inline int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)
{
    if (!param)
        return EINVAL;
    
    long result = sys_p_thread_sched_policy(PSCHED_SETPARAM, thread, policy, param->sched_priority);
    
    return (result < 0) ? -result : 0;
}

/**
 * Get scheduling parameters for a thread
 */
static inline int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{
    if (!policy || !param)
        return EINVAL;
    
    long p, pri;
    long result = sys_p_thread_sched_policy(PSCHED_GETPARAM, thread, (long)&p, (long)&pri);
    
    if (result < 0)
        return -result;
    
    *policy = p;
    param->sched_priority = pri;
    
    return 0;
}

/**
 * Set scheduling policy and parameters for a thread
 */
static inline int pthread_setschedprio(pthread_t thread, int prio)
{
    struct sched_param param;
    int policy;
    
    int result = pthread_getschedparam(thread, &policy, &param);
    if (result != 0)
        return result;
    
    param.sched_priority = prio;
    return pthread_setschedparam(thread, policy, &param);
}

/**
 * Get the concurrency level
 */
static inline int pthread_getconcurrency(void)
{
    return 1;  // FreeMiNT is single-processor
}

/**
 * Set the concurrency level (hint to the implementation)
 */
static inline int pthread_setconcurrency(int level)
{
    return 0;  // Ignore, FreeMiNT is single-processor
}

/* Thread cancellation functions */
/* Note: These are stubs - FreeMiNT doesn't support thread cancellation directly */

#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
#define PTHREAD_CANCELED           ((void *) -1)

/**
 * Cancel a thread (stub implementation)
 */
static inline int pthread_cancel(pthread_t thread)
{
    return ENOSYS;  // Not supported
}

/**
 * Set cancelability state (stub implementation)
 */
static inline int pthread_setcancelstate(int state, int *oldstate)
{
    if (oldstate)
        *oldstate = PTHREAD_CANCEL_DISABLE;
    return 0;
}

/**
 * Set cancelability type (stub implementation)
 */
static inline int pthread_setcanceltype(int type, int *oldtype)
{
    if (oldtype)
        *oldtype = PTHREAD_CANCEL_DEFERRED;
    return 0;
}

/**
 * Test for a cancellation point (stub implementation)
 */
static inline void pthread_testcancel(void)
{
    // Do nothing
}

/* Thread sleep functions */

/**
 * Sleep for a specified duration
 * 
 * @param ms Milliseconds to sleep
 * @return 0 on success, error code on failure
 */
static inline int pthread_sleep_ms(long ms)
{
    long result = proc_thread_sleep(ms);
    return (result < 0) ? -result : 0;
}

/* Thread barrier functions */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int count;
    unsigned int waiting;
    unsigned int generation;
} pthread_barrier_t;

typedef struct {
    int pshared;
} pthread_barrierattr_t;

/**
 * Initialize a barrier
 */
static inline int pthread_barrier_init(pthread_barrier_t *barrier, 
                                      const pthread_barrierattr_t *attr,
                                      unsigned int count)
{
    if (!barrier || count == 0)
        return EINVAL;
    
    int result = pthread_mutex_init(&barrier->mutex, NULL);
    if (result != 0)
        return result;
    
    result = pthread_cond_init(&barrier->cond, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&barrier->mutex);
        return result;
    }
    
    barrier->count = count;
    barrier->waiting = 0;
    barrier->generation = 0;
    
    return 0;
}

/**
 * Destroy a barrier
 */
static inline int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    if (!barrier)
        return EINVAL;
    
    int result = pthread_mutex_lock(&barrier->mutex);
    if (result != 0)
        return result;
    
    if (barrier->waiting > 0) {
        pthread_mutex_unlock(&barrier->mutex);
        return EBUSY;
    }
    
    pthread_mutex_unlock(&barrier->mutex);
    
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
    
    return 0;
}

/**
 * Wait on a barrier
 */
static inline int pthread_barrier_wait(pthread_barrier_t *barrier)
{
    if (!barrier)
        return EINVAL;
    
    int result = pthread_mutex_lock(&barrier->mutex);
    if (result != 0)
        return result;
    
    unsigned int gen = barrier->generation;
    
    barrier->waiting++;
    
    if (barrier->waiting == barrier->count) {
        barrier->waiting = 0;
        barrier->generation++;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return PTHREAD_BARRIER_SERIAL_THREAD;
    }
    
    while (gen == barrier->generation) {
        result = pthread_cond_wait(&barrier->cond, &barrier->mutex);
        if (result != 0) {
            barrier->waiting--;
            pthread_mutex_unlock(&barrier->mutex);
            return result;
        }
    }
    
    pthread_mutex_unlock(&barrier->mutex);
    return 0;
}

/* Spinlock functions */
typedef int pthread_spinlock_t;

#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED  1

/**
 * Initialize a spinlock
 */
static inline int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    if (!lock)
        return EINVAL;
    
    *lock = 0;
    return 0;
}

/**
 * Destroy a spinlock
 */
static inline int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    
    return 0;
}

/**
 * Lock a spinlock
 */
static inline int pthread_spin_lock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    
    while (1) {
        unsigned short sr = 0;
        asm volatile ("move.w %%sr,%0" : "=d" (sr));
        asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
        
        if (*lock == 0) {
            *lock = 1;
            asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
            return 0;
        }
        
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        pthread_yield();
    }
}

/**
 * Try to lock a spinlock (non-blocking)
 */
static inline int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (*lock == 0) {
        *lock = 1;
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        return 0;
    }
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return EBUSY;
}

/**
 * Unlock a spinlock
 */
static inline int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    *lock = 0;
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return 0;
}

/* Thread-safe versions of standard functions */

/**
 * Thread-safe version of strtok
 */
static inline char *mint_strtok_r(char *str, const char *delim, char **saveptr)
{
    char *token;
    
    if (str == NULL)
        str = *saveptr;
    
    /* Skip leading delimiters */
    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    
    /* Find the end of the token */
    token = str;
    str = strpbrk(token, delim);
    if (str == NULL) {
        /* This token finishes the string */
        *saveptr = strchr(token, '\0');
    } else {
        /* Terminate the token and make *saveptr point past it */
        *str = '\0';
        *saveptr = str + 1;
    }
    
    return token;
}

/* Additional utility functions */

/**
 * Sleep for a specified number of milliseconds
 */
static inline int msleep(long ms)
{
    return pthread_sleep_ms(ms);
}

/**
 * Get the current thread's CPU time
 */
static inline int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id)
{
    if (!clock_id)
        return EINVAL;
    
    *clock_id = CLOCK_THREAD_CPUTIME_ID;
    return 0;
}

/**
 * Set thread name (not supported in FreeMiNT)
 */
static inline int pthread_setname_np(pthread_t thread, const char *name)
{
    return ENOSYS;  // Not supported
}

/**
 * Get thread name (not supported in FreeMiNT)
 */
static inline int pthread_getname_np(pthread_t thread, char *name, size_t len)
{
    if (!name || len == 0)
        return EINVAL;
    
    name[0] = '\0';
    return ENOSYS;  // Not supported
}

/* Optimized mutex implementation for single-CPU systems */

/**
 * Fast mutex type for single-CPU systems
 */
typedef struct {
    volatile int locked;
} pthread_fastmutex_t;

#define PTHREAD_FASTMUTEX_INITIALIZER {0}

/**
 * Initialize a fast mutex
 */
static inline int pthread_fastmutex_init(pthread_fastmutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    mutex->locked = 0;
    return 0;
}

/**
 * Lock a fast mutex
 */
static inline int pthread_fastmutex_lock(pthread_fastmutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    while (1) {
        unsigned short sr = 0;
        asm volatile ("move.w %%sr,%0" : "=d" (sr));
        asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
        
        if (mutex->locked == 0) {
            mutex->locked = 1;
            asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
            return 0;
        }
        
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        pthread_yield();
    }
}

/**
 * Try to lock a fast mutex (non-blocking)
 */
static inline int pthread_fastmutex_trylock(pthread_fastmutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (mutex->locked == 0) {
        mutex->locked = 1;
        asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
        return 0;
    }
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return EBUSY;
}

/**
 * Unlock a fast mutex
 */
static inline int pthread_fastmutex_unlock(pthread_fastmutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    unsigned short sr = 0;
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    mutex->locked = 0;
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    return 0;
}

/**
 * Destroy a fast mutex
 */
static inline int pthread_fastmutex_destroy(pthread_fastmutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    return 0;
}

/* Thread pool implementation */

typedef struct thread_pool_task {
    void (*function)(void *);
    void *argument;
    struct thread_pool_task *next;
} thread_pool_task_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    thread_pool_task_t *queue;
    int thread_count;
    int queue_size;
    int shutdown;
    int started;
} thread_pool_t;

/**
 * Destroy a thread pool
 */
static inline int _thread_pool_destroy(thread_pool_t *pool, int graceful)
{
    int i;
    thread_pool_task_t *task;
    
    if (pool == NULL)
        return -1;
    
    pthread_mutex_lock(&pool->lock);
    
    /* Set shutdown flag */
    pool->shutdown = 1;
    
    /* Wake up all worker threads */
    pthread_cond_broadcast(&pool->notify);
    
    pthread_mutex_unlock(&pool->lock);
    
    /* Wait for all worker threads to exit */
    for (i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    /* Free resources */
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    
    /* Free task queue */
    while (pool->queue) {
        task = pool->queue;
        pool->queue = task->next;
        
        if (!graceful) {
            /* Execute the task before freeing it */
            task->function(task->argument);
        }
        
        free(task);
    }
    
    free(pool->threads);
    free(pool);
    
    return 0;
}


/**
 * Thread pool worker function
 */
static void *thread_pool_worker(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    thread_pool_task_t *task;
    
    while (1) {
        pthread_mutex_lock(&pool->lock);
        
        /* Wait for a task to be available or for shutdown signal */
        while (pool->queue == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }
        
        /* Check for shutdown */
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }
        
        /* Get the first task from the queue */
        task = pool->queue;
        if (task) {
            pool->queue = task->next;
            pool->queue_size--;
        }
        
        pthread_mutex_unlock(&pool->lock);
        
        /* Execute the task */
        if (task) {
            task->function(task->argument);
            free(task);
        }
    }
    
    return NULL;
}

/**
 * Create a thread pool
 */
static inline thread_pool_t *thread_pool_create(int thread_count)
{
    thread_pool_t *pool;
    int i;
    
    /* Check arguments */
    if (thread_count <= 0)
        thread_count = 1;
    
    /* Allocate pool structure */
    pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool == NULL)
        return NULL;
    
    /* Initialize pool structure */
    pool->thread_count = thread_count;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->started = 0;
    pool->queue = NULL;
    
    /* Initialize mutex and condition variable */
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->notify, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool);
        return NULL;
    }
    
    /* Allocate thread array */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->notify);
        free(pool);
        return NULL;
    }
    
    /* Create worker threads */
    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool) != 0) {
            _thread_pool_destroy(pool, 1);
            return NULL;
        }
        pool->started++;
    }
    
    return pool;
}

/**
 * Add a task to the thread pool
 */
static inline int _thread_pool_add(thread_pool_t *pool, void (*function)(void *), void *argument)
{
    thread_pool_task_t *task;
    
    if (pool == NULL || function == NULL)
        return -1;
    
    /* Allocate task structure */
    task = (thread_pool_task_t *)malloc(sizeof(thread_pool_task_t));
    if (task == NULL)
        return -1;
    
    /* Initialize task */
    task->function = function;
    task->argument = argument;
    task->next = NULL;
    
    /* Add task to queue */
    pthread_mutex_lock(&pool->lock);
    
    /* Check for shutdown */
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        free(task);
        return -1;
    }
    
    /* Add task to queue */
    if (pool->queue == NULL) {
        pool->queue = task;
    } else {
        thread_pool_task_t *last = pool->queue;
        while (last->next)
            last = last->next;
        last->next = task;
    }
    
    pool->queue_size++;
    
    /* Signal a worker thread */
    pthread_cond_signal(&pool->notify);
    
    pthread_mutex_unlock(&pool->lock);
    
    return 0;
}

/* Public thread pool destroy function */
static inline int thread_pool_destroy(thread_pool_t *pool, int graceful)
{
    return _thread_pool_destroy(pool, graceful);
}

/* Atomic operations */

/**
 * Atomic increment
 */
static inline int atomic_increment(volatile int *value)
{
    unsigned short sr = 0;
    int result;
    
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    result = ++(*value);
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    
    return result;
}

/**
 * Atomic decrement
 */
static inline int atomic_decrement(volatile int *value)
{
    unsigned short sr = 0;
    int result;
    
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    result = --(*value);
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    
    return result;
}

/**
 * Atomic compare and swap
 */
static inline int atomic_cas(volatile int *ptr, int oldval, int newval)
{
    unsigned short sr = 0;
    int result;
    
    asm volatile ("move.w %%sr,%0" : "=d" (sr));
    asm volatile ("ori.w #0x0700,%%sr" : : : "memory");
    
    if (*ptr == oldval) {
        *ptr = newval;
        result = 1;
    } else {
        result = 0;
    }
    
    asm volatile ("move.w %0,%%sr" : : "d" (sr) : "memory");
    
    return result;
}

/* Optimized versions of common operations */

/**
 * Optimized memory barrier
 */
static inline void memory_barrier(void)
{
    asm volatile ("" : : : "memory");
}

/**
 * Optimized thread yield that minimizes context switches
 */
static inline void thread_yield_hint(void)
{
    /* For very short waits, just yield the CPU briefly */
    proc_thread_sleep(0);
}

/**
 * Optimized mutex implementation using atomic operations
 */
typedef struct {
    volatile int lock;
    volatile int contention;
} pthread_optimized_mutex_t;

#define PTHREAD_SYNCTIMIZED_MUTEX_INITIALIZER {0, 0}

/**
 * Initialize an optimized mutex
 */
static inline int pthread_optimized_mutex_init(pthread_optimized_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    mutex->lock = 0;
    mutex->contention = 0;
    
    return 0;
}

/**
 * Lock an optimized mutex
 */
static inline int pthread_optimized_mutex_lock(pthread_optimized_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    /* Try fast path first */
    if (atomic_cas(&mutex->lock, 0, 1))
        return 0;
    
    /* Contention detected, use slower path */
    atomic_increment(&mutex->contention);
    
    while (!atomic_cas(&mutex->lock, 0, 1)) {
        /* Yield to avoid busy waiting */
        thread_yield_hint();
    }
    
    atomic_decrement(&mutex->contention);
    
    return 0;
}

/**
 * Try to lock an optimized mutex
 */
static inline int pthread_optimized_mutex_trylock(pthread_optimized_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    if (atomic_cas(&mutex->lock, 0, 1))
        return 0;
    
    return EBUSY;
}

/**
 * Unlock an optimized mutex
 */
static inline int pthread_optimized_mutex_unlock(pthread_optimized_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    memory_barrier();
    mutex->lock = 0;
    
    return 0;
}

/**
 * Destroy an optimized mutex
 */
static inline int pthread_optimized_mutex_destroy(pthread_optimized_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    
    if (mutex->lock || mutex->contention)
        return EBUSY;
    
    return 0;
}

/* Utility macros */

/**
 * Execute code once per process
 */
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_ONCE(once_control, init_routine) \
    do { \
        static pthread_once_t once_control = PTHREAD_ONCE_INIT; \
        pthread_once(&once_control, init_routine); \
    } while (0)

/**
 * Critical section macro
 */
#define PTHREAD_CRITICAL_SECTION(mutex) \
    for (int _cs_done = 0, _cs_result = pthread_mutex_lock(mutex); \
         !_cs_done; \
         _cs_done = 1, pthread_mutex_unlock(mutex))

/**
 * Thread-safe initialization macro
 */
#define PTHREAD_INIT_ONCE(flag, init_code) \
    do { \
        static volatile int _init_flag = 0; \
        static pthread_mutex_t _init_mutex = PTHREAD_MUTEX_INITIALIZER; \
        if (!_init_flag) { \
            pthread_mutex_lock(&_init_mutex); \
            if (!_init_flag) { \
                init_code; \
                memory_barrier(); \
                _init_flag = 1; \
            } \
            pthread_mutex_unlock(&_init_mutex); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MINT_PTHREAD_H */

/*
# Pthread Functions Requiring Future Kernel Implementation

Here's a list of pthread functions that would benefit from proper kernel-level implementation in FreeMiNT:

1. **Thread Management**
   - `pthread_join` - Needs proper kernel support for thread waiting
   - `pthread_detach` - Needs kernel support for resource cleanup
   - `pthread_cancel` - Currently a stub, needs full cancellation support
   - `pthread_setname_np` - For thread naming/debugging
   - `pthread_getname_np` - For thread naming/debugging

2. **Condition Variables**
   - `pthread_cond_signal` - Needs proper thread wakeup mechanism
   - `pthread_cond_broadcast` - Needs proper thread wakeup mechanism
   - `pthread_cond_wait` - Needs proper thread blocking mechanism
   - `pthread_cond_timedwait` - Needs proper timeout handling

3. **Thread-Specific Data**
   - `pthread_key_create` - Needs per-thread storage in kernel
   - `pthread_getspecific` - Needs per-thread storage in kernel
   - `pthread_setspecific` - Needs per-thread storage in kernel

4. **Scheduling**
   - `pthread_setschedparam` - Needs full scheduler integration
   - `pthread_getschedparam` - Needs full scheduler integration
   - `pthread_setschedprio` - Needs full scheduler integration

5. **Cancellation Points**
   - `pthread_testcancel` - Needs cancellation support
   - `pthread_setcancelstate` - Needs cancellation support
   - `pthread_setcanceltype` - Needs cancellation support

6. **Barriers**
   - `pthread_barrier_wait` - Needs efficient implementation

7. **Read-Write Locks**
   - `pthread_rwlock_rdlock` - Needs efficient implementation
   - `pthread_rwlock_wrlock` - Needs efficient implementation
   - `pthread_rwlock_unlock` - Needs efficient implementation

8. **Spinlocks**
   - `pthread_spin_lock` - Needs CPU-specific optimizations
   - `pthread_spin_unlock` - Needs CPU-specific optimizations

9. **Signal Handling**
   - `pthread_sigmask` - Needs per-thread signal masks
   - `pthread_kill` - Needs thread-specific signal delivery

10. **Cleanup Handlers**
    - `pthread_cleanup_push` - Needs stack unwinding support
    - `pthread_cleanup_pop` - Needs stack unwinding support

This list represents the functions that would most benefit from kernel-level implementation to improve performance, correctness, and POSIX compliance in FreeMiNT's threading system.

*/