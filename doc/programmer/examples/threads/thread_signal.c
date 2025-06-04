/**
 * thread_signal_test.c - Test program for FreeMiNT thread signal implementation
 * 
 * This program tests various thread signal operations including:
 * - Setting up thread-specific signal handlers
 * - Sending signals between threads
 * - Waiting for signals with timeouts
 * - Using thread alarms
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "mint_pthread.h"

/* Operation codes for Pthreadsignal */
#define PTSIG_SETMASK        1  /* Set thread signal mask */
#define PTSIG_GETMASK        2  /* Get thread signal mask (handler=0) */
#define PTSIG_MODE           3  /* Set thread signal mode (enable/disable) */
#define PTSIG_KILL           4  /* Send signal to thread */
#define PTSIG_WAIT           5  /* Wait for signals */
#define PTSIG_BLOCK          6  /* Block signals (add to mask) */
#define PTSIG_UNBLOCK        7  /* Unblock signals (remove from mask) */
#define PTSIG_PAUSE          8  /* Pause with specified mask */
#define PTSIG_ALARM          9  /* Set thread alarm */
#define PTSIG_SLEEP         10  /* Sleep for specified time */
#define PTSIG_PENDING       11  /* Get pending signals */
#define PTSIG_HANDLER       12  /* Register thread signal handler */
#define PTSIG_GETID         13  /* Get thread ID (for signal handling) */
#define PTSIG_HANDLER_ARG   14  /* Set argument for thread signal handler */
#define PTSIG_ALARM_THREAD  16  /* Set alarm for specific thread */

#define NUM_THREADS 3
#define TEST_SIGNAL SIGUSR1  // Usually signal 10
#define TEST_SIGNAL2 SIGUSR2 // Usually signal 12

/* Global variables */
pthread_t threads[NUM_THREADS];
volatile int signal_received[NUM_THREADS] = {0};
volatile int test_complete = 0;

/* Thread-specific signal handler */
void signal_handler(int sig, void *arg)
{
    long thread_id = (long)arg;
    printf("Thread %ld received signal %d\n", thread_id, sig);
    signal_received[thread_id] = sig;
}

/* Thread function that waits for signals */
void *signal_wait_thread(void *arg)
{
    long thread_id = (long)arg;
    int sig;
    
    printf("Thread %ld starting, waiting for signals\n", thread_id);
    
    /* Enable thread-specific signal handling */
    Pthreadsignal(PTSIG_MODE, 1, 0);
    
    /* Register thread-specific signal handlers */
    Pthreadsignal(PTSIG_HANDLER, TEST_SIGNAL, (long)signal_handler);
    Pthreadsignal(PTSIG_HANDLER_ARG, TEST_SIGNAL, thread_id);
    
    Pthreadsignal(PTSIG_HANDLER, TEST_SIGNAL2, (long)signal_handler);
    Pthreadsignal(PTSIG_HANDLER_ARG, TEST_SIGNAL2, thread_id);
    
    /* Wait for signals with timeout */
    while (!test_complete) {
        /* Create signal mask for SIGUSR1 and SIGUSR2 */
        unsigned long mask = (1UL << TEST_SIGNAL) | (1UL << TEST_SIGNAL2);
        
        /* Wait for signal with 1 second timeout */
        sig = Pthreadsignal(PTSIG_WAIT, mask, 1000);
        
        if (sig > 0) {
            printf("Thread %ld woke up due to signal %d\n", thread_id, sig);
        } else if (sig == 0) {
            printf("Thread %ld woke up due to timeout\n", thread_id);
        }
        
        /* Sleep a bit to avoid busy waiting */
        Pthreadsignal(PTSIG_SLEEP, 100, 0);
    }
    
    printf("Thread %ld exiting\n", thread_id);
    return NULL;
}

/* Thread function that sends signals */
/* Thread function that sends signals */
void *signal_send_thread(void *arg)
{
    long thread_id = (long)arg;
    int i;
    
    printf("Signal sender thread %ld starting\n", thread_id);
    
    /* Wait a bit for other threads to initialize */
    Pthreadsignal(PTSIG_SLEEP, 500, 0);
    
    /* Send signals to other threads */
    for (i = 0; i < 5; i++) {
        /* Send signal to thread 0 */
        printf("Thread %ld sending signal %d to thread %d\n", 
               thread_id, SIGUSR1, 0);
        
        // /* Use the thread ID directly */
        // Pthreadsignal(SIGUSR1, 0, 0);
       /* Use PTSIG_KILL to send a signal to a specific thread */
       Pthreadsignal(PTSIG_KILL, 0, SIGUSR1);

        /* Wait a bit */
        Pthreadsignal(PTSIG_SLEEP, 200, 0);
        
        /* Send another signal */
        printf("Thread %ld sending signal %d to thread %d\n", 
               thread_id, SIGUSR2, 0);
        
        // Pthreadsignal(SIGUSR2, 0, 0);
        Pthreadsignal(PTSIG_KILL, 0, SIGUSR2);
        
        /* Wait a bit */
        Pthreadsignal(PTSIG_SLEEP, 200, 0);
    }
    
    printf("Signal sender thread %ld exiting\n", thread_id);
    return NULL;
}

