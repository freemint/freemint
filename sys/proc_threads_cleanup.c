/**
 * Thread cleanup handler implementation
 * 
 * This implements pthread_cleanup_push/pthread_cleanup_pop functionality
 * for the FreeMiNT threading system. The kernel manages the cleanup stack,
 * but userspace is responsible for executing the cleanup routines.
 */

#include "proc_threads_cleanup.h"

/* Maximum number of cleanup handlers per thread */
#define MAX_CLEANUP_HANDLERS 32

/**
 * Cleanup handler entry structure
 */
struct cleanup_handler {
    void (*routine)(void*);         /* Cleanup function */
    void *arg;                      /* Argument to cleanup function */
    struct cleanup_handler *next;   /* Next handler in stack */
};

/**
 * Push a cleanup handler onto the current thread's cleanup stack
 * 
 * @param routine The cleanup function to call
 * @param arg Argument to pass to the cleanup function
 * @return 0 on success, negative error code on failure
 */
long thread_cleanup_push(void (*routine)(void*), void *arg) {
    struct proc *p = curproc;
    struct thread *t;
    struct cleanup_handler *handler;
    struct cleanup_handler *current_head;
    int count = 0;
    
    if (!p || !p->current_thread) {
        TRACE_THREAD("thread_cleanup_push: no current thread\n");
        return EINVAL;
    }
    
    if (!routine) {
        TRACE_THREAD("thread_cleanup_push: null routine\n");
        return EINVAL;
    }
    
    t = p->current_thread;
    
    TRACE_THREAD("thread_cleanup_push: thread=%p, routine=%p, arg=%p\n", t, routine, arg);
    
    /* Count existing handlers to prevent stack overflow */
    current_head = (struct cleanup_handler*)t->cleanup_stack;
    while (current_head) {
        count++;
        if (count >= MAX_CLEANUP_HANDLERS) {
            TRACE_THREAD("thread_cleanup_push: too many cleanup handlers\n");
            return ENOMEM;
        }
        current_head = current_head->next;
    }
    
    /* Allocate new handler */
    handler = kmalloc(sizeof(struct cleanup_handler));
    if (!handler) {
        TRACE_THREAD("thread_cleanup_push: failed to allocate handler\n");
        return ENOMEM;
    }
    
    /* Initialize handler */
    handler->routine = routine;
    handler->arg = arg;
    handler->next = (struct cleanup_handler*)t->cleanup_stack;
    
    /* Push onto stack (LIFO order) */
    t->cleanup_stack = handler;
    
    TRACE_THREAD("thread_cleanup_push: pushed handler %p onto stack\n", handler);
    return 0;
}

/**
 * Pop a cleanup handler from the current thread's cleanup stack
 * 
 * @param routine_ptr Pointer to store the routine function pointer
 * @param arg_ptr Pointer to store the argument
 * @return 1 if handler was found and popped, 0 if stack empty, negative on error
 */
long thread_cleanup_pop(void (**routine_ptr)(void*), void **arg_ptr) {
    struct proc *p = curproc;
    struct thread *t;
    struct cleanup_handler *handler;
    
    if (!p || !p->current_thread) {
        TRACE_THREAD("thread_cleanup_pop: no current thread\n");
        return EINVAL;
    }
    
    if (!routine_ptr || !arg_ptr) {
        TRACE_THREAD("thread_cleanup_pop: null pointers\n");
        return EINVAL;
    }
    
    t = p->current_thread;
    handler = (struct cleanup_handler*)t->cleanup_stack;
    
    if (!handler) {
        TRACE_THREAD("thread_cleanup_pop: no cleanup handlers on stack\n");
        *routine_ptr = NULL;
        *arg_ptr = NULL;
        return 0; /* Stack empty, not an error */
    }
    
    TRACE_THREAD("thread_cleanup_pop: thread=%p, handler=%p\n", t, handler);
    
    /* Remove from stack */
    t->cleanup_stack = handler->next;
    
    /* Return handler info to userspace */
    *routine_ptr = handler->routine;
    *arg_ptr = handler->arg;
    
    /* Free the handler structure */
    kfree(handler);
    
    TRACE_THREAD("thread_cleanup_pop: popped routine=%p, arg=%p\n", *routine_ptr, *arg_ptr);
    return 1; /* Handler was popped */
}

