#include "api_task.h"
#include "IntTimer.h"

SemaphoreHandle_t xBinarySemaphore, xMutex;
QueueHandle_t xLogQueue;

static OverrunPolicy_t xGlobalPolicy ;
static bool xTrace = 0;
static size_t uxMaxTasks,uxNumTasks;
static ConfTask_t xTasks[MAX_TASKS];
static PtlTask_t xPtlTasks[MAX_TASKS];
static uint32_t ulT0=0;
volatile uint64_t ullGlobalIdleCount = 0;
static volatile bool xTaskRunning = false;

/* Function used to count how much time the system was idle */
void vApplicationIdleHook(void)
{
    ullGlobalIdleCount++;
    __asm volatile("wfi");
}

/* Simple routine used to simulate a task working for some time */
void vDelayRoutine(void * pvParameters) 
{
    PtlTask_t *pxT = (PtlTask_t *)pvParameters;
    uint32_t ulCounter = (uint32_t)pxT->pxCfg->pvArg;
    ulCounter = ulCounter * configCPU_CLOCK_HZ / ( 5 * 10);
    asm volatile(
        "mov r1, %0 \n"
        "delay: \n"
        "subs r1, #1 \n"
        "nop \n"
        "nop \n"
        "bne delay \n"
        :
        : "r" (ulCounter)
    );
}

/* Another simple routine that can be used to simulate a task working for the specified time */
void vBusyWaitTask(void *pvParameters) 
{
    PtlTask_t *pxTt = (PtlTask_t *)pvParameters;
    TickType_t xWorkTicks = pdMS_TO_TICKS((uint32_t)pxTt->pxCfg->pvArg);
    TickType_t xLastWake = xTaskGetTickCount();

    for (TickType_t i = 0; i < xWorkTicks; i++)
    {
        while (xTaskGetTickCount() == xLastWake)
        {
            __asm volatile("nop");
        }

        xLastWake = xTaskGetTickCount();
    }
}

/* Function that handles the deadline timers */
void vRearmDeadlineTimer(uint32_t ulNow)
{
    bool xFound=false;
    uint32_t ulDeadlineMin = portMAX_DELAY;

    xSemaphoreTake(xMutex, portMAX_DELAY);

    for(size_t uxI=0;uxI<uxNumTasks;uxI++)
    {
        if(xPtlTasks[uxI].xJobActive && xPtlTasks[uxI].xAbsDeadline < ulDeadlineMin)
        {
            ulDeadlineMin = xPtlTasks[uxI].xAbsDeadline;
            xFound = true;
        }
    }

    xSemaphoreGive(xMutex);

    if (xFound && ulDeadlineMin > ulNow)
    {
        vSetDeadlineTimer((ulDeadlineMin-ulNow)*(configCPU_CLOCK_HZ / 1000UL));
    }
    else 
    {
        vStopDeadlineTimer();
    }
}

/* Function that checks if the tasks have missed their deadline */
void vCheckDeadline(uint32_t ulNow)
{
    if(!xIsTaskRunning()) return;
    xSemaphoreTake(xMutex, portMAX_DELAY);
    for(size_t uxI=0;uxI<uxNumTasks;uxI++)
    {
        if (xPtlTasks[uxI].xJobActive && ulNow > xPtlTasks[uxI].xAbsDeadline && !xPtlTasks[uxI].xMissLogged) 
        {
            xPtlTasks[uxI].xMissLogged = true;
            xPtlTasks[uxI].ulTotalMisses += 1;
            if(xTrace)
            {
                char pcMsg[64];
                snprintf(pcMsg, sizeof(pcMsg), "[ %4lu ] %s DEADLINE_MISS (D = %lu)\n", (unsigned long) ulNow, xPtlTasks[uxI].pxCfg->pcName, (unsigned long) xPtlTasks[uxI].xAbsDeadline);
                if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
                {
                    UART_printf("WARNING: xLogQueue is full!");
                }
            }
        }
    }
    xSemaphoreGive(xMutex);
}

