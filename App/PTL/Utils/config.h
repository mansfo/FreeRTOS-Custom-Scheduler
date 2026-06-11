#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include "FreeRTOS.h"
#include <stdio.h>

#define USE_TESTS 0     // If 1, the test suite is run; ele run the user-defined tasks
#define MAX_TASKS 8     // Define max number of tasks

/* Custom enum type to define the possible overrun policies */
typedef enum OverrunPolicy
{
    POLICY_SKIP,
    POLICY_KILL,
    POLICY_CATCH_UP,
    NO_POLICY
} OverrunPolicy_t;


typedef void (*xPointerTask)(void *);

/* Custom type to define a task, specifying the various parameters */
typedef struct ConfTask_t
{
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

/* Custom type to define global parameters of the Scheduler */
typedef struct SchedulerConfig_t
{
    OverrunPolicy_t xGlobalPolicy;      // Global overrun policy to adopt
    bool xTraceEnabled;                 // If enabled, info on the task execution will be printed
    size_t uxMaxTasks;                  // Max number of tasks allowed to be executed
    ConfTask_t xTasks[MAX_TASKS];       // Array containing the configuration of each task
    size_t uxNumTasks;                  // Number of tasks defined
} SchedulerConfig_t;

#endif