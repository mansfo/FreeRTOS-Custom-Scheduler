# PTL Scheduler - API

This directory contains the **Periodic Task Layer (PTL)** API, which provides first-class support for **periodic tasks with deadlines** on top of the FreeRTOS kernel. It implements deadline monitoring, period overrun detection, and three flexible overrun policies without modifying the FreeRTOS kernel itself.

## Files Overview

### api_task.h
Header file defining the public API and core data structures:
- `PtlTask_t` — Runtime state of each periodic task
- `PtlTaskStats_t` — Statistics for task execution analysis
- Function declarations for initialization, release, completion, and stats retrieval

### api_task.c
Main implementation containing:
- **Periodic task management** — Job release, deadline checking, overrun handling
- **Policy implementation** — SKIP, KILL, and CATCH UP handlers
- **Timer rearm logic** — Maintains deadline timer state for efficient checking
- **Synchronization** — Mutex and queue management for thread-safe operations
- **Trace system** — Logging of all scheduler events for debugging/validation

#### Key functions:
- `vInitRTOSPrimitives()` — Creates mutex, binary semaphore, and logging queue
- `vInit(SchedulerConfig_t *pxCfg)` — Configures scheduler with task definitions
- `vStart()` — Releases first jobs ( t₀ = now + 40 ms in this implementation — chosen as the minimum common time that makes all configured tasks start together), creates periodic handler task
- `vPeriodicTaskHandler()` — Background task driving periodic releases
- `xPTL_OnRelease()` — Attempts to release a new job; applies policy on overrun
- `vPTL_OnJobComplete()` — Checks deadline and updates statistics
- `vReleaseJob()` — Internally triggers FreeRTOS task creation for a job
- `vRearmDeadlineTimer()` — Dynamically sets TIMER1 to fire at nearest deadline
- `vCheckDeadline()` — Called after timer ISR; detects deadline misses
 - `vApplicationIdleHook()` — Counts idle ticks used to estimate system idle time with FreeRTOS native idle hook (`configUSE_IDLE_HOOK`) 

### IntTimer.h / IntTimer.c
Hardware timer interface and implementation
- `vInitialiseTimers()` — Configures TIMER0 and TIMER1
- `TIMER0_HANDLER()` — Interrupt handler for periodic releases
- `TIMER1_HANDLER()` — Interrupt handler for deadline detection
- `vSetDeadlineTimer()` — Arms deadline timer with remaining time
- `vStopDeadlineTimer()` — Disarms deadline timer

#### Hardware timer implementation for ARM Cortex-M3:
- **TIMER0** (3 kHz frequency): Drives periodic job releases at configurable intervals
- **TIMER1** : Monitors absolute deadlines; triggers on deadline expiration
- Why separate timer: Allows independent control; deadline timer can be restarmed as jobs complete
- Both timers signal the binary semaphore to wake the scheduler 

## Core Data Structures

### `ConfTask_t` (in ../Utils/config.h)
Defines a single periodic task's configuration:
```c
typedef struct ConfTask_t {
  char *pcName;                   // Name of the task
  xPointerTask xEntry;            // Routine to execute
  void * pvArg;                   // Arguments to the task, used to pass the worst case execution time
  uint32_t ulStackSize;           // Stack size for the task
  size_t uxPriority;              // Task priority
  uint32_t ulPeriod;              // Task period
  uint32_t ulDeadline;            // Task deadline
  uint32_t ulStartOffset;         // Initial delay in ms
  OverrunPolicy_t xPolicy;        // Overrun policy of the task
} ConfTask_t;
```

### `PtlTask_t`
Runtime state maintained by PTL for each task:
```c
typedef struct PtlTask {
  const ConfTask_t *pxCfg;        // Struct describing the task parameters (period, deadline, ... )
  TaskHandle_t xHandle;           // Handler of the task to execute
  uint32_t ulJobId;               // ID of the job that is running
  TickType_t xLastRelease;        // Tick when the last job was released
  TickType_t xAbsDeadline;        // Absolute deadline of the current job
  bool xJobActive;                // Is the job active?
  bool xMissLogged;               // Did a miss happen?
  bool xMissCatchUp;              // Did a miss + catch up happen?
  bool xPolicyActive;             // Does the task have a specific policy defined?
  uint32_t ulTimeline;            // Time reference for the task
  uint32_t ulTotalMisses;         // Counter for the misses (all jobs of this task)
  uint32_t ulTotalOverruns;       // Counter for the overruns (all jobs of this task)
} PtlTask_t;
```

## Synchronization Primitives

- `xBinarySemaphore`
Signal job release or deadline event from interrupt context by TIMER0 or TIMER1 ISR.
`vPeriodicTaskHandler()` waits on this semaphore to process events

- `xMutex`
Protect shared scheduler state from concurrent access. Protects `xPtlTasks[]` array, job counters, and active flags used by task release logic, deadline checking, and statistics retrieval. 

- `xLogQueue`
Thread-safe trace buffer for scheduler events with 100 entries of 64 bytes per message. Logs release, deadline miss, overrun,warning and completion events when `xTraceEnabled=true`. 



## Policy Implementation

When a new job release is requested while the previous job is still active (overrun), `xPTL_OnRelease()` applies one of three policies:

### POLICY_SKIP
- **Action:** Do not release the new job; leave previous job running
- **Implementation:** Sets `pxT->xPolicyActive = true`; returns immediately without calling `vReleaseJob()`
- **Logged as:** `"OVERRUN + SKIP"`

### POLICY_KILL
- **Action:** Immediately terminate the running job and release the new one
- **Implementation:** Immediately terminates the running job via `vTaskDelete`, resets `xJobActive`, and recreates the task to release a new job.
- **Logged as:** `"OVERRUN + KILL"`

### POLICY_CATCH_UP
- **Action:** Release new job immediately while marking previous job as missed
- **Implementation:** Sets `xMissCatchUp=true` and `xJobActive=false` on the previous job, then calls `vReleaseJob()`
- **Logged as:** `"OVERRUN + CATCH_UP"`

All policies log their action to the trace queue and update `ulTotalOverruns` counter for statistics.



## Trace and Statistics

- **Trace output (when enabled):** Timestamped events logged to queue for UART output
  - Format: `[ TIME_MS ] TaskName EVENT [details]`
  - Events: RELEASE, START, COMPLETE, DEADLINE_MISS, OVERRUN + POLICY, ...

- **Statistics retrieval:** `xPTL_GetTaskStats(index, &stats)` fills:
  - `ulJobReleases` — Total jobs released (= `ulJobId`)
  - `ulDeadlineMisses` — Deadline misses detected
  - `ulOverruns` — Overrun events encountered