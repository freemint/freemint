/**
 * pthread_join_test.c - Test program for FreeMiNT pthread_join implementation
 * 
 * This program creates multiple threads that perform different tasks and
 * then joins them, demonstrating thread creation, termination, and joining.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mint_pthread.h"

#define NUM_THREADS 5
#define ITERATIONS 1000000

/* Thread function that performs a computation */
void *compute_thread(void *arg)
{
    long thread_id = (long)arg;
    long sum = 0;
    
    printf("Thread %ld starting computation...\n", thread_id);
    
    /* Perform a simple computation */
    for (int i = 0; i < ITERATIONS; i++) {
        sum += i;
        
        /* Occasionally yield to other threads */
        if (i % 100000 == 0) {
            pthread_yield();
        }
    }
    
    printf("Thread %ld finished computation, result: %ld\n", thread_id, sum);
    
    /* Return the result as the thread's exit value */
    return (void *)sum;
}

/* Thread function that sleeps for a while */
void *sleep_thread(void *arg)
{
    long thread_id = (long)arg;
    long sleep_time = thread_id * 500; /* Sleep time in milliseconds */
    
    printf("Thread %ld sleeping for %ld ms...\n", thread_id, sleep_time);
    
    /* Sleep for the specified time */
    pthread_sleep_ms(sleep_time);
    
    printf("Thread %ld woke up after sleeping\n", thread_id);
    
    /* Return the sleep time as the thread's exit value */
    return (void *)sleep_time;
}

/**
 * Main function that tests the pthread_join implementation by creating,
 * running, and joining multiple threads. Alternates between compute and
 * sleep threads, collects their results, and demonstrates handling of
 * various error cases, such as joining an invalid thread ID, joining
 * the current thread (which would cause a deadlock), and attempting to
 * join a detached thread.
 */

int main(void)
{
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    void *results[NUM_THREADS];
    int rc;
    long t;
    
    printf("pthread_join test program starting\n");
    
    /* Initialize thread attributes */
    rc = pthread_attr_init(&attr);
    if (rc != 0) {
        printf("ERROR: pthread_attr_init failed with code %d\n", rc);
        return 1;
    }
    
    /* Set thread attributes */
    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc != 0) {
        printf("ERROR: pthread_attr_setdetachstate failed with code %d\n", rc);
        return 1;
    }
    
    /* Create threads */
    for (t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %ld\n", t);
        
        /* Alternate between compute and sleep threads */
        if (t % 2 == 0) {
            rc = pthread_create(&threads[t], &attr, compute_thread, (void *)t);
        } else {
            rc = pthread_create(&threads[t], &attr, sleep_thread, (void *)t);
        }
        
        if (rc != 0) {
            printf("ERROR: pthread_create failed with code %d\n", rc);
            return 1;
        }
    }
    
    /* Free attribute memory */
    rc = pthread_attr_destroy(&attr);
    if (rc != 0) {
        printf("ERROR: pthread_attr_destroy failed with code %d\n", rc);
        return 1;
    }
    
    /* Wait for threads to complete and collect results */
    for (t = 0; t < NUM_THREADS; t++) {
        printf("Joining thread %ld\n", t);
        
        rc = pthread_join(threads[t], &results[t]);
        if (rc != 0) {
            printf("ERROR: pthread_join failed with code %d\n", rc);
            return 1;
        }
        
        printf("Thread %ld joined, returned value: %ld\n", t, (long)results[t]);
    }
    
    /* Test error cases */
    printf("\nTesting error cases:\n");
    
    /* Try to join an invalid thread ID */
    rc = pthread_join(99999, NULL);
    printf("Joining invalid thread ID: %d (expected ESRCH)\n", rc);
    
    /* Try to join the current thread (should detect deadlock) */
    rc = pthread_join(pthread_self(), NULL);
    printf("Joining self: %d (expected EDEADLK)\n", rc);
    
    /* Create a detached thread and try to join it */
    pthread_t detached_thread;
    pthread_attr_t detached_attr;
    
    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    
    rc = pthread_create(&detached_thread, &detached_attr, sleep_thread, (void *)999);
    if (rc == 0) {
        /* Give the thread time to start */
        pthread_sleep_ms(100);
        
        /* Try to join the detached thread (should fail) */
        rc = pthread_join(detached_thread, NULL);
        printf("Joining detached thread: %d (expected EINVAL)\n", rc);
    } else {
        printf("Failed to create detached thread: %d\n", rc);
    }
    
    pthread_attr_destroy(&detached_attr);
    
    printf("\nAll tests completed successfully!\n");
    
    return 0;
}
