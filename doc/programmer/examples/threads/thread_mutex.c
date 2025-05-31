#include <stdio.h>
#include <mint/mintbind.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

struct semaphore {
    int count;
    struct thread *wait_queue;
};

struct mutex {
    int locked;
    struct thread *owner;
    struct thread *wait_queue;
};




// Mutex and semaphore test code

// Global mutex and semaphore variables
struct mutex my_mutex;
struct semaphore my_semaphore;

// Thread function to test mutex
void *mutex_test_thread(void *arg) {
    printf("Mutex test thread started\n");
    
    // Lock the mutex
    mutex_lock(&my_mutex);
    
    printf("Mutex locked by thread %ld\n", pthread_self());
    
    // Sleep for 2 seconds
    sys_p_sleepthread(2000);
    
    // Unlock the mutex
    mutex_unlock(&my_mutex);
    
    printf("Mutex unlocked by thread %ld\n", pthread_self());
    
    return NULL;
}

// Thread function to test semaphore
void *semaphore_test_thread(void *arg) {
    printf("Semaphore test thread started\n");
    
    // Wait on the semaphore
    semaphore_down(&my_semaphore);
    
    printf("Semaphore acquired by thread %ld\n", pthread_self());
    
    // Sleep for 2 seconds
    sys_p_sleepthread(2000);
    
    // Release the semaphore
    semaphore_up(&my_semaphore);
    
    printf("Semaphore released by thread %ld\n", pthread_self());
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    // Initialize the mutex and semaphore
    mutex_init(&my_mutex);
    // semaphore_init(&my_semaphore, 1);
    
    // Create two threads to test the mutex
    pthread_create(&thread1, NULL, mutex_test_thread, NULL);
    pthread_create(&thread2, NULL, mutex_test_thread, NULL);
    
    // // Wait for the threads to finish
    // pthread_join(thread1, NULL);
    // pthread_join(thread2, NULL);
    
    // // Create two threads to test the semaphore
    // pthread_create(&thread1, NULL, semaphore_test_thread, NULL);
    // pthread_create(&thread2, NULL, semaphore_test_thread, NULL);
    
    // // Wait for the threads to finish
    // pthread_join(thread1, NULL);
    // pthread_join(thread2, NULL);
    
    return 0;
}
