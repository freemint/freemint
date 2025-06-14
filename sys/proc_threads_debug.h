#ifndef PROC_THREADS_DEBUG_H
#define PROC_THREADS_DEBUG_H

#include "debug.h"

/* Thread debugging levels */
#define THREAD_DEBUG_NONE     0  /* No thread debugging */
#define THREAD_DEBUG_MINIMAL  1  /* Only critical thread operations */
#define THREAD_DEBUG_NORMAL   2  /* Normal thread operations */
#define THREAD_DEBUG_VERBOSE  3  /* Verbose thread operations */
#define THREAD_DEBUG_ALL      4  /* All thread operations including queue dumps */

/* Current thread debug level - change this to adjust verbosity */
#define THREAD_DEBUG_LEVEL THREAD_DEBUG_NORMAL

#ifdef DEBUG_THREAD
/* Function declaration for thread logging */
extern void debug_to_file(const char *filename, const char *fmt, ...);

/* Only log if current level is >= required level */
#define TRACE_THREAD_LEVEL(level, fmt, ...) \
    if (level <= THREAD_DEBUG_LEVEL) { \
        debug_to_file("c:\\thread.log", fmt, ##__VA_ARGS__); \
    }

/* Backward compatibility */
#define TRACE_THREAD(fmt, ...) TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, fmt, ##__VA_ARGS__)

/* Special macros for common operations */
#define TRACE_THREAD_STATE(t, old_state, new_state) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_MINIMAL, "STATE: Thread %d state change from %d to %d", \
                      (t)->tid, old_state, new_state)

#define TRACE_THREAD_SWITCH(from, to) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_MINIMAL, "SWITCH: Switching threads: %d -> %d", \
                      (from)->tid, (to)->tid)

#define TRACE_THREAD_QUEUE_BEFORE(t) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_ALL, "READY_Q: Current queue before adding thread %d:", (t)->tid)

#define TRACE_THREAD_QUEUE_AFTER(t) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_ALL, "READY_Q: Final queue after adding thread %d:", (t)->tid)

#define TRACE_THREAD_SCHED(fmt, ...) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "SCHED: " fmt, ##__VA_ARGS__)

/* Additional specialized macros for common patterns */
#define TRACE_THREAD_CREATE(t, func, arg) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "CREATE: Thread %d created with func=%p, arg=%p", \
                      (t)->tid, func, arg)

#define TRACE_THREAD_EXIT(t, retval) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_MINIMAL, "EXIT: Thread %d exiting with retval=%p", \
                      (t)->tid, retval)

#define TRACE_THREAD_SLEEP(t, ms, ticks, wake_time) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "SLEEP: Thread %d sleeping for %ld ms (%lu ticks), will wake at tick %lu", \
                      (t)->tid, ms, ticks, wake_time)

#define TRACE_THREAD_WAKEUP(t) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "WAKEUP: Thread %d waking up", (t)->tid)

#define TRACE_THREAD_PRIORITY(t, old_pri, new_pri) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "PRIORITY: Thread %d priority change from %d to %d", \
                      (t)->tid, old_pri, new_pri)

#define TRACE_THREAD_MUTEX(op, t, mutex) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "MUTEX: Thread %d %s mutex %p", \
                      (t)->tid, op, mutex)

#define TRACE_THREAD_JOIN(joiner, target) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_NORMAL, "JOIN: Thread %d waiting for thread %d to exit", \
                      (joiner)->tid, (target)->tid)

#define TRACE_THREAD_ERROR(fmt, ...) \
    TRACE_THREAD_LEVEL(THREAD_DEBUG_MINIMAL, "ERROR: " fmt, ##__VA_ARGS__)

#else
/* No-op versions when debugging is disabled */
#define TRACE_THREAD_LEVEL(level, fmt, ...)
#define TRACE_THREAD(fmt, ...)
#define TRACE_THREAD_STATE(t, old_state, new_state)
#define TRACE_THREAD_SWITCH(from, to)
#define TRACE_THREAD_QUEUE_BEFORE(t)
#define TRACE_THREAD_QUEUE_AFTER(t)
#define TRACE_THREAD_SCHED(fmt, ...)
#define TRACE_THREAD_CREATE(t, func, arg)
#define TRACE_THREAD_EXIT(t, retval)
#define TRACE_THREAD_SLEEP(t, ms, ticks, wake_time)
#define TRACE_THREAD_WAKEUP(t)
#define TRACE_THREAD_PRIORITY(t, old_pri, new_pri)
#define TRACE_THREAD_MUTEX(op, t, mutex)
#define TRACE_THREAD_JOIN(joiner, target)
#define TRACE_THREAD_ERROR(fmt, ...)
#endif

#endif /* PROC_THREADS_DEBUG_H */