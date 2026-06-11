#include "api_test.h"

static Test_t xTests[TEST_NUM];
static ConfTask_t xLoadSafeTasks[MAX_TASKS];
static ConfTask_t xRandTasks[MAX_TASKS];
static ConfTask_t xEdgeTasks[MAX_TASKS];
static ConfTask_t xDetTasks[DET_TASKS];
static TestReport_t xReportLog[TEST_NUM];

/* The Task in charge of defining and executing tests */
void vTestRunner(void *pvParameters)
{
    (void) pvParameters;

    vDefineTests(8);
    vExecuteTests();

    for (;;);
}

/* Define the tests to be run */
void vDefineTests(uint32_t ulTaskNum)
{
    uint32_t ulBaseSeed = pdTICKS_TO_MS(xTaskGetTickCount());    // Define a seed

    // Add entropy to the seed based on compile time
    char *pcTimeStr = __TIME__;
    for (size_t uxI = 0; pcTimeStr[uxI] != '\0'; uxI++) {
        ulBaseSeed = ulBaseSeed * 31 + pcTimeStr[uxI];
    }

    // Define random tasks
    vGenerateRandTasks(xLoadSafeTasks, &ulTaskNum, ulBaseSeed, true); 
    vGenerateRandTasks(xRandTasks, &ulTaskNum, ulBaseSeed + 9876, false);
    vGenerateEdgeCaseTasks(xEdgeTasks, &ulTaskNum, ulBaseSeed + 1234);

    // Define deterministic tasks
    vCreateDetermTasks(xDetTasks);

    // Define the tests
    xTests[0] = (Test_t)
    {
        .ulTestId=1,
        .pcName = "Test 1: DETERMINISTIC TEST - POLICY SKIP\0",
        .ulNumTasks = 2,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_SKIP,
        .pxTasks = xDetTasks,
        .xTestType = TEST_TYPE_DETERMINISTIC
    };
    xTests[1] = (Test_t)
    {
        .ulTestId=2,
        .pcName = "Test 2: DETERMINISTIC TEST - POLICY KILL\0",
        .ulNumTasks = 2,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_KILL,
        .pxTasks = xDetTasks,
        .xTestType = TEST_TYPE_DETERMINISTIC
    };
    xTests[2] = (Test_t)
    {
        .ulTestId=3,
        .pcName = "Test 3: DETERMINISTIC TEST - POLICY CATCH UP\0",
        .ulNumTasks = 2,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_CATCH_UP,
        .pxTasks = xDetTasks,
        .xTestType = TEST_TYPE_DETERMINISTIC
    };
    xTests[3] = (Test_t)
    {
        .ulTestId=4,
        .pcName = "Test 4: DETERMINISTIC TEST - ROUND ROBIN\0",
        .ulNumTasks = 2,
        .ulDuration = 2000, //ms
        .xPolicy = POLICY_SKIP,
        .pxTasks = &xDetTasks[2],
        .xTestType = TEST_TYPE_DETERMINISTIC
    };
    xTests[4] = (Test_t)
    {
        .ulTestId = 5,
        .pcName = "Test 5: LOAD SAFE TEST\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xLoadSafeTasks,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_SKIP,
        .xTestType = TEST_TYPE_LOAD_SAFE
    };
    xTests[5] = (Test_t)
    {
        .ulTestId=6,
        .pcName = "Test 6: OVERLOAD TEST - POLICY SKIP\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xRandTasks,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_SKIP,
        .xTestType = TEST_TYPE_OVERLOAD
    };
    xTests[6] = (Test_t)
    {
        .ulTestId=7,
        .pcName = "Test 7: OVERLOAD TEST - POLICY KILL\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xRandTasks,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_KILL,
        .xTestType = TEST_TYPE_OVERLOAD
    };
    xTests[7] = (Test_t)
    {
        .ulTestId=8,
        .pcName = "Test 8: OVERLOAD TEST - POLICY CATCH UP\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xRandTasks,
        .ulDuration = 3000, //ms
        .xPolicy = POLICY_CATCH_UP,
        .xTestType = TEST_TYPE_OVERLOAD
    };
    xTests[8] = (Test_t)
    {
        .ulTestId=9,
        .pcName = "Test 9: EDGE-CASES TEST - POLICY SKIP\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xEdgeTasks,
        .ulDuration = 3000,
        .xPolicy = POLICY_SKIP,
        .xTestType = TEST_TYPE_OVERLOAD
    };
    xTests[9] = (Test_t)
    {
        .ulTestId=10,
        .pcName = "Test 10: EDGE-CASES TEST - POLICY KILL\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xEdgeTasks,
        .ulDuration = 3000,
        .xPolicy = POLICY_KILL,
        .xTestType = TEST_TYPE_OVERLOAD
    };
    xTests[10] = (Test_t)
    {
        .ulTestId=11,
        .pcName = "Test 11: EDGE-CASES TEST - POLICY CATCH UP\0",
        .ulNumTasks = ulTaskNum,
        .pxTasks = xEdgeTasks,
        .ulDuration = 3000,
        .xPolicy = POLICY_CATCH_UP,
        .xTestType = TEST_TYPE_OVERLOAD
    };
}

