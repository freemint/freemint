#include <stdio.h>
#include <mint/mintbind.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

/* MiNT system call numbers */
#define P_TREADSHED     0x185
#define P_SLEEP         0x186
#define P_EXIT          0x18A
#define P_THREADSIGNAL  0x18E

/* Define the PE_THREAD mode for Pexec */
#define PE_THREAD      107

#define PTSIG_GETID     13
#define PSCHED_YIELD    3

/* Thread operation modes for sys_p_exitthread */
#define THREAD_EXIT     0   /* Exit the current thread */
#define THREAD_JOIN     1   /* Join a thread and wait for it to terminate */
#define THREAD_DETACH   2   /* Detach a thread, making it unjoinable */

/* MiNT system call wrappers */

long Pthreadsignal(int op, long arg1, long arg2) {
    return trap_1_wlll(P_THREADSIGNAL, op, arg1, arg2);
}

/* Thread Safe sleep function */
long sys_p_sleepthread(long ms) {
    return trap_1_wl(P_SLEEP, ms);
}

/* Not needed, for now threads exit once their function end */
void sys_p_thread_exit(void) {
    trap_1_wlll(P_EXIT, THREAD_EXIT, O, O);
}

long sys_p_threadjoin(long tid, void **status) {
    return trap_1_wlll(P_EXIT, THREAD_JOIN, tid, (long)status);
}

long sys_p_threaddetach(long tid) {
    return trap_1_wlll(P_EXIT, THREAD_DETACH, tid, O);
}

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

struct sched_param {
    int sched_priority;
    int sched_policy;
};

/*
 struct sched_param param;
param.sched_priority = 10;
param.sched_policy = SCHED_FIFO;
*/

/* Constants */
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

/* Function prototypes */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start_routine)(void*), void *arg);

pthread_t pthread_self(void);

int pthread_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* _PTHREAD_H */

/* pthread implementation */

/**
 * Causes the calling thread to relinquish the CPU, allowing other threads to be scheduled.
 * The thread is moved to the end of the queue for its static priority and a new thread
 * is scheduled to run.
 * 
 * @return 0 on success or a negative error code on failure.
 */
int pthread_yield(void)
{
    return (int)trap_1_wllll(P_TREADSHED, (long)PSCHED_YIELD, 0, 0, 0);
}

/**
 * Returns the thread ID of the calling thread.
 * 
 * @return The thread ID of the calling thread
 */
pthread_t pthread_self(void)
{
    /* Call the MiNT function to get the current thread ID */
    return (pthread_t)Pthreadsignal(PTSIG_GETID, 0, 0);
}

/**
 * Create a new thread, starting with execution of START-ROUTINE
 * getting passed ARG. Creation attributes come from ATTR.
 * 
 * @param thread    Pointer to pthread_t where thread ID will be stored
 * @param attr      Thread attributes (ignored in this implementation)
 * @param start_routine  Function to be executed by the thread
 * @param arg       Argument to pass to the start_routine
 * @return 0 on success, error code on failure
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start_routine)(void*), void *arg)
{
    long tid;
    
    /* Validate parameters */
    if (!thread || !start_routine)
        return EINVAL;
    
    /* Call Pexec with PE_THREAD mode to create the thread */
    tid = Pexec(PE_THREAD, start_routine, arg, NULL);
    
    if (tid < 0) {
        /* Convert MiNT error code to pthread error code */
        switch (tid) {
            case -ENOMEM:
                return EAGAIN;  /* No resources to create another thread */
            case -EINVAL:
                return EINVAL;  /* Invalid parameters */
            default:
                return EAGAIN;  /* Generic error */
        }
    }
    
    /* Store the thread ID */
    *thread = (pthread_t)tid;
    
    return 0;
}

// Global flag for synchronization
volatile int thread_done = 0;
volatile int counter = 0;

// Thread function that increments a counter
void counter_thread(void *arg) {
    int thread_num = (int)(long)arg;
    
    // Increment counter a few times
    for (int i = 0; i < 10; i++) {
        counter++;
        printf("Thread %d: counter = %d\n", thread_num, counter);
        pthread_yield();
    }
    
    // Mark as done if this is thread 1
    if (thread_num == 1) {
	    printf("thread 1 executed\n");
        thread_done = 1;
    }
}

int main(void) {

    pthread_t thread1, thread2;
    int result;
    
    printf("Main: TEST THREADS\n");
    
    result = pthread_create(&thread1, NULL, (void *(*)(void *))counter_thread, (void*)1);
    if (result != 0) {
        printf("Error creating 1st thread: %d\n", result);
        return 1;
    }
    
    result = pthread_create(&thread2, NULL, (void *(*)(void *))counter_thread, (void*)2);
    if (result != 0) {
        printf("Error creating 2nd thread: %d\n", result);
        return 1;
    }

    printf("Created threads: %ld, %ld\n", (long)thread1, (long)thread2);

    int timeout = 0;
    while (!thread_done && timeout < 30) {
        
        printf("Main: waiting for threads, counter = %d\n", counter);
        
        // Sleep for a bit to give threads a chance to run
        sleep(1);
        
        timeout++;
    }
    
    printf("Main: threads %s, final counter = %d\n", 
           thread_done ? "completed" : "timed out", counter);
    printf("Test completed\n");
    
    return 0;
}