/* Function that handles the release of the periodic tasks */
void vPeriodicTaskHandler(void *pvParameters)
{
    (void) pvParameters;
    TickType_t xNow;
    char pcMsg[64];
    static uint32_t ulLastStatsTime = 0;

    for(;;)
    {    
        if( xSemaphoreTake(xBinarySemaphore,portMAX_DELAY) == pdTRUE)
        {
            xNow = pdTICKS_TO_MS(xTaskGetTickCount());
            if ((xNow - ulLastStatsTime) >= 1000) 
            {
                if(xTrace)
                {
                    char pcStatMsg[64];
                    snprintf(pcStatMsg, sizeof(pcStatMsg), "[ %4lu ] SYSTEM_IDLE_TICKS: %lu\n", (unsigned long)xNow, (unsigned long)ullGlobalIdleCount);
                    if(xQueueSend(xLogQueue,pcStatMsg,0) != pdPASS)
                    {
                        UART_printf("WARNING: xLogQueue is full!");
                    }
                }
                ullGlobalIdleCount = 0;
                ulLastStatsTime = xNow;
            }
        
            for(size_t uxI =0; uxI<uxNumTasks; uxI++)
            {
                if (xIsTaskRunning() && xNow >= xPtlTasks[uxI].ulTimeline) 
                {
                    if(xPTL_OnRelease(&xPtlTasks[uxI],xNow))
                    {
                        xPtlTasks[uxI].ulTimeline +=xPtlTasks[uxI].pxCfg->ulPeriod;
                    }
                }
            }
            if(xIsTaskRunning())
            {
                vCheckDeadline(xNow);
                vRearmDeadlineTimer(xNow);
            }

            else 
            {
                vStopDeadlineTimer();
            }

            for(size_t uxI = 0; uxI < 5; uxI++)
            {
                if (xQueueReceive(xLogQueue, pcMsg, 0) != pdPASS)
                {
                    break; 
                }
                UART_printf(pcMsg);
            }
        }
    }
}

/* Function that initializes the RTOS primitives in the system */
void vInitRTOSPrimitives(void)
{
    xMutex = xSemaphoreCreateMutex();
    xBinarySemaphore = xSemaphoreCreateBinary();
    xLogQueue = xQueueCreate(100,64);
    if ( xBinarySemaphore == NULL || xMutex == NULL)
    {
        UART_printf("ERROR in Semaphores'creation.\n");
    }
    vInitialiseTimers();
}

/* Function that defines global parameters for the system and that defines the tasks to run */
void vInit(SchedulerConfig_t *pxCfg)
{
    xGlobalPolicy = pxCfg->xGlobalPolicy;
    xTrace = pxCfg->xTraceEnabled;
    uxMaxTasks = pxCfg->uxMaxTasks;
    uxNumTasks = pxCfg->uxNumTasks;
    for (size_t uxI = 0; uxI<uxNumTasks; uxI++)
    {
        xTasks[uxI] = pxCfg->xTasks[uxI];
        if (xTasks[uxI].xPolicy == NO_POLICY)
        {
            xTasks[uxI].xPolicy = xGlobalPolicy;
        }
        xPtlTasks[uxI] = (PtlTask_t)
        {
            .pxCfg = &xTasks[uxI],
            .ulJobId = 0,
            .xJobActive = false,
            .xMissLogged = false,
            .ulTimeline = 0,
            .xMissCatchUp = false
        };
    }
}