/* Run the tests, verify their results and print a summary */
void vExecuteTests()
{
    for(size_t uxI=0; uxI<TEST_NUM; uxI++)
    {
        Test_t *pxTest = &xTests[uxI];
        UART_printf("Running Test...\n");

        // Set the configuration of the scheduler
        SchedulerConfig_t xTestCfg;
        xTestCfg.xGlobalPolicy = pxTest->xPolicy;
        xTestCfg.xTraceEnabled = true;
        xTestCfg.uxMaxTasks = pxTest->ulNumTasks;
        xTestCfg.uxNumTasks = pxTest->ulNumTasks;
        for (size_t uxJ = 0; uxJ < pxTest->ulNumTasks; uxJ++) 
        {
            xTestCfg.xTasks[uxJ] = pxTest->pxTasks[uxJ];  // Load the tasks defined for this test
        }

        vInit(&xTestCfg);
        vStart();   // Start the test

        vTaskDelay(pdMS_TO_TICKS(pxTest->ulDuration));      // Wait to let the test execute for the predefined time
        UART_printf("Ending Test...\n");
        vStopTest(); // Stop the test
        vVerifyTestResult(uxI, pxTest, pxTest->xTestType);    // Check the results of this test
    }
    // All tests have been executed, print a summary
    vPrintSummary();
}

/* Stop the test, avoid the release of new jobs */
void vStopTest()
{
    vSchedulerRequestStop();  // Request the scheduler to stop executing jobs of the task in the test just ended
    vTaskDelay(pdMS_TO_TICKS(1000)); //wait for old task to finish their execution
}

/* Function used to check the necessary condition for the schedulability of the tasks in input */
bool xCheckSchedulability(ConfTask_t *pxInTasks, uint32_t ulNum)
{
    ConfTask_t pxTasks[ulNum];
    memcpy(pxTasks, pxInTasks, ulNum * sizeof(ConfTask_t)); // Use a copy of the task array

    // Sort tasks according to their priorities
    for (size_t uxI = 0; uxI < ulNum - 1; uxI++) 
    {
        for (size_t uxJ = uxI + 1; uxJ < ulNum; uxJ++) 
        {
            if (pxTasks[uxJ].uxPriority > pxTasks[uxI].uxPriority) 
            {
                ConfTask_t xTmp = pxTasks[uxI];
                pxTasks[uxI] = pxTasks[uxJ];
                pxTasks[uxJ] = xTmp;
            }
        }
    }

    // Check the necessary condition for schedulability
    for (size_t uxI = 0; uxI < ulNum; uxI++) 
    {
        uint32_t ulCi = (uint32_t)pxTasks[uxI].pvArg;
        uint32_t ulDi = pxTasks[uxI].ulDeadline;

        uint32_t ulR = ulCi;

        while (1)
        {
            uint32_t ulI = 0;
            for (size_t uxJ = 0; uxJ < uxI; uxJ++) 
            {
                uint32_t ulCj = (uint32_t)pxTasks[uxJ].pvArg;
                uint32_t ulTj = pxTasks[uxJ].ulPeriod;

                ulI += ((ulR + ulTj - 1) / ulTj) * ulCj;
            }

            uint32_t ulRnext = ulCi + ulI;

            if (ulRnext > ulDi)
            {
                return false;
            }

            if (ulRnext == ulR)
            {
                break;
            }
            ulR = ulRnext;
        }
    }
    return true;
}


