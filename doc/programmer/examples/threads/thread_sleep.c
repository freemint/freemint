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

#define PTSIG_GETID     13
#define PSCHED_YIELD    3

/* PE_THREAD mode for Pexec */
#define PE_THREAD      107

/* MiNT system call wrappers */
long Pthreadsignal(int op, long arg1, long arg2) {
    return trap_1_wlll(P_THREADSIGNAL, op, arg1, arg2);
}

long sys_p_sleepthread(long ms) {
    return trap_1_wl(P_SLEEP, ms);
}

void thread_exit(void) {
    trap_1_w(P_EXIT);
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

/* Constants */
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

/* Function prototypes */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int pthread_yield(void);
pthread_t pthread_self(void);

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

/* Global shared variables */
volatile int counter = 0;
volatile int thread1_done = 0;
volatile int thread2_done = 0;

/* Thread functions */
void *thread1_func(void *arg) {
    pthread_t my_tid = pthread_self();
    const char *my_name = (const char *)arg;
    
    printf("%s running with ID: %ld\n", my_name, my_tid);
    printf("Thread %ld: STARTED\n", my_tid);
    
    struct timeval sleep_time, wake_time;
    long delta_ms;
    
    while (!thread2_done) {
        printf("Thread %ld: counter = %d\n", my_tid, counter);
        printf("Thread %ld: Sleeping for 1 second\n", my_tid);
        
        // Get current time before sleep
        gettimeofday(&sleep_time, NULL);
        sys_p_sleepthread(1000);
        
        // Get current time after wake up
        gettimeofday(&wake_time, NULL);
        
        // Calculate delta in milliseconds
        delta_ms = (wake_time.tv_sec - sleep_time.tv_sec) * 1000 + 
                  (wake_time.tv_usec - sleep_time.tv_usec) / 1000;
        
        printf("Thread %ld: Woke up after %ld ms\n", my_tid, delta_ms);
    }
    
    printf("Thread 1: FINISHED\n");
    thread1_done = 1;
    
    return NULL;  // Must return a value
}

void *thread2_func(void *arg) {
    pthread_t my_tid = pthread_self();
    const char *my_name = (const char *)arg;
    
    printf("%s running with ID: %ld\n", my_name, my_tid);
    printf("Thread %ld: STARTED\n", my_tid);
    
    for(int i = 0; i < 5; i++) {
        counter++;
        printf("Thread %ld: counter = %d\n", my_tid, counter);
    }
    
    thread2_done = 1;
    printf("Thread %ld: FINISHED\n", my_tid);
    
    return NULL;  // Must return a value
}

int main(void) {
    pthread_t thread1, thread2;
    int result;
    const char *thread1_name = "This is Thread 1";
    const char *thread2_name = "This is Thread 2";
    
    printf("Main: CREATE THREADS\n");
    
    result = pthread_create(&thread1, NULL, (void *(*)(void *))thread1_func, (void*)thread1_name);
    if (result != 0) {
        printf("Error creating 1st thread: %d\n", result);
        return 1;
    }
    
    result = pthread_create(&thread2, NULL, (void *(*)(void *))thread2_func, (void*)thread2_name);
    if (result != 0) {
        printf("Error creating 2nd thread: %d\n", result);
        return 1;
    }
    
    printf("Main: threads created (tid1=%ld, tid2=%ld)\n", (long)thread1, (long)thread2);
    
    // Wait for both threads to finish
    while (!thread1_done || !thread2_done) {
        printf("Main: counter = %d, sleeping for 1 second\n", counter);
        usleep(1000000);  // Sleep for 1 second
    }
    
    printf("Main: all threads finished, counter final = %d\n", counter);
    
    return 0;
}
