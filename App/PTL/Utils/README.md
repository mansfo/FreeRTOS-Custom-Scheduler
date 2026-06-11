# Utils Folder Documentation

## Overview
The **Utils** folder contains essential utility modules for scheduler configuration and serial communication of the PTL framework. It provides centralized data structures and basic functions to manage task execution and UART communication.

## Folder Contents

### 1. **config.h**
Defines the data structures and fundamental system configurations.

**Main Content:**
- **OverrunPolicy_t**: Enum defining possible overrun handling policies for tasks:
  - `POLICY_SKIP`: Skips the task instance if it runs late
  - `POLICY_KILL`: Terminates the running task
  - `POLICY_CATCH_UP`: Recovers execution of skipped tasks
  - `NO_POLICY`: No particular policy

- **ConfTask_t**: Structure defining the configuration of a single task:
  - `pcName`: Task name
  - `xEntry`: Pointer to the function to execute
  - `pvArg`: Arguments passed to the task
  - `ulStackSize`: Stack size
  - `uxPriority`: Task priority
  - `ulPeriod`: Execution period (ms)
  - `ulDeadline`: Task deadline (ms)
  - `ulStartOffset`: Initial delay (ms)
  - `xPolicy`: Task-specific overrun policy

- **SchedulerConfig_t**: Global structure for scheduler configuration:
  - `xGlobalPolicy`: Global overrun policy
  - `xTraceEnabled`: Enables execution tracing
  - `uxMaxTasks`: Maximum number of tasks
  - `xTasks`: Array of task configurations
  - `uxNumTasks`: Actual number of defined tasks

**Constants:**
- `USE_TESTS`: Activates/deactivates the test suite (default: 0)
- `MAX_TASKS`: Maximum number of supported tasks (default: 8)

### 2. **ptl_config.h**
Support header file that aggregates necessary dependencies.

**Declared Dependencies:**
- Includes `config.h` to access data structures
- Includes `api_task.h` and `api_test.h` for framework integration
- Declares `extern SchedulerConfig_t cfg` for global configuration access

### 3. **ptl_config.c**
Implementation of the scheduler configuration with task parameters.

**Content:**
- **cfg Instance**: Global scheduler configuration with:
  - Global policy: `POLICY_SKIP`
  - Tracing enabled
  - 8 example tasks (Task_A to Task_H) with variable periods and deadlines

**Example Task Configuration:**
```c
{
    .pcName = "Task_A",
    .xEntry = vBusyWaitTask,
    .pvArg = (void*)NULL,
    .ulStackSize = 200,
    .uxPriority = tskIDLE_PRIORITY,
    .ulPeriod = 100,      // Period of 100ms
    .ulDeadline = 50,     // Deadline of 50ms
    .xPolicy = NO_POLICY
}
```

### 4. **uart.h**
Header file defining the interface for UART communication.

### 5. **uart.c**
Implementation of UART0 drivers.

## Dependencies
- **FreeRTOS**: For task-related data structures and priorities
- **CMSIS**: For access to ARM hardware definitions
- **api_task.h / api_test.h**: PTL framework modules