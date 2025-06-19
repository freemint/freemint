#include "proc_threads.h"

#ifndef PROC_THREADS_CANCEL_H
#define PROC_THREADS_CANCEL_H

void check_thread_cancellation(struct thread *t);
void pthread_testcancel_internal(struct thread *t);
void init_thread_cancellation(struct thread *t);
void cleanup_thread_cancellation(struct thread *t);

// System calls
long _cdecl proc_thread_setcancelstate(int state, int *oldstate);
long _cdecl proc_thread_setcanceltype(int type, int *oldtype);
long _cdecl proc_thread_cancel(struct thread *t);
long _cdecl proc_thread_testcancel(void);

#endif //PROC_THREADS_CANCEL_H