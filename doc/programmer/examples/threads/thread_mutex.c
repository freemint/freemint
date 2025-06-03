#include <stdio.h>
#include <mint/mintbind.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

/* MiNT system call numbers */
#define P_TREADSHED     0x185
#define P_SLEEP         0x186
#define P_THREADOP      0x18D
#define P_EXIT          0x18A
#define P_THREADSIGNAL  0x18E

/* Define the PE_THREAD mode for Pexec */
#define PE_THREAD      107

#define PTSIG_GETID     13
#define PSCHED_YIELD    3

#define THREAD_OP_SEM_WAIT      1
#define THREAD_OP_SEM_POST      2
#define THREAD_OP_MUTEX_LOCK    3
#define THREAD_OP_MUTEX_UNLOCK  4
#define THREAD_OP_MUTEX_INIT    5
#define THREAD_OP_SEM_INIT      6

/* Thread operation modes for sys_p_exitthread */
#define THREAD_EXIT     0   /* Exit the current thread */
#define THREAD_JOIN     1   /* Join a thread and wait for it to terminate */
#define THREAD_DETACH   2   /* Detach a thread, making it unjoinable */

/* pthread.h definitions */
#ifndef _PTHREAD_H
#define _PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Thread ID type */
typedef long pthread_t;

/* Thread attribute type (minimal version) */
typedef struct {
    int detachstate;
    size_t stacksize;
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

/* Constants */
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1
#define PTHREAD_MUTEX_INITIALIZER {0, 0, NULL, NULL}

/* MiNT system call wrappers */

long sys_pthreadop(int op, void *arg) {
    return trap_1_wwl(P_THREADOP, (short)op, (long)arg);
}

long Pthreadsignal(int op, long arg1, long arg2) {
    return trap_1_wlll(P_THREADSIGNAL, op, arg1, arg2);
}

long sys_p_sleepthread(long ms) {
    return trap_1_wl(P_SLEEP, ms);
}

void sys_p_thread_exit(void) {
    trap_1_wlll(P_EXIT, THREAD_EXIT, O, O);
}

long sys_p_threadjoin(long tid, void **status) {
    return trap_1_wlll(P_EXIT, THREAD_JOIN, tid, (long)status);
}

long sys_p_threaddetach(long tid) {
    return trap_1_wlll(P_EXIT, THREAD_DETACH, tid, O);
}

/* Function prototypes */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int pthread_yield(void);
pthread_t pthread_self(void);

/* Mutex functions */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

/* Semaphore functions */
int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
int sem_destroy(sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* _PTHREAD_H */

/* pthread implementation */

/**
 * Causes the calling thread to relinquish the CPU
 */
int pthread_yield(void)
{
    return (int)trap_1_wllll(P_TREADSHED, (long)PSCHED_YIELD, 0, 0, 0);
}

/**
 * Returns the thread ID of the calling thread
 */
pthread_t pthread_self(void)
{
    return (pthread_t)Pthreadsignal(PTSIG_GETID, 0, 0);
}

/**
 * Create a new thread
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
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
    return 0;
}

/* Mutex functions with standard pthread prototypes */

/**
 * Initialize a mutex
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex init */
    long result = sys_pthreadop(THREAD_OP_MUTEX_INIT, mutex);
    
    return (result < 0) ? -result : 0;
}

/**
 * Lock a mutex
 */
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex lock */
    long result = sys_pthreadop(THREAD_OP_MUTEX_LOCK, mutex);
    
    return (result < 0) ? -result : 0;
}

/**
 * Try to lock a mutex (non-blocking)
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex) {
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
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!mutex)
        return EINVAL;
    
    /* Use MiNT thread operation system call for mutex unlock */
    long result = sys_pthreadop(THREAD_OP_MUTEX_UNLOCK, mutex);
    
    return (result < 0) ? -result : 0;
}

/**
 * Destroy a mutex
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex) {
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

/* Semaphore functions with standard POSIX prototypes */

/**
 * Initialize a semaphore
 */
int sem_init(sem_t *sem, int pshared, unsigned int value) {
    if (!sem){
        return -1;
    }
    
    /* Set initial count for kernel function to use */
    sem->count = (unsigned short)value;
    sem->wait_queue = NULL;

    /* Use MiNT thread operation system call for semaphore init */
    long result = sys_pthreadop(THREAD_OP_SEM_INIT, sem);

    return (result < 0) ? -1 : 0;
}

/**
 * Wait on a semaphore (decrement)
 */
int sem_wait(sem_t *sem) {
    if (!sem)
        return -1;
    
    /* Use MiNT thread operation system call for semaphore wait */
    long result = sys_pthreadop(THREAD_OP_SEM_WAIT, sem);
    
    return (result < 0) ? -1 : 0;
}

/**
 * Try to wait on a semaphore (non-blocking)
 */
int sem_trywait(sem_t *sem) {
    if (!sem)
        return -1;
    
    /* Check if count > 0 */
    if (sem->count <= 0) {
        errno = EAGAIN;
        return -1;
    }
    
    return sem_wait(sem);
}

/**
 * Post to a semaphore (increment)
 */
int sem_post(sem_t *sem) {
    if (!sem)
        return -1;
    
    /* Use MiNT thread operation system call for semaphore post */
    long result = sys_pthreadop(THREAD_OP_SEM_POST, sem);
    
    return (result < 0) ? -1 : 0;
}

/**
 * Get the current value of a semaphore
 */
