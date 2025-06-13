#include <stdio.h>
#include <stdlib.h>
#include "mint_pthread.h"

/* Thread-specific data key */
pthread_key_t key;

/* Destructor for thread-specific data */
void destructor(void *value)
{
    printf("Destructor called with value: %p\n", value);
    free(value);
}

/* Thread function */
void *thread_func(void *arg)
{
    int thread_num = *(int*)arg;
    
    /* Allocate thread-specific data */
    int *value = malloc(sizeof(int));
    if (!value) {
        printf("Thread %d: Failed to allocate memory\n", thread_num);
        return NULL;
    }
    
    /* Set thread-specific value */
    *value = thread_num * 100;
    pthread_setspecific(key, value);
    
    /* Get and print thread-specific value */
    int *tsd_value = pthread_getspecific(key);
    printf("Thread %d: TSD value = %d\n", thread_num, *tsd_value);
    
    /* Sleep to demonstrate destructor */
    proc_thread_sleep(100);
    
    /* Get and print thread-specific value again */
    tsd_value = pthread_getspecific(key);
    printf("Thread %d: TSD value = %d\n", thread_num, *tsd_value);
    
    return NULL;
}

int main(void)
{
    pthread_t threads[3];
    int thread_nums[3] = {1, 2, 3};
    int i;
    
    /* Create thread-specific data key */
    if (pthread_key_create(&key, destructor) != 0) {
        printf("Failed to create thread-specific data key\n");
        return 1;
    }
    
    /* Create threads */
    for (i = 0; i < 3; i++) {
        if (pthread_create(&threads[i], NULL, thread_func, &thread_nums[i]) != 0) {
            printf("Failed to create thread %d\n", i);
            return 1;
        }
    }
    
    /* Wait for threads to complete */
    for (i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Delete key */
    pthread_key_delete(key);
    
    printf("All threads completed\n");
    return 0;
}