/* Generate random tasks */
void vGenerateRandTasks(ConfTask_t *pxOut, uint32_t *pulNum, uint32_t ulSeed, bool xLoadSafe)
{
    srand(ulSeed);
    static char pcNames[MAX_TASKS][15];
    uint32_t ulNum = *pulNum < MAX_TASKS? *pulNum : MAX_TASKS; // Make sure we don't exceed the max number of tasks
    size_t uxPriority;
    uint32_t ulDeadline, ulPeriod, ulStackSize, ulWCET;
    bool xOk = false;

    while(!xOk)
    {
        float fTotalUtilization = 0.0f;
        for(size_t uxI=0; uxI<ulNum; uxI++)
        {
            ulPeriod = 50 + (rand() % 200); // Define a period
            if (rand() % 2 == 0)            // Define a deadline: half times it will be equal to the period
            {          
                ulDeadline = ulPeriod;
            } 
            else 
            {
                uint32_t ulMinDeadline = ulPeriod / 2; 
                if (ulMinDeadline < 20) 
                {
                    ulMinDeadline = 20; 
                }
                
                ulDeadline = ulMinDeadline + (rand() % (ulPeriod - ulMinDeadline + 1));
            }
            uint32_t ulMaxWT = (ulDeadline * 60) / 100; // Max work time = 60% deadline
            if (ulMaxWT < 1)
            {
                ulMaxWT = 1; // Safety check
            }
            ulWCET = 1 + (rand() % ulMaxWT);
            fTotalUtilization += (float)ulWCET / (float)ulPeriod;    // Compute utilization factor

            uxPriority = 1 + (rand() % 3) + tskIDLE_PRIORITY; // Define the priority: max 3, as TestRunner has priority 4
            ulStackSize = configMINIMAL_STACK_SIZE*(1 + (rand() % 5));
            snprintf(pcNames[uxI], sizeof(pcNames[uxI]), "TaskR%02u", uxI);

            // Configure a task with the values computed
            pxOut[uxI] = (ConfTask_t)
            {
                .pcName = pcNames[uxI],
                .pvArg = (void *)ulWCET,
                .xEntry = vBusyWaitTask,
                .ulStackSize = ulStackSize,
                .uxPriority = uxPriority,
                .ulPeriod = ulPeriod,
                .ulDeadline = ulDeadline,
                .xPolicy = rand() % 2 == 0? NO_POLICY : rand() % 4, // Half times the policy will be the global one
                .ulStartOffset = rand() % 4 != 1? 0 : rand() % 50   // One fourth of the times the offset will be != 0
            };
        }

        xOk = true;

        if(xLoadSafe)   // If we want schedulable tasks, we have to check schedulability
        {
            xOk = xCheckSchedulability(pxOut, ulNum);
        }
    }
    *pulNum = ulNum;
}

/* Define some edge case tasks to check the scheduler's behavior */
void vGenerateEdgeCaseTasks(ConfTask_t *pxOut, uint32_t *pulNum, uint32_t ulSeed)
{
    srand(ulSeed);
    uint32_t ulNum = *pulNum < MAX_TASKS? *pulNum : MAX_TASKS; 
    if(ulNum>=5)
    {

        vGenerateRandTasks(pxOut, &ulNum, ulSeed, false);   // Generate random tasks, then fix some properties

        // Task with WCET = T
        pxOut[ulNum-1].pvArg = (void*)(uintptr_t)pxOut[ulNum-1].ulPeriod;

        // Task with WCET = D
        pxOut[ulNum-2].pvArg = (void*)(uintptr_t)pxOut[ulNum-2].ulDeadline;

        // Task with period smaller than deadline
        pxOut[ulNum-3].ulPeriod = pxOut[ulNum-3].ulDeadline - 10;

        // Task with WCET > D
        pxOut[ulNum-4].pvArg = (void*)(uintptr_t)(pxOut[ulNum-4].ulDeadline+1);

        // Task with WCET > T
        pxOut[ulNum-5].pvArg = (void*)(uintptr_t)(pxOut[ulNum-5].ulPeriod+1);
        *pulNum = ulNum;
    }
    else
    {
        UART_printf("Error generating edge cases tasks");
    }
}