int sem_getvalue(sem_t *sem, int *sval) {
    if (!sem || !sval)
        return -1;
    
    *sval = sem->count;
    return 0;
}

/**
 * Destroy a semaphore
 */
int sem_destroy(sem_t *sem) {
    if (!sem)
        return -1;
    
    /* Check if there are waiting threads */
    if (sem->wait_queue != NULL) {
        errno = EBUSY;
        return -1;
    }
    
    /* Reset fields */
    sem->count = 0;
    sem->wait_queue = NULL;
    
    return 0;
}

/* Test code using standard pthread functions */

// Global mutex and semaphore variables
pthread_mutex_t my_mutex;
sem_t my_semaphore;

volatile int done = 0;

// Thread function to test mutex
void *mutex_test_thread(void *arg) {
    int thread_num = *(int*)arg;
    
    printf("Mutex test thread %d started (ID: %ld)\n", thread_num, pthread_self());
    
    // Lock the mutex
    if (pthread_mutex_lock(&my_mutex) == 0) {
        printf("Mutex locked by thread %d (ID: %ld)\n", thread_num, pthread_self());
        
        // Sleep for 2 seconds while holding the mutex
        sys_p_sleepthread(2000);
        
        // Unlock the mutex
        if (pthread_mutex_unlock(&my_mutex) == 0) {
            printf("Mutex unlocked by thread %d (ID: %ld)\n", thread_num, pthread_self());
        } else {
            printf("Failed to unlock mutex in thread %d\n", thread_num);
        }
    } else {
        printf("Failed to lock mutex in thread %d\n", thread_num);
    }
    done += 1;
    return NULL;
}

// Thread function to test semaphore
void *semaphore_test_thread(void *arg) {
    int thread_num = *(int*)arg;
    
    printf("Semaphore test thread %d started (ID: %ld)\n", thread_num, pthread_self());
    
    // Wait on the semaphore
    if (sem_wait(&my_semaphore) == 0) {
        printf("Semaphore acquired by thread %d (ID: %ld)\n", thread_num, pthread_self());
        
        // Sleep for 2 seconds while holding the semaphore
        sys_p_sleepthread(2000);
        
        // Release the semaphore
        if (sem_post(&my_semaphore) == 0) {
            printf("Semaphore released by thread %d (ID: %ld)\n", thread_num, pthread_self());
        } else {
            printf("Failed to release semaphore in thread %d\n", thread_num);
        }
    } else {
        printf("Failed to acquire semaphore in thread %d\n", thread_num);
    }
    done += 1;
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3, thread4;
    int thread_args[4] = {1, 2, 3, 4};
    
    printf("Starting pthread synchronization test on MiNT\n");
    printf("Main process PID: %d\n\n", getpid());
    
    // Initialize the mutex and semaphore
    if (pthread_mutex_init(&my_mutex, NULL) != 0) {
        printf("Main process: Failed to initialize mutex\n");
        return 1;
    }
    printf("Main process: Mutex initialized\n");
    
    if (sem_init(&my_semaphore, 0, 1) != 0) {  // Initialize with count of 1
        printf("Main process: Failed to initialize semaphore\n");
        return 1;
    }
    printf("Main process: Semaphore initialized with count 1\n");
    
    printf("\n=== Testing Mutex (Main Process creates threads) ===\n");
    
    // Create two threads to test the mutex
    if (pthread_create(&thread1, NULL, mutex_test_thread, &thread_args[0]) != 0) {
        printf("Main process: Failed to create mutex test thread 1\n");
        return 1;
    }
    printf("Main process: Created mutex test thread 1 (TID: %ld)\n", thread1);
    
    if (pthread_create(&thread2, NULL, mutex_test_thread, &thread_args[1]) != 0) {
        printf("Main process: Failed to create mutex test thread 2\n");
        return 1;
    }
    printf("Main process: Created mutex test thread 2 (TID: %ld)\n", thread2);
    
    // Main process waits for mutex test to complete
    printf("Main process: Waiting for mutex tests to complete...\n");
    
    while (done < 2)
    {
        // printf("done = %d\n", done);
        sleep(1);
    }
    
    printf("\n=== Testing Semaphore (Main Process creates threads) ===\n");
    
    // Create two threads to test the semaphore
    if (pthread_create(&thread3, NULL, semaphore_test_thread, &thread_args[2]) != 0) {
        printf("Main process: Failed to create semaphore test thread 1\n");
        return 1;
    }
    printf("Main process: Created semaphore test thread 1 (TID: %ld)\n", thread3);
    
    if (pthread_create(&thread4, NULL, semaphore_test_thread, &thread_args[3]) != 0) {
        printf("Main process: Failed to create semaphore test thread 2\n");
        return 1;
    }
    printf("Main process: Created semaphore test thread 2 (TID: %ld)\n", thread4);
    
    // Main process waits for all tests to complete
    printf("Main process: Waiting for semaphore tests to complete...\n");
    
    while (done < 4)
    {
        // printf("done = %d\n", done);
        sleep(1);
    }
    
    // Clean up resources in main process
    printf("\nMain process: Cleaning up resources...\n");
    if (pthread_mutex_destroy(&my_mutex) == 0) {
        printf("Main process: Mutex destroyed successfully\n");
    } else {
        printf("Main process: Failed to destroy mutex\n");
    }
    
    if (sem_destroy(&my_semaphore) == 0) {
        printf("Main process: Semaphore destroyed successfully\n");
    } else {
        printf("Main process: Failed to destroy semaphore\n");
    }
    
    printf("Main process: Test completed successfully!\n");
    
    return 0;
}