/* Wrapper function used to release tasks inside the system */
static void prvWrapper(void *pvParameters) 
{
    PtlTask_t *pxT = (PtlTask_t *)pvParameters;
    
    for(;;) 
    {
        if (!xIsTaskRunning())
        {
            vTaskDelete(NULL);
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xSemaphoreTake(xMutex, portMAX_DELAY);
        pxT->xJobActive = true;
        xSemaphoreGive(xMutex);
        TickType_t xNow = xTaskGetTickCount();
        if(xTrace)
        {
            char pcMsg[64];
            snprintf(pcMsg, sizeof(pcMsg), "[ %4lu ] %s START\n", (unsigned long)pdTICKS_TO_MS(xNow), pxT->pxCfg->pcName);
            if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
            {
                UART_printf("WARNING: xLogQueue is full!");
            }
        }
        pxT->pxCfg->xEntry(pxT); 
        vPTL_OnJobComplete(pxT);
        taskYIELD();
    }
}

/* Function called to define a global starting time t0 and to create the defined tasks */
void vStart(void)
{
    if(!xIsTaskRunning()) 
    {
        vSetTaskRunning(true);
    }
    char pcBuffer[90];
    ulT0=pdTICKS_TO_MS(xTaskGetTickCount())+40;
    if(xTrace)
    {
        snprintf(pcBuffer, sizeof(pcBuffer), "--- SYSTEM START t0: %lu ---\n", (unsigned long)ulT0);
        UART_printf(pcBuffer);
    }
    for (size_t uxI = 0; uxI<uxNumTasks; uxI++)
    {       
        xPtlTasks[uxI].ulTimeline = pdMS_TO_TICKS(ulT0 + xTasks[uxI].ulStartOffset);   // Task starts at t0 + offset (default 0)
        if(xTaskCreate(prvWrapper,xTasks[uxI].pcName,xTasks[uxI].ulStackSize*6, (void*)&xPtlTasks[uxI],xTasks[uxI].uxPriority,&xPtlTasks[uxI].xHandle) != pdPASS)
        {
            UART_printf("Task creation error.\n");
        }
        if(xTrace)
        {
            snprintf(pcBuffer, sizeof(pcBuffer), "Name: %s, Priority %d, WCET: %d, Period: %ld, D: %ld, Offset: %ld, Policy: %d\n",
                xTasks[uxI].pcName,xTasks[uxI].uxPriority,(int)xTasks[uxI].pvArg,xTasks[uxI].ulPeriod,xTasks[uxI].ulDeadline, xTasks[uxI].ulStartOffset, xTasks[uxI].xPolicy);
            UART_printf(pcBuffer);
        }
    }
    if (xTaskCreate(vPeriodicTaskHandler,"Periodic_Dealer",configMINIMAL_STACK_SIZE * 2,NULL,configMAX_SYSCALL_INTERRUPT_PRIORITY -1 ,NULL) != pdPASS)
    {
        UART_printf("Task creation error.\n");
    }
}

/* Function that handles the release of a new job of a specified task */
bool xPTL_OnRelease(PtlTask_t *pxT, uint32_t ulTimeStamp)
{
    char pcMsg[64];
    xSemaphoreTake(xMutex, portMAX_DELAY);
    bool xIsActive = pxT->xJobActive;
    xSemaphoreGive(xMutex);

    uint32_t ulNow = ulTimeStamp;
    OverrunPolicy_t xPolicy = pxT->pxCfg->xPolicy;
    if (xPolicy == NO_POLICY) 
    {
        xPolicy = xGlobalPolicy;
    }

    if (!xIsActive) 
    {   
        vReleaseJob(pxT,ulNow);
        pxT->xPolicyActive = false;
        return true;
    }                
    xSemaphoreTake(xMutex, portMAX_DELAY);
    pxT->ulTotalOverruns++;
    xSemaphoreGive(xMutex);
    switch (xPolicy) 
    {
        case POLICY_KILL:
            if(xTrace)
            {
                snprintf(pcMsg, sizeof(pcMsg),"[ %4lu ] %s OVERRUN + KILL\n", (unsigned long)ulNow, pxT->pxCfg->pcName);
                if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
                {
                    UART_printf("WARNING: xLogQueue is full!");
                }
            }
            if (pxT->xHandle != NULL) 
            {
                vTaskDelete(pxT->xHandle);  // Delete the Task
                pxT->xHandle = NULL; 
            }
            xSemaphoreTake(xMutex, portMAX_DELAY);
            pxT->xJobActive = false; 
            xSemaphoreGive(xMutex);

            // Re-create the task
            if(xTaskCreate(prvWrapper, 
                pxT->pxCfg->pcName, 
                pxT->pxCfg->ulStackSize * 6, 
                (void*)pxT, 
                pxT->pxCfg->uxPriority, 
                &pxT->xHandle) != pdPASS)
            {
                UART_printf("Error recreating task during KILL\n");
            }
            // Release a new job
            vReleaseJob(pxT, ulNow);
            return true;

        case POLICY_SKIP:
            if(!pxT->xPolicyActive)
            {
                if(xTrace)
                {
                    snprintf(pcMsg, sizeof(pcMsg), "[ %4lu ] %s OVERRUN + SKIP\n", (unsigned long)ulNow, pxT->pxCfg->pcName);
                    if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
                    {
                        UART_printf("WARNING: xLogQueue is full!");
                    }
                }
                pxT->xPolicyActive = true;
            }  
            return true;

        case POLICY_CATCH_UP:
            if(xTrace)
            {
                snprintf(pcMsg, sizeof(pcMsg),"[ %4lu ] %s OVERRUN + CATCH UP\n", (unsigned long)ulNow, pxT->pxCfg->pcName);
                if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
                {
                    UART_printf("WARNING: xLogQueue is full!");
                }
            }

            xSemaphoreTake(xMutex, portMAX_DELAY);
            pxT->xMissCatchUp = true;
            pxT->xJobActive = false;
            xSemaphoreGive(xMutex);
            vReleaseJob(pxT, ulNow);
            return true;
        
        default:
            return true;
    }
}

/* Function that handles the job after it completed its execution */
void vPTL_OnJobComplete(PtlTask_t *pxT)
{
    if (!pxT->xJobActive && !pxT->xMissCatchUp) 
    {
        return;
    }

    uint32_t ulNow = pdTICKS_TO_MS(xTaskGetTickCount());
    bool xMissCatchUp;
   

    xSemaphoreTake(xMutex, portMAX_DELAY);
    xMissCatchUp = pxT->xMissCatchUp;
    if (xIsTaskRunning() ) 
    {
        if (!pxT->xMissLogged && ulNow > pxT->xAbsDeadline) 
        {
            pxT->ulTotalMisses++; 
            pxT->xMissLogged = true; 
        }
    }
    pxT->xMissCatchUp = false;
    pxT->xJobActive = false;
    
    xSemaphoreGive(xMutex);
    char pcMsg[64];
    if(xTrace)
    {
        if (xMissCatchUp)
        {
            snprintf(pcMsg, sizeof(pcMsg), "[ %4lu ] %s COMPLETE (CATCH UP)\n", (unsigned long)ulNow, pxT->pxCfg->pcName);
        }
        else if (pxT->xPolicyActive && pxT->pxCfg->xPolicy == POLICY_SKIP)
        { 
            snprintf(pcMsg, sizeof(pcMsg), "[ %4lu ] %s COMPLETE (SKIP) (D = %lu)\n", (unsigned long)ulNow, pxT->pxCfg->pcName, (unsigned long)pxT->xAbsDeadline);
        }
        else
        {    
            snprintf(pcMsg, sizeof(pcMsg),"[ %4lu ] %s COMPLETE\n", (unsigned long)ulNow, pxT->pxCfg->pcName);
        }
        if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
        {
            UART_printf("WARNING: xLogQueue is full!");
        }
    }
    vRearmDeadlineTimer(ulNow);
}

/* Function used to release a new job of a specified task */
void vReleaseJob(PtlTask_t *pxT, TickType_t xNow)
{
    xSemaphoreTake(xMutex, portMAX_DELAY);
    pxT->ulJobId++;
    pxT->xLastRelease = xNow;
    pxT->xAbsDeadline = xNow + pxT->pxCfg->ulDeadline;
    pxT->xMissLogged = false;
    xSemaphoreGive(xMutex);
    if(xTrace)
    {
        char pcMsg[64];
        snprintf(pcMsg, sizeof(pcMsg),"[ %4lu ] %s RELEASE - DEADLINE: %lu\n", (unsigned long)xNow, pxT->pxCfg->pcName,(unsigned long) pxT->xAbsDeadline);
        if(xQueueSend(xLogQueue,pcMsg,0) != pdPASS)
        {
            UART_printf("WARNING: xLogQueue is full!");
        }
    }
    xTaskNotifyGive(pxT->xHandle);
}

/* Function used to know if tasks are currently able to run or not */
bool xIsTaskRunning()
{
    return xTaskRunning;
}

/* Function that sets a flag used to check if new jobs should be released */
void vSetTaskRunning(bool xState)
{
    xTaskRunning = xState;
}

/* Function used to stop the execution of the tasks currently scheduled */
void vSchedulerRequestStop(void)
{
    if(xIsTaskRunning()) {
        vSetTaskRunning(false);
    }

    if (xBinarySemaphore != NULL) 
    {
        xSemaphoreGive(xBinarySemaphore);
    }

    for (size_t uxI = 0; uxI < uxNumTasks; uxI++) 
    {
        if (xPtlTasks[uxI].xHandle != NULL) 
        {
            xTaskNotifyGive(xPtlTasks[uxI].xHandle);
        }
    }
}

/* Function that stores in pxStats a summary of the statistics related to a task */
bool xPTL_GetTaskStats(size_t uxTaskIndex, PtlTaskStats_t *pxStats) 
{
    if (uxTaskIndex >= uxNumTasks) {
        return false;
    }
    xSemaphoreTake(xMutex, portMAX_DELAY);
    pxStats->ulJobReleases = xPtlTasks[uxTaskIndex].ulJobId;
    pxStats->ulDeadlineMisses = xPtlTasks[uxTaskIndex].ulTotalMisses; 
    pxStats->ulOverruns = xPtlTasks[uxTaskIndex].ulTotalOverruns;
    xSemaphoreGive(xMutex);
    return true;
}