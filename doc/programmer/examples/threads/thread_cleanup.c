#include <stdio.h>
#include <stdlib.h>
#include "mint_pthread.h"

void cleanup_handler1(void *arg) {
    printf("Cleanup Handler 1: %s\n", (char*)arg);
}

void cleanup_handler2(void *arg) {
    printf("Cleanup Handler 2: %s\n", (char*)arg);
}

void *thread_func(void *arg) {
    int id = *(int*)arg;
    printf("Thread %d started\n", id);

    /* Push cleanup handlers */
    pthread_cleanup_push(cleanup_handler1, "Resource A");
    pthread_cleanup_push(cleanup_handler2, "Resource B");

    if (id == 1) {
        /* Pop without executing */
        printf("Thread %d popping without execution\n", id);
        pthread_cleanup_pop(0);
    } else {
        /* Pop with execution */
        printf("Thread %d popping with execution\n", id);
        pthread_cleanup_pop(1);
    }

    /* Thread 3 exits without popping */
    if (id != 3) {
        pthread_cleanup_pop(1);
    } 
    else {
        printf("Thread %d exiting with handler active\n", id);
        pthread_exit(NULL);
    }

    return NULL;
}

int main() {
    pthread_t t1, t2, t3;
    int id1 = 1, id2 = 2, id3 = 3;

    pthread_create(&t1, NULL, thread_func, &id1);
    pthread_create(&t2, NULL, thread_func, &id2);
    pthread_create(&t3, NULL, thread_func, &id3);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("All threads completed\n");
    return 0;
}

/*

Thread 1 started
Thread 1 popping without execution
Cleanup Handler 1: Resource A
Thread 2 started
Thread 2 popping with execution
Cleanup Handler 2: Resource B
Cleanup Handler 1: Resource A
Thread 3 started
Thread 3 exiting with handler active
Cleanup Handler 1: Resource A
All threads completed

*/