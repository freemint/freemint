#include "mint_pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Thread function that can be cancelled
void* cancellable_thread(void* arg) {
    int thread_id = *(int*)arg;
    printf("Thread %d: Starting execution\n", thread_id);
    
    // Set cancellation type to asynchronous (can be cancelled at any time)
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    // Simulate some work
    for (int i = 0; i < 10; i++) {
        printf("Thread %d: Working... iteration %d\n", thread_id, i);
        pthread_sleep_ms(1000);
        
        // Test cancellation point
        pthread_testcancel();
    }
    
    printf("Thread %d: Completed normally\n", thread_id);
    return NULL;
}

// Thread function that demonstrates deferred cancellation
void* deferred_cancel_thread(void* arg) {
    int thread_id = *(int*)arg;
    printf("Thread %d (deferred): Starting execution\n", thread_id);
    
    // Set cancellation type to deferred (default)
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    // Disable cancellation temporarily
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    printf("Thread %d (deferred): Cancellation disabled - doing critical work\n", thread_id);
    pthread_sleep_ms(2000);
    
    // Re-enable cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    printf("Thread %d (deferred): Cancellation re-enabled\n", thread_id);
    
    // Loop with cancellation points
    for (int i = 0; i < 10; i++) {
        printf("Thread %d (deferred): Working... iteration %d\n", thread_id, i);
        pthread_sleep_ms(1000);  // sleep() is a cancellation point
    }
    
    printf("Thread %d (deferred): Completed normally\n", thread_id);
    return NULL;
}

// Cleanup handler function
void cleanup_handler(void* arg) {
    char* msg = (char*)arg;
    printf("Cleanup handler called: %s\n", msg);
}

// Thread with cleanup handler
void* thread_with_cleanup(void* arg) {
    int thread_id = *(int*)arg;
    char cleanup_msg[100];
    snprintf(cleanup_msg, sizeof(cleanup_msg), "Cleaning up thread %d", thread_id);
    
    // Push cleanup handler
    pthread_cleanup_push(cleanup_handler, cleanup_msg);
    
    printf("Thread %d (cleanup): Starting with cleanup handler\n", thread_id);
    
    for (int i = 0; i < 10; i++) {
        printf("Thread %d (cleanup): Working... iteration %d\n", thread_id, i);
        pthread_sleep_ms(1000);
        pthread_testcancel();
    }
    
    printf("Thread %d (cleanup): Completed normally\n", thread_id);
    
    // Pop cleanup handler (execute = 0 means don't execute on normal exit)
    pthread_cleanup_pop(0);
    return NULL;
}

int main() {
    pthread_t threads[3];
    int thread_ids[3] = {1, 2, 3};
    int result;
    
    printf("=== pthread_cancel Test Program ===\n\n");
    
    // Test 1: Basic cancellation
    printf("Test 1: Creating thread with asynchronous cancellation\n");
    result = pthread_create(&threads[0], NULL, cancellable_thread, &thread_ids[0]);
    if (result != 0) {
        fprintf(stderr, "Error creating thread 1: %s\n", strerror(result));
        exit(1);
    }
    
    // Let it run for a bit, then cancel
    // sleep(3);
    printf("Main: Cancelling thread 1\n");
    result = pthread_cancel(threads[0]);
    if (result != 0) {
        fprintf(stderr, "Error cancelling thread 1: %s\n", strerror(result));
    }
    
    // Wait for thread to finish
    void* thread_result;
    result = pthread_join(threads[0], &thread_result);
    if (result != 0) {
        fprintf(stderr, "Error joining thread 1: %s\n", strerror(result));
    }
    
    if (thread_result == PTHREAD_CANCELED) {
        printf("Thread 1: Was successfully cancelled\n");
    } else {
        printf("Thread 1: Completed normally\n");
    }
    
    printf("\n");
    
    // Test 2: Deferred cancellation
    printf("Test 2: Creating thread with deferred cancellation\n");
    result = pthread_create(&threads[1], NULL, deferred_cancel_thread, &thread_ids[1]);
    if (result != 0) {
        fprintf(stderr, "Error creating thread 2: %s\n", strerror(result));
        exit(1);
    }
    
    // Try to cancel immediately (should be deferred)
    // sleep(1);
    printf("Main: Requesting cancellation of thread 2 (deferred)\n");
    result = pthread_cancel(threads[1]);
    if (result != 0) {
        fprintf(stderr, "Error cancelling thread 2: %s\n", strerror(result));
    }
    
    result = pthread_join(threads[1], &thread_result);
    if (result != 0) {
        fprintf(stderr, "Error joining thread 2: %s\n", strerror(result));
    }
    
    if (thread_result == PTHREAD_CANCELED) {
        printf("Thread 2: Was successfully cancelled\n");
    } else {
        printf("Thread 2: Completed normally\n");
    }
    
    printf("\n");
    
    // Test 3: Cancellation with cleanup handler
    printf("Test 3: Creating thread with cleanup handler\n");
    result = pthread_create(&threads[2], NULL, thread_with_cleanup, &thread_ids[2]);
    if (result != 0) {
        fprintf(stderr, "Error creating thread 3: %s\n", strerror(result));
        exit(1);
    }
    
    // sleep(2);
    printf("Main: Cancelling thread 3 (with cleanup)\n");
    result = pthread_cancel(threads[2]);
    if (result != 0) {
        fprintf(stderr, "Error cancelling thread 3: %s\n", strerror(result));
    }
    
    result = pthread_join(threads[2], &thread_result);
    if (result != 0) {
        fprintf(stderr, "Error joining thread 3: %s\n", strerror(result));
    }
    
    if (thread_result == PTHREAD_CANCELED) {
        printf("Thread 3: Was successfully cancelled\n");
    } else {
        printf("Thread 3: Completed normally\n");
    }
    
    printf("\n=== Test completed ===\n");
    return 0;
}