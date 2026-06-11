#ifndef __API_TASKS__
#define __API_TASKS__

#include <stdbool.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "task.h"
#include "uart.h"
#include "config.h"

/* Custom type used to define a task in the PTL and the running jobs */
typedef struct PtlTask
{
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

/* Custom type to load some stats of the task, used for the test summary */
typedef struct PtlTaskStats 
{
    uint32_t ulJobReleases;         // Number of job released
    uint32_t ulDeadlineMisses;      // Number of deadlines
    uint32_t ulOverruns;            // Number of overruns
} PtlTaskStats_t;


/* Definition of the various functions */

void vSetTaskRunning(bool xState);
bool xIsTaskRunning(void);
void vSchedulerRequestStop(void);
bool xPTL_OnRelease(PtlTask_t *pxT,uint32_t ulTimeStamp);
void vPTL_OnJobComplete(PtlTask_t *pxT);
void vReleaseJob(PtlTask_t *pxT, TickType_t xNow);
void vInitRTOSPrimitives(void);
void vInit(SchedulerConfig_t *pxCfg);
void vStart();
void vDelayRoutine(void * pvParameters);
void vBusyWaitTask(void *pvParameters);
bool xPTL_GetTaskStats(size_t uxTaskIndex, PtlTaskStats_t *pxStats); 

#endif