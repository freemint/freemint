#include <stdio.h>
#include <mint/mintbind.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "mint_pthread.h"

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
        proc_thread_sleep(2700);
        
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

    pthread_tryjoin(thread1, NULL);
    pthread_tryjoin(thread2, NULL);

    printf("Main: all threads finished, counter final = %d\n", counter);
    
    return 0;
}
