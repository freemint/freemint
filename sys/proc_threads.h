#include "libkern/libkern.h" /* memset and memcpy */

#include "proc.h"
#include "mint/signal.h"
#include "timeout.h"
#include "arch/context.h"
#include "mint/arch/asm_spl.h"
#include "kmemory.h"
#include "kentry.h"

#define THREAD_SWITCH_TIMEOUT_MS 100  /* 100ms timeout for thread switches */
#define MAX_SWITCH_RETRIES 3

/* Thread scheduling constants */
#define THREAD_MIN_TIMESLICE 4        /* Minimum timeslice in ticks (20ms) */
#define THREAD_RR_TIMESLICE 20        /* Round-robin timeslice in ticks (100ms) */
#define THREAD_PREEMPT_INTERVAL_TICKS 10  /* Preemption interval in ticks (100ms) */
#define MAX_THREAD_PRIORITY 99        /* Maximum thread priority (POSIX compliant) */
#define THREAD_CREATION_PRIORITY_BOOST 3  // Priority boost for newly created threads

/* Thread state management */
void atomic_thread_state_change(struct thread *t, int new_state);

/* Ready queue management */
void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);
int is_in_ready_queue(struct thread *t);
void add_to_sleep_queue(struct thread *t);
void remove_from_sleep_queue(struct thread *t);
int is_in_sleep_queue(struct thread *t);

/* Thread scheduling */
void schedule(void);

/* Checks for pending signals in a thread, returns signal number or 0 */
int check_thread_signals(struct thread *t);
/* Thread-aware version of raise(), delivers signal to appropriate thread or process */
int thread_aware_raise(struct proc *p, int sig);
/* Delivers a signal to a specific thread, returns 1 if delivered, 0 otherwise */
int deliver_signal_to_thread(struct proc *p, struct thread *t, int sig);
/* Processes a signal for a thread using thread-specific or process-level handlers */
void handle_thread_signal(struct thread *t, int sig);
/* Trampoline function to call thread signal handlers with proper context management */
void thread_signal_trampoline(int sig, void *arg);
/* Saves thread execution context before signal handling */
void save_thread_context(struct thread *t, struct thread_sigcontext *ctx);
/* Restores thread execution context after signal handling */
void restore_thread_context(struct thread *t, struct thread_sigcontext *ctx);
/* Thread timeout handling */
void thread_timeout_handler(PROC *p, long arg);

void update_thread_timeslice(struct thread *t);