/* Create two deterministic tasks */
void vCreateDetermTasks(ConfTask_t *pxOut)
{
    pxOut[0] = (ConfTask_t)
    {
        .pcName = "A",
        .pvArg = (void *)80,    // Worst case execution time
        .xEntry = vBusyWaitTask,
        .ulStackSize = configMINIMAL_STACK_SIZE*2,
        .uxPriority = 2,
        .ulPeriod = 100,
        .ulDeadline = 100,
        .xPolicy = NO_POLICY
    };
    pxOut[1] = (ConfTask_t)
    {
        .pcName = "B",
        .pvArg = (void *)50,    // Worst case execution time
        .xEntry = vBusyWaitTask,
        .ulStackSize = configMINIMAL_STACK_SIZE*2,
        .uxPriority = 3,
        .ulPeriod = 150,
        .ulDeadline = 100,
        .xPolicy = NO_POLICY
    };
    pxOut[2] = (ConfTask_t)
    {
        .pcName = "C",
        .pvArg = (void *)80,
        .xEntry = vBusyWaitTask,
        .ulStackSize = configMINIMAL_STACK_SIZE*2,
        .uxPriority = 2, // Same as D
        .ulPeriod = 200,
        .ulDeadline = 200,
        .xPolicy = NO_POLICY
    };
    pxOut[3] = (ConfTask_t)
    {
        .pcName = "D",
        .pvArg = (void *)80,
        .xEntry = vBusyWaitTask,
        .ulStackSize = configMINIMAL_STACK_SIZE*2,
        .uxPriority = 2, // Same as C
        .ulPeriod = 200,
        .ulDeadline = 200,
        .xPolicy = NO_POLICY
    };
}

/* Check the results of a test */
void vVerifyTestResult(uint32_t ulTestIndex, Test_t *pxTest, eTestType xType) 
{
    xReportLog[ulTestIndex].ulId = pxTest->ulTestId;
    strncpy(xReportLog[ulTestIndex].pcName, pxTest->pcName, 63);
    xReportLog[ulTestIndex].xType = xType;
    xReportLog[ulTestIndex].xPassed = true;
    xReportLog[ulTestIndex].pcFailReason[0] = '\0';
    
    PtlTaskStats_t xStats;
    uint32_t ulTotalMisses = 0;
    uint32_t ulHighPrioMisses = 0;
    uint32_t ulLowPrioMisses = 0;
    uint32_t ulHighJobs = 0, ulLowJobs = 0;
    float fHighMissRate = 0.0f;
    float fLowMissRate = 0.0f;

    for (size_t uxI = 0; uxI < pxTest->ulNumTasks; uxI++) {
        xPTL_GetTaskStats(uxI, &xStats);   // Get stats of the task
        ulTotalMisses += xStats.ulDeadlineMisses;
        if (pxTest->pxTasks[uxI].uxPriority >= 3) 
        {
            ulHighPrioMisses += xStats.ulDeadlineMisses;
            ulHighJobs += xStats.ulJobReleases;
        }
        else
        {
            ulLowPrioMisses += xStats.ulDeadlineMisses;
            ulLowJobs += xStats.ulJobReleases;
        }
    }
    
    if (xType == TEST_TYPE_LOAD_SAFE)
    {
        if (ulTotalMisses > 0) 
        { // A single miss is a fail
            xReportLog[ulTestIndex].xPassed = false;
            snprintf(xReportLog[ulTestIndex].pcFailReason, 64, "%lu Deadline Misses detected", (unsigned long)ulTotalMisses);
        }
    }
    else if (xType == TEST_TYPE_OVERLOAD) 
    { // Fail if priorities are not respected
        if (ulHighJobs > 0) 
        {
            fHighMissRate = (float)ulHighPrioMisses / (float)ulHighJobs;
        }
        if (ulLowJobs > 0) 
        {
            fLowMissRate = (float)ulLowPrioMisses / (float)ulLowJobs;
        }

        if (fHighMissRate > (fLowMissRate * 1.1f) && ulLowJobs > 0)     
        {   // If the rate of high prio misses is higher than the low prio misses, the test is failed, as priorities are not respected
            xReportLog[ulTestIndex].xPassed = false;
            snprintf(xReportLog[ulTestIndex].pcFailReason, 128,
                "Priority Inversion: High Priority Miss Rate (%d%%) > Low Priority Miss Rate (%d%%)", 
                (int)(fHighMissRate*100.0), (int)(fLowMissRate*100.0)
            );
        }
    }
    else if (xType == TEST_TYPE_DETERMINISTIC) 
    {
        char pcReason[64];
        if (!xCheckDeterministicLogic(pxTest, pcReason)) 
        {
            xReportLog[ulTestIndex].xPassed = false;
            strncpy(xReportLog[ulTestIndex].pcFailReason, pcReason, 63);
        }
    }
}

/* Helper function to check the results of the deterministic tests */
bool xCheckDeterministicLogic(Test_t *pxTest, char *pcFailReason) 
{
    
    PtlTaskStats_t xStats1, xStats2;
    // Get stats of the two tasks
    xPTL_GetTaskStats(0, &xStats1);
    xPTL_GetTaskStats(1, &xStats2);

    if (strstr(pxTest->pcName, "ROUND ROBIN") != NULL)  // if name contains Round Robin
    {   
        // In RR, both tasks need to finish with no misses
        if (xStats1.ulDeadlineMisses > 0 || xStats2.ulDeadlineMisses > 0) 
        {
            snprintf(pcFailReason, 64, "RR Failure: Misses detected (C:%lu, D:%lu)\n", 
                     (unsigned long)xStats1.ulDeadlineMisses, (unsigned long)xStats2.ulDeadlineMisses);
            return false;
        }
        
        // Check jobs were correctly released
        uint32_t ulExpected = pxTest->ulDuration / 200;
        if (xStats1.ulJobReleases < ulExpected-1 || xStats2.ulJobReleases < ulExpected-1) 
        {
            strcpy(pcFailReason, "RR Failure: Low release count");
            return false;
        }
        
        return true;
    }
    else
    {
        // B has higher priority, so it should have no overruns and no misses
        if (xStats2.ulDeadlineMisses > 0 || xStats2.ulOverruns > 0) 
        {
            snprintf(pcFailReason, 64, "High Prio Task B failed! (Miss: %lu)", (unsigned long)xStats2.ulDeadlineMisses);
            return false;
        }

        uint32_t ulExpectedReleasesA = pxTest->ulDuration / pxTest->pxTasks[0].ulPeriod;
        uint32_t ulTolerance = 1; 

        // Verify policy on Task A
        switch (pxTest->xPolicy) 
        {
            case POLICY_SKIP:
                // When A skips, a job is not released. If releases are more than expected, A did not skip
                if (xStats1.ulJobReleases >= (ulExpectedReleasesA - ulTolerance))
                {
                    snprintf(pcFailReason, 64, "Policy SKIP failed: A did not skip jobs (Rel: %lu/%lu)", 
                            (unsigned long)xStats1.ulJobReleases, (unsigned long)ulExpectedReleasesA);
                    return false;
                }
                // Task A must have overrun to activate SKIP policy
                if (xStats1.ulOverruns == 0)
                {
                    strcpy(pcFailReason, "Policy SKIP failed: No Overruns detected");
                    return false;
                }
                break;

            case POLICY_KILL:
                // Task A should have many overruns
                if (xStats1.ulOverruns == 0) 
                {
                    strcpy(pcFailReason, "Policy KILL failed: No kills detected");
                    return false;
                }
                // Kill leads to a new release, so releases should be consistent
                if (xStats1.ulJobReleases < (ulExpectedReleasesA - ulTolerance)) 
                {
                    snprintf(pcFailReason, 64, "Policy KILL anomaly: Low releases (%lu/%lu)", 
                            (unsigned long)xStats1.ulJobReleases, (unsigned long)ulExpectedReleasesA);
                    return false;
                }
                break;

            case POLICY_CATCH_UP:
                // All jobs are released, but we should have deadline miss as they finish late
                if (xStats1.ulJobReleases < (ulExpectedReleasesA - ulTolerance)) 
                {
                    snprintf(pcFailReason, 64, "Policy CATCH_UP failed: Jobs lost (%lu/%lu)", 
                            (unsigned long)xStats1.ulJobReleases, (unsigned long)ulExpectedReleasesA);
                    return false;
                }
                if (xStats1.ulDeadlineMisses == 0 && xStats1.ulOverruns == 0) 
                {
                    strcpy(pcFailReason, "Policy CATCH_UP failed: No delay detected");
                    return false;
                }
                break;
                
            default:
                break;
        }
    }
    return true; // OK
}

/* Print a human-readable report summary */
void vPrintSummary() 
{
    UART_printf("\n\n==========================================================================================\n");
    UART_printf("                                 FINAL REGRESSION REPORT                                  \n");
    UART_printf("==========================================================================================\n");

    for (size_t uxI = 0; uxI < TEST_NUM; uxI++) 
    {
        if (xReportLog[uxI].ulId == 0) 
        {
            continue; // Skip if test not executed
        }

        char pcMsg[256];
        if (xReportLog[uxI].xPassed) 
        {
            snprintf(pcMsg, sizeof(pcMsg), "%s: PASSED\n", xReportLog[uxI].pcName);
        } 
        else 
        {
            snprintf(pcMsg, sizeof(pcMsg), "%s: FAILED (%s)\n", xReportLog[uxI].pcName, xReportLog[uxI].pcFailReason);
        }
        UART_printf(pcMsg);
    }
    UART_printf("==========================================================================================\n");
}