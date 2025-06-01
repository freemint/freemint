Here's what you should expect to see when running this pthread synchronization test:

## Expected Output and Behavior

**Initial Setup:**
```
Starting pthread synchronization test on MiNT
Main process PID: 1234

Main process: Mutex initialized
Main process: Semaphore initialized with count 1
```

**Mutex Test Section:**
```
=== Testing Mutex (Main Process creates threads) ===
Main process: Created mutex test thread 1 (TID: 5678)
Main process: Created mutex test thread 2 (TID: 5679)
Main process: Waiting for mutex tests to complete...

Mutex test thread 1 started (ID: 5678)
Mutex test thread 2 started (ID: 5679)
Mutex locked by thread 1 (ID: 5678)
[2-second pause - thread 1 holds mutex]
Mutex unlocked by thread 1 (ID: 5678)
Mutex locked by thread 2 (ID: 5679)
[2-second pause - thread 2 holds mutex]
Mutex unlocked by thread 2 (ID: 5679)
```

**Semaphore Test Section:**
```
=== Testing Semaphore (Main Process creates threads) ===
Main process: Created semaphore test thread 1 (TID: 5680)
Main process: Created semaphore test thread 2 (TID: 5681)
Main process: Waiting for semaphore tests to complete...

Semaphore test thread 3 started (ID: 5680)
Semaphore test thread 4 started (ID: 5681)
Semaphore acquired by thread 3 (ID: 5680)
[2-second pause - thread 3 holds semaphore]
Semaphore released by thread 3 (ID: 5680)
Semaphore acquired by thread 4 (ID: 5681)
[2-second pause - thread 4 holds semaphore]
Semaphore released by thread 4 (ID: 5681)
```

**Cleanup:**
```
Main process: Cleaning up resources...
Main process: Mutex destroyed successfully
Main process: Semaphore destroyed successfully
Main process: Test completed successfully!
```

## Key Behaviors to Observe

**Mutex Synchronization:**
- Only ONE thread can hold the mutex at a time
- Thread 2 will wait until Thread 1 releases the mutex
- You'll see a clear 2-second gap between "unlocked by thread 1" and "locked by thread 2"
- This demonstrates **mutual exclusion**

**Semaphore Synchronization:**
- Similar behavior since semaphore is initialized with count=1
- Acts like a binary semaphore (mutex-like behavior)
- Thread 4 waits until Thread 3 releases the semaphore
- This demonstrates **resource counting** (even with count=1)

**Timing:**
- Each thread holds its resource for 2 seconds
- Total mutex test time: ~4+ seconds
- Total semaphore test time: ~4+ seconds
- Main process waits 6 seconds between test phases

**Thread Ordering:**
- Thread creation order may not match execution order
- The MiNT scheduler determines which thread runs first
- You might see threads start in different orders than created

## What This Proves

1. **Thread Creation Works**: Pexec with PE_THREAD successfully creates threads
2. **Synchronization Works**: MiNT's THREAD_OP_MUTEX_LOCK/UNLOCK prevent race conditions
3. **Semaphores Work**: MiNT's THREAD_OP_SEM_WAIT/POST control resource access
4. **System Calls Work**: The trap_1_w* wrappers correctly interface with MiNT

If you see interleaved or simultaneous access (both threads holding resources at once), that would indicate a problem with the MiNT threading system calls.