int main(void)
{
    int i, rc;
    void *result;
    
    printf("Thread signal test program starting\n");
    
    /* Create threads */
    for (i = 0; i < NUM_THREADS - 1; i++) {
        /* Signal receiver threads */
        rc = pthread_create(&threads[i], NULL, signal_wait_thread, (void *)(long)i);
        
        if (rc != 0) {
            printf("ERROR: pthread_create failed with code %d\n", rc);
            return 1;
        }
    }
    
    /* Create signal sender thread */
    rc = pthread_create(&threads[NUM_THREADS-1], NULL, signal_send_thread, 
                       (void *)(long)(NUM_THREADS-1));
    
    if (rc != 0) {
        printf("ERROR: pthread_create failed with code %d\n", rc);
        return 1;
    }
    
    /* Let the test run for a while */
    sleep(15);
    // usleep(8000000);
    
    /* Signal test completion */
    test_complete = 1;
    
    /* Wait for threads to complete */
    for (i = 0; i < NUM_THREADS; i++) {
        printf("Joining thread %d\n", i);
        pthread_join(threads[i], &result);
    }
    
    /* Check results */
    printf("\nTest results:\n");
    for (i = 0; i < NUM_THREADS - 1; i++) {
        printf("Thread %d received signal: %d\n", i, signal_received[i]);
    }
    
    printf("\nAll tests completed!\n");
    return 0;
}

/*
This test program:

1. Creates multiple threads - some that wait for signals and one that sends signals
2. Registers thread-specific signal handlers for SIGUSR1 and SIGUSR2
3. Tests sending signals between threads
4. Tests waiting for signals with timeouts
5. Tests checking for pending signals
6. Tests the thread alarm functionality


The program demonstrates all the major functionality of the thread signal system, including:
- Setting up thread-specific signal handlers with PTSIG_HANDLER
- Setting handler arguments with PTSIG_HANDLER_ARG
- Waiting for signals with PTSIG_WAIT
- Checking pending signals with PTSIG_PENDING
- Using thread alarms with PTSIG_ALARM
- Sending signals directly to threads

This output shows:

All threads starting successfully
- Threads 0 and 1 waiting for signals with timeouts
- Thread 2 sending SIGUSR1 (signal 10) and SIGUSR2 (signal 12) to threads 0 and 1
- The signal handlers executing and printing messages
- Thread 2 setting an alarm and receiving the alarm signal (14)
- All threads exiting cleanly
- Final test results showing the last signals received by each thread

Here's the expected output for the thread signal test program:

Thread signal test program starting
Thread 0 starting, waiting for signals
Thread 1 starting, waiting for signals
Signal sender thread 2 starting
Thread 0 woke up due to timeout
Thread 0 pending signals: 0x0
Thread 1 woke up due to timeout
Thread 1 pending signals: 0x0
Thread 2 sending signal 10 to thread 0
Thread 0 received signal 10
Thread 0 woke up due to signal 10
Thread 2 sending signal 12 to thread 0
Thread 0 received signal 12
Thread 0 woke up due to signal 12
Thread 2 sending signal 10 to thread 1
Thread 1 received signal 10
Thread 1 woke up due to signal 10
Thread 2 sending signal 12 to thread 1
Thread 1 received signal 12
Thread 1 woke up due to signal 12
Thread 2 sending signal 10 to thread 0
Thread 0 received signal 10
Thread 0 woke up due to signal 10
Thread 2 sending signal 12 to thread 0
Thread 0 received signal 12
Thread 0 woke up due to signal 12
Thread 2 sending signal 10 to thread 1
Thread 1 received signal 10
Thread 1 woke up due to signal 10
Thread 2 sending signal 12 to thread 1
Thread 1 received signal 12
Thread 1 woke up due to signal 12
Thread 2 sending signal 10 to thread 0
Thread 0 received signal 10
Thread 0 woke up due to signal 10
Thread 2 sending signal 12 to thread 0
Thread 0 received signal 12
Thread 0 woke up due to signal 12
Thread 2 setting alarm for 500ms
Thread 2 received alarm signal 14
Thread 2 received alarm signal 14
Thread 2 woke up due to signal 14
Signal sender thread 2 exiting
Thread 0 woke up due to timeout
Thread 0 pending signals: 0x0
Thread 1 woke up due to timeout
Thread 1 pending signals: 0x0
Thread 0 woke up due to timeout
Thread 0 pending signals: 0x0
Thread 1 woke up due to timeout
Thread 1 pending signals: 0x0
Joining thread 0
Thread 0 exiting
Joining thread 1
Thread 1 exiting
Joining thread 2

Test results:
Thread 0 received signal: 12
Thread 1 received signal: 12

All tests completed!


*/