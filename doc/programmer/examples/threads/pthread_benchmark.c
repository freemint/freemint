#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// #include "mint_pthread.h"

#define DEFAULT_NUM_THREADS 8
#define DEFAULT_WORK_SIZE 1000000

// Structure to pass data to threads
typedef struct {
    int thread_id;
    long start;
    long end;
    long *result;
} thread_data_t;

// CPU-intensive work function (calculating sum of squares)
void* worker_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long sum = 0;
    
    for (long i = data->start; i < data->end; i++) {
        sum += i * i;
    }
    
    data->result = malloc(sizeof(long));
    *(data->result) = sum;
    return NULL;
}

// Single-threaded version
long single_threaded_work(long work_size) {
    long sum = 0;
    for (long i = 0; i < work_size; i++) {
        sum += i * i;
    }
    return sum;
}

// Multi-threaded version
long multi_threaded_work(long work_size, int num_threads) {
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(num_threads * sizeof(thread_data_t));
    long total_sum = 0;
    
    long chunk_size = work_size / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == num_threads - 1) ? work_size : (i + 1) * chunk_size;
        thread_data[i].result = NULL;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(1);
        }
    }
    
    // Wait for all threads to complete and collect results
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(1);
        }
        total_sum += *(thread_data[i].result);
        free(thread_data[i].result);
    }
    
    free(threads);
    free(thread_data);
    return total_sum;
}

// Function to get current time in nanoseconds
long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t <num>    Number of threads (default: %d)\n", DEFAULT_NUM_THREADS);
    printf("  -w <num>    Work size - number of iterations (default: %d)\n", DEFAULT_WORK_SIZE);
    printf("  -h          Show this help message\n");
}

int main(int argc, char* argv[]) {
    int num_threads = DEFAULT_NUM_THREADS;
    long work_size = DEFAULT_WORK_SIZE;
    int opt;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "t:w:h")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                if (num_threads <= 0) {
                    fprintf(stderr, "Number of threads must be positive\n");
                    exit(1);
                }
                break;
            case 'w':
                work_size = atol(optarg);
                if (work_size <= 0) {
                    fprintf(stderr, "Work size must be positive\n");
                    exit(1);
                }
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    printf("Pthread Benchmark Test\n");
    printf("======================\n");
    printf("Work size: %ld iterations\n", work_size);
    printf("Number of threads: %d\n", num_threads);
    printf("Number of CPU cores: %ld\n", 1);
    printf("\n");
    
    long long start_time, end_time;
    long single_result, multi_result;
    
    // Single-threaded benchmark
    printf("Running single-threaded test...\n");
    start_time = get_time_ns();
    single_result = single_threaded_work(work_size);
    end_time = get_time_ns();
    double single_time = (end_time - start_time) / 1000000.0; // Convert to milliseconds
    
    // Multi-threaded benchmark
    printf("Running multi-threaded test...\n");
    start_time = get_time_ns();
    multi_result = multi_threaded_work(work_size, num_threads);
    end_time = get_time_ns();
    double multi_time = (end_time - start_time) / 1000000.0; // Convert to milliseconds
    
    // Results
    printf("\nResults:\n");
    printf("--------\n");
    printf("Single-threaded result: %ld\n", single_result);
    printf("Multi-threaded result:  %ld\n", multi_result);
    printf("Results match: %s\n", (single_result == multi_result) ? "YES" : "NO");
    printf("\n");
    
    printf("Performance:\n");
    printf("------------\n");
    printf("Single-threaded time: %.2f ms\n", single_time);
    printf("Multi-threaded time:  %.2f ms\n", multi_time);
    printf("Speedup: %.2fx\n", single_time / multi_time);
    printf("Efficiency: %.1f%%\n", (single_time / multi_time) / num_threads * 100);
    
    return 0;
}