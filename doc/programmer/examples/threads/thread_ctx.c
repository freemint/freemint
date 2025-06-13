#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "mint_pthread.h"

#define DEFAULT_ITERATIONS 1000
#define DEFAULT_THREADS 5

typedef struct {
    int thread_id;
    long iterations;
    volatile long counter;
    volatile int ready;
    pthread_mutex_t *barrier_mutex;
    pthread_cond_t *barrier_cond;
    volatile int *threads_ready;
    volatile int *start_flag;
    volatile int *threads_done;
} thread_data_t;

void *thread_func(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    long i;
    
    // Signal that this thread is ready
    pthread_mutex_lock(data->barrier_mutex);
    data->ready = 1;
    (*data->threads_ready)++;
    pthread_cond_signal(data->barrier_cond);
    pthread_mutex_unlock(data->barrier_mutex);
    
    // Wait for start signal from main thread
    pthread_mutex_lock(data->barrier_mutex);
    while (*(data->start_flag) == 0) {
        pthread_cond_wait(data->barrier_cond, data->barrier_mutex);
    }
    pthread_mutex_unlock(data->barrier_mutex);
    
    // Perform context switches
    for (i = 0; i < data->iterations; i++) {
        data->counter++;
        pthread_yield(); // Force a context switch
    }
    
    // Signal completion
    pthread_mutex_lock(data->barrier_mutex);
    (*data->threads_done)++;
    pthread_cond_signal(data->barrier_cond);
    pthread_mutex_unlock(data->barrier_mutex);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    clock_t start_time, end_time;
    int i;
    pthread_t threads[DEFAULT_THREADS];
    thread_data_t thread_info[DEFAULT_THREADS];
    long iterations = DEFAULT_ITERATIONS;
    int num_threads = DEFAULT_THREADS;
    volatile int threads_ready = 0;
    volatile int start_flag = 0;
    volatile int threads_done = 0;
    pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t barrier_cond = PTHREAD_COND_INITIALIZER;
    
    // Allow command-line override of parameters
    if (argc > 1) {
        iterations = atol(argv[1]);
        if (iterations <= 0) iterations = DEFAULT_ITERATIONS;
    }
    
    if (argc > 2) {
        num_threads = atoi(argv[2]);
        if (num_threads <= 0 || num_threads > DEFAULT_THREADS) 
            num_threads = DEFAULT_THREADS;
    }
    
    printf("FreeMiNT pthread context switching benchmark\n");
    printf("Testing with %ld iterations per thread, %d threads\n", 
           iterations, num_threads);
    
    // Initialize mutex and condition variable
    if (pthread_mutex_init(&barrier_mutex, NULL) != 0) {
        printf("Error initializing mutex\n");
        return 1;
    }
    
    if (pthread_cond_init(&barrier_cond, NULL) != 0) {
        printf("Error initializing condition variable\n");
        pthread_mutex_destroy(&barrier_mutex);
        return 1;
    }
    
    // Initialize thread data
    for (i = 0; i < num_threads; i++) {
        thread_info[i].thread_id = i + 1;
        thread_info[i].iterations = iterations;
        thread_info[i].counter = 0;
        thread_info[i].ready = 0;
        thread_info[i].barrier_mutex = &barrier_mutex;
        thread_info[i].barrier_cond = &barrier_cond;
        thread_info[i].threads_ready = &threads_ready;
        thread_info[i].start_flag = &start_flag;
        thread_info[i].threads_done = &threads_done;
    }
    
    // Create threads
    printf("Creating %d threads...\n", num_threads);
    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, thread_func, &thread_info[i]) != 0) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
        printf("Created thread %d (tid=%ld)\n", i+1, threads[i]);
    }
    
    // Wait for all threads to be ready
    printf("Waiting for threads to initialize...\n");
    pthread_mutex_lock(&barrier_mutex);
    while (threads_ready < num_threads) {
        pthread_cond_wait(&barrier_cond, &barrier_mutex);
    }
    pthread_mutex_unlock(&barrier_mutex);
    
    // Warm up the system
    printf("Warming up...\n");
    for (i = 0; i < 1000; i++) {
        pthread_yield();
    }
    
    // Start the benchmark
    printf("Starting benchmark...\n");
    start_time = clock();
    
    // Signal threads to start
    pthread_mutex_lock(&barrier_mutex);
    start_flag = 1;
    pthread_cond_broadcast(&barrier_cond);
    pthread_mutex_unlock(&barrier_mutex);
    
    // Wait for all threads to complete
    pthread_mutex_lock(&barrier_mutex);
    while (threads_done < num_threads) {
        pthread_cond_wait(&barrier_cond, &barrier_mutex);
    }
    pthread_mutex_unlock(&barrier_mutex);
    
    end_time = clock();
    
    // Calculate results
    long total_switches = iterations * num_threads;
    double elapsed_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    double switches_per_second = total_switches / elapsed_seconds;
    double time_per_switch_us = (elapsed_seconds * 1000000.0) / total_switches;
    
    printf("\nBenchmark Results:\n");
    printf("Total context switches: %ld\n", total_switches);
    printf("Total time: %.2f seconds\n", elapsed_seconds);
    printf("Context switches per second: %.2f\n", switches_per_second);
    printf("Time per context switch: %.2f microseconds\n", time_per_switch_us);
    
    // Verify that all threads completed their work
    printf("\nVerification:\n");
    for (i = 0; i < num_threads; i++) {
        printf("Thread %d counter: %ld (expected: %ld)\n", 
               thread_info[i].thread_id, thread_info[i].counter, iterations);
    }
    
    // Wait for threads to finish
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Cleanup
    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&barrier_cond);
    
    printf("Benchmark completed successfully\n");
    return 0;
}
