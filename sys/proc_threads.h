#include "libkern/libkern.h" /* memset and memcpy */

#include "proc.h"
#include "mint/signal.h"
#include "timeout.h"
#include "arch/context.h"
#include "mint/arch/asm_spl.h"
#include "kmemory.h"
#include "kentry.h"

/* Thread state management */
void atomic_thread_state_change(struct thread *t, int new_state);

/* Ready queue management */
void add_to_ready_queue(struct thread *t);
void remove_from_ready_queue(struct thread *t);

/* Thread scheduling */
void schedule(void);

/* Thread timeout handling */
void thread_timeout_handler(PROC *p, long arg);

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