/**
 * Get all cleanup handlers for a thread and return them to userspace
 * This is called during thread exit to let userspace execute all handlers
 * 
 * @param t The thread whose cleanup handlers should be retrieved
 * @param handlers Array to store handler info (allocated by caller)
 * @param max_handlers Maximum number of handlers to retrieve
 * @return Number of handlers retrieved, or negative error code
 */
long get_cleanup_handlers(struct thread *t, struct cleanup_info *handlers, int max_handlers) {
    struct cleanup_handler *handler;
    int count = 0;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return EINVAL;
    }
    
    if (!handlers || max_handlers <= 0) {
        return EINVAL;
    }
    
    TRACE_THREAD("get_cleanup_handlers: retrieving cleanup handlers for thread=%p\n", t);
    
    handler = (struct cleanup_handler*)t->cleanup_stack;
    
    /* Collect handlers in execution order (LIFO from stack) */
    while (handler && count < max_handlers) {
        handlers[count].routine = handler->routine;
        handlers[count].arg = handler->arg;
        count++;
        handler = handler->next;
    }
    
    TRACE_THREAD("get_cleanup_handlers: retrieved %d handlers for thread=%p\n", count, t);
    return count;
}

/**
 * Initialize cleanup handler stack for a new thread
 * 
 * @param t The thread to initialize
 * @return 0 on success, error code on failure
 */
int init_thread_cleanup(struct thread *t) {
    if (!t) {
        return EINVAL;
    }
    
    TRACE_THREAD("init_thread_cleanup: initializing cleanup stack for thread=%p\n", t);
    
    /* Initialize cleanup stack to empty */
    t->cleanup_stack = NULL;
    
    return 0;
}

/**
 * Clean up cleanup handler stack when a thread exits
 * This frees the handler structures without executing them
 * (execution is handled by userspace)
 * 
 * @param t The thread being cleaned up
 */
void cleanup_thread_handlers(struct thread *t) {
    struct cleanup_handler *handler;
    struct cleanup_handler *next;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return;
    }
    
    TRACE_THREAD("cleanup_thread_handlers: cleaning up handlers for thread=%p\n", t);
    
    handler = (struct cleanup_handler*)t->cleanup_stack;
    
    /* Free all handlers without executing them */
    while (handler) {
        next = handler->next;
        TRACE_THREAD("cleanup_thread_handlers: freeing handler %p\n", handler);
        kfree(handler);
        handler = next;
    }
    
    /* Clear the cleanup stack */
    t->cleanup_stack = NULL;
}

/**
 * Execute all cleanup handlers for a thread during thread exit
 * This is called automatically when a thread exits
 * 
 * @param t The thread whose cleanup handlers should be executed
 */
void run_cleanup_handlers(struct thread *t) {
    struct cleanup_handler *handler;
    struct cleanup_handler *next;
    
    if (!t || t->magic != CTXT_MAGIC) {
        return;
    }
    
    TRACE_THREAD("run_cleanup_handlers: executing cleanup handlers for thread=%p\n", t);
    
    handler = (struct cleanup_handler*)t->cleanup_stack;
    
    /* Execute handlers in LIFO order (reverse order from push) */
    while (handler) {
        next = handler->next;
        
        if (handler->routine) {
            TRACE_THREAD("run_cleanup_handlers: executing handler %p(arg=%p)\n", 
                        handler->routine, handler->arg);
            
            /* Execute the cleanup routine */
            handler->routine(handler->arg);
        }
        
        /* Free the handler structure */
        kfree(handler);
        handler = next;
    }
    
    /* Clear the cleanup stack */
    t->cleanup_stack = NULL;
    
    TRACE_THREAD("run_cleanup_handlers: finished executing cleanup handlers for thread=%p\n", t);
}