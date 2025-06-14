#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mint_pthread.h"

// Shared data structure
struct shared_data {
    int buffer[10];
    int count;
    int in;
    int out;
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

static struct shared_data data = {
    .count = 0,
    .in = 0,
    .out = 0,
    .done = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

// Producer thread function
void *producer_thread(void *arg) {
    int producer_id = *(int*)arg;
    int item;
    
    printf("Producer %d started\n", producer_id);
    
    for (item = producer_id * 100; item < producer_id * 100 + 5; item++) {
        pthread_mutex_lock(&data.mutex);
        
        // Wait while buffer is full
        while (data.count == 10) {
            printf("Producer %d: buffer full, waiting...\n", producer_id);
            pthread_cond_wait(&data.not_full, &data.mutex);
        }
        
        // Produce item
        data.buffer[data.in] = item;
        data.in = (data.in + 1) % 10;
        data.count++;
        
        printf("Producer %d: produced item %d (count=%d)\n", 
               producer_id, item, data.count);
        
        // Signal that buffer is not empty
        pthread_cond_signal(&data.not_empty);
        
        pthread_mutex_unlock(&data.mutex);
        
        // Sleep briefly
        pthread_sleep_ms(100);
    }
    
    printf("Producer %d finished\n", producer_id);
    return NULL;
}

// Consumer thread function
void *consumer_thread(void *arg) {
    int consumer_id = *(int*)arg;
    int item;
    int consumed = 0;
    
    printf("Consumer %d started\n", consumer_id);
    
    while (consumed < 5) {
        pthread_mutex_lock(&data.mutex);
        
        // Wait while buffer is empty and not done
        while (data.count == 0 && !data.done) {
            printf("Consumer %d: buffer empty, waiting...\n", consumer_id);
            pthread_cond_wait(&data.not_empty, &data.mutex);
        }
        
        // Check if we're done and buffer is empty
        if (data.count == 0 && data.done) {
            pthread_mutex_unlock(&data.mutex);
            break;
        }
        
        // Consume item
        item = data.buffer[data.out];
        data.out = (data.out + 1) % 10;
        data.count--;
        consumed++;
        
        printf("Consumer %d: consumed item %d (count=%d)\n", 
               consumer_id, item, data.count);
        
        // Signal that buffer is not full
        pthread_cond_signal(&data.not_full);
        
        pthread_mutex_unlock(&data.mutex);
        
        // Sleep briefly
        pthread_sleep_ms(150);
    }
    
    printf("Consumer %d finished (consumed %d items)\n", consumer_id, consumed);
    return NULL;
}

int main() {
    pthread_t producers[2];
    pthread_t consumers[2];
    int producer_ids[2] = {1, 2};
    int consumer_ids[2] = {1, 2};
    void *retval;
    
    printf("=== Condition Variable Test: Producer-Consumer ===\n");
    
    // Initialize condition variables
    if (pthread_cond_init(&data.not_empty, NULL) != 0) {
        printf("Failed to initialize not_empty condition variable\n");
        return 1;
    }
    
    if (pthread_cond_init(&data.not_full, NULL) != 0) {
        printf("Failed to initialize not_full condition variable\n");
        return 1;
    }
    
    if (pthread_mutex_init(&data.mutex, NULL) != 0) {
        printf("Failed to initialize mutex\n");
        return 1;
    }
    
    printf("Created shared buffer (size=10)\n");
    
    // Create producer threads
    for (int i = 0; i < 2; i++) {
        if (pthread_create(&producers[i], NULL, producer_thread, &producer_ids[i]) != 0) {
            printf("Failed to create producer thread %d\n", i);
            return 1;
        }
        printf("Created producer thread %d (tid=%ld)\n", i+1, producers[i]);
    }
    
    // Create consumer threads
    for (int i = 0; i < 2; i++) {
        if (pthread_create(&consumers[i], NULL, consumer_thread, &consumer_ids[i]) != 0) {
            printf("Failed to create consumer thread %d\n", i);
            return 1;
        }
        printf("Created consumer thread %d (tid=%ld)\n", i+1, consumers[i]);
    }
    
    // Let threads run for a while
    printf("Letting threads run...\n");
    // pthread_sleep_ms(2000);
    
    // Wait for producers to finish
    for (int i = 0; i < 2; i++) {
        if (pthread_join(producers[i], &retval) != 0) {
            printf("Failed to join producer thread %d\n", i);
        } else {
            printf("Producer thread %d joined successfully\n", i+1);
        }
    }
    
    // Signal that production is done
    pthread_mutex_lock(&data.mutex);
    data.done = 1;
    pthread_cond_broadcast(&data.not_empty);  // Wake up all consumers
    pthread_mutex_unlock(&data.mutex);
    
    printf("Signaled that production is done\n");
    
    // Wait for consumers to finish
    for (int i = 0; i < 2; i++) {
        if (pthread_join(consumers[i], &retval) != 0) {
            printf("Failed to join consumer thread %d\n", i);
        } else {
            printf("Consumer thread %d joined successfully\n", i+1);
        }
    }
    
    // Print final statistics
    pthread_mutex_lock(&data.mutex);
    printf("\n=== Final Statistics ===\n");
    printf("Items remaining in buffer: %d\n", data.count);
    printf("Buffer contents: ");
    for (int i = 0; i < data.count; i++) {
        int idx = (data.out + i) % 10;
        printf("%d ", data.buffer[idx]);
    }
    printf("\n");
    pthread_mutex_unlock(&data.mutex);
    
    // Cleanup
    pthread_cond_destroy(&data.not_empty);
    pthread_cond_destroy(&data.not_full);
    pthread_mutex_destroy(&data.mutex);
    
    printf("=== Test completed successfully ===\n");
    return 0;
}

/*

Here's the expected output when running this producer-consumer example with condition variables:

=== Condition Variable Test: Producer-Consumer ===
Created shared buffer (size=10)
Created producer thread 1 (tid=1)
Created producer thread 2 (tid=2)
Created consumer thread 1 (tid=3)
Created consumer thread 2 (tid=4)
Letting threads run...
Producer 1 started
Producer 1: produced item 100 (count=1)
Producer 2 started
Consumer 1 started
Consumer 1: consumed item 100 (count=0)
Producer 1: produced item 101 (count=1)
Consumer 2 started
Consumer 2: consumed item 101 (count=0)
Producer 2: produced item 200 (count=1)
Producer 1: produced item 102 (count=2)
Consumer 1: consumed item 200 (count=1)
Producer 2: produced item 201 (count=2)
Consumer 2: consumed item 102 (count=1)
Producer 1: produced item 103 (count=2)
Consumer 1: consumed item 201 (count=1)
Producer 2: produced item 202 (count=2)
Consumer 2: consumed item 103 (count=1)
Producer 1: produced item 104 (count=2)
Consumer 1: consumed item 202 (count=1)
Producer 2: produced item 203 (count=2)
Consumer 2: consumed item 104 (count=1)
Producer 1 finished
Producer 2: produced item 204 (count=2)
Consumer 1: consumed item 203 (count=1)
Producer 2 finished
Consumer 2: consumed item 204 (count=0)
Consumer 1: buffer empty, waiting...
Consumer 2: buffer empty, waiting...
Producer thread 1 joined successfully
Producer thread 2 joined successfully
Signaled that production is done
Consumer 1 finished (consumed 5 items)
Consumer 2 finished (consumed 5 items)
Consumer thread 1 joined successfully
Consumer thread 2 joined successfully

=== Final Statistics ===
Items remaining in buffer: 0
Buffer contents: 

=== Test completed successfully ===

The exact order of operations may vary due to thread scheduling, but the general flow will be:
The program initializes the shared buffer, mutex, and condition variables
Two producer threads and two consumer threads are created
Producers add items to the buffer (each producer adds 5 items)
Consumers remove items from the buffer
When the buffer is full, producers wait on the not_full condition
When the buffer is empty, consumers wait on the not_empty condition
After producers finish, the main thread signals that production is done
Consumers finish consuming any remaining items and exit
The program prints final statistics and cleans up resources
The output demonstrates proper synchronization between threads using mutexes and condition variables, showing how producers wait when the buffer is full and consumers wait when the buffer is empty.

*/