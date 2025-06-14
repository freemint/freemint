#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "mint_pthread.h"

// Global variables
volatile int signal_count = 0;
pthread_mutex_t count_mutex;

// Signal handler for thread-specific signals
void thread_signal_handler(int sig, void *arg) {
    int thread_num = *(int*)arg;
    printf("Thread %ld (arg=%d) received signal %d\n", pthread_self(), thread_num, sig);
    
    pthread_mutex_lock(&count_mutex);
    signal_count++;
    pthread_mutex_unlock(&count_mutex);
    
    // Sleep briefly to test signal stack handling
    pthread_sleep_ms(100);
}

// Process signal handler
void process_signal_handler(int sig) {
    printf("Process received signal %d in thread %ld\n", sig, pthread_self());
    
    pthread_mutex_lock(&count_mutex);
    signal_count++;
    pthread_mutex_unlock(&count_mutex);
}

// Thread function
void *thread_func(void *arg) {
    int thread_num = *(int*)arg;
    int local_arg = thread_num;  // Create a local copy for the signal handler
    
    printf("Thread %d (id=%ld) started\n", thread_num, pthread_self());
    
    // Register thread-specific signal handlers
    proc_thread_signal(PTSIG_HANDLER, SIGUSR1, (long)thread_signal_handler);
    proc_thread_signal(PTSIG_HANDLER_ARG, SIGUSR1, (long)&local_arg);
    
    // Also register for SIGUSR2
    proc_thread_signal(PTSIG_HANDLER, SIGUSR2, (long)thread_signal_handler);
    proc_thread_signal(PTSIG_HANDLER_ARG, SIGUSR2, (long)&local_arg);
    
    // Wait for signals using sigwait
    printf("Thread %d waiting for signals\n", thread_num);
    
    // Create a signal mask for SIGUSR1 and SIGUSR2
    unsigned long mask = (1UL << SIGUSR1) | (1UL << SIGUSR2);
    
    // Wait for signals with timeout
    int sig = proc_thread_signal(PTSIG_WAIT, mask, 5000);  // 5 second timeout
    
    if (sig > 0) {
        printf("Thread %d woke up due to signal %d\n", thread_num, sig);
    } else if (sig == 0) {
        printf("Thread %d timed out waiting for signals\n", thread_num);
    } else {
        printf("Thread %d sigwait error: %d\n", thread_num, sig);
    }
    
    printf("Thread %d exiting\n", thread_num);
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3;
    int t1_arg = 1, t2_arg = 2, t3_arg = 3;
    void *retval;
    
    // Initialize mutex
    pthread_mutex_init(&count_mutex, NULL);
    
    printf("Main: Testing thread signal handling\n");
    
    // Register process signal handler
    signal(SIGINT, process_signal_handler);
    
    // Enable thread-specific signals
    proc_thread_signal(PTSIG_MODE, 1, 0);  // 1 = enable
    
    // Create three threads
    if (pthread_create(&thread1, NULL, thread_func, &t1_arg) != 0) {
        perror("Failed to create thread 1");
        return 1;
    }
    
    if (pthread_create(&thread2, NULL, thread_func, &t2_arg) != 0) {
        perror("Failed to create thread 2");
        return 1;
    }
    
    if (pthread_create(&thread3, NULL, thread_func, &t3_arg) != 0) {
        perror("Failed to create thread 3");
        return 1;
    }
    
    printf("Main: Threads created (tid1=%ld, tid2=%ld, tid3=%ld)\n", 
           thread1, thread2, thread3);
    
    // Give threads time to initialize
    // sleep(1);
    
    // Send signals to specific threads
    printf("Main: Sending SIGUSR1 to thread 1\n");
    proc_thread_signal(PTSIG_KILL, thread1, SIGUSR1);
    
    // sleep(1);
    
    printf("Main: Sending SIGUSR2 to thread 2\n");
    proc_thread_signal(PTSIG_KILL, thread2, SIGUSR2);
    
    // sleep(1);
    
    // Send a process signal
    printf("Main: Sending SIGINT to process\n");
    raise(SIGINT);
    
    // sleep(1);
    
    // Send multiple signals to test handling
    printf("Main: Sending multiple signals to thread 3\n");
    proc_thread_signal(PTSIG_KILL, thread3, SIGUSR1);
    proc_thread_signal(PTSIG_KILL, thread3, SIGUSR2);
    
    // Wait for threads to finish
    printf("Main: Waiting for threads to finish\n");
    pthread_join(thread1, &retval);
    pthread_join(thread2, &retval);
    pthread_join(thread3, &retval);
    
    // Clean up
    pthread_mutex_destroy(&count_mutex);
    
    printf("Main: All threads finished, received %d signals\n", signal_count);
    return 0;
}

/*

[root@atari d]# ./thread_signal2
Main: Testing thread signal handling
Main: Threads created (tid1=1, tid2=2, tid3=3)
Main: Sending SIGUSR1 to thread 1
Main: Sending SIGUSR2 to thread 2
Main: Sending SIGINT to process
Process received signal 2 in thread 0
Thread 1 (id=1) started
Thread 1 waiting for signals
Thread 1 woke up due to signal 29
Thread 1 exiting
Thread 2 (id=2) started
Thread 2 waiting for signals
Thread 2 woke up due to signal 30
Thread 2 exiting
Thread 3 (id=3) started
Thread 3 waiting for signals
Main: Sending multiple signals to thread 3
Main: Waiting for threads to finish
Thread 3 woke up due to signal 30
Thread 3 exiting
Main: All threads finished, received 1 signals

*/