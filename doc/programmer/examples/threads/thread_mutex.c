#include <stdio.h>
#include <mint/mintbind.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "mint_pthread.h"

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
        proc_thread_sleep(2000);
        
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
        proc_thread_sleep(2000);
        
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