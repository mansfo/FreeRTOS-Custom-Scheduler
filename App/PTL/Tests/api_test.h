#ifndef API_TEST_H
#define API_TEST_H

#include "api_task.h"
#include <stdlib.h>
#include <string.h>

#define TEST_NUM 11
#define DET_TASKS 4

/* Custom enum type to define the possible different tests */
typedef enum TestType_t 
{
    TEST_TYPE_DETERMINISTIC,    // Simple test with only two tasks
    TEST_TYPE_LOAD_SAFE,        // Random schedulable tasks
    TEST_TYPE_OVERLOAD          // Random tasks aiming at stressing the scheduler
} eTestType;

/* Custom type to define a Test */
typedef struct Test 
{
    uint32_t ulTestId;          // Test ID
    const char *pcName;         // A name to identify the test
    OverrunPolicy_t xPolicy;    // Global policy adopted by the test
    ConfTask_t *pxTasks;        // Pointer to a struct containing the tasks for the test
    uint32_t ulNumTasks;        // Number of tasks run in the test
    uint32_t ulDuration;        // Duration of the test
    eTestType xTestType;        // Type of the test (Deterministic, Random, ...)
} Test_t;

/* Custom type to store a report of each test */
typedef struct TestReport
{
    uint32_t ulId;              // Test ID
    char pcName[128];           // Test name
    eTestType xType;            // Test type
    bool xPassed;               // Is the test successful?
    char pcFailReason[128];     // Store info about the (possible) fail
} TestReport_t;


/* Definition of functions used by the Test suite */

void vTestRunner(void *pvParameters);
void vDefineTests(uint32_t ulTaskNum);
void vExecuteTests(void);
void vStopTest(void);
void vGenerateRandTasks(ConfTask_t *pxOut, uint32_t *pulNum, uint32_t ulSeed, bool xLoadSafe);
void vGenerateEdgeCaseTasks(ConfTask_t *pxOut, uint32_t *pulNum, uint32_t ulSeed);
void vCreateDetermTasks(ConfTask_t *pxOut);
void vVerifyTestResult(uint32_t ulTestIndex, Test_t *pxTest, eTestType xType);
bool xCheckDeterministicLogic(Test_t *pxTest, char *pcFailReason);
void vPrintSummary(void);

#endif