# PTL Scheduler - Automated Regression Test Suite

This directory contains the automated test suite developed to validate the correctness, robustness, and policy compliance of the custom Periodic Task Scheduler (PTL) based on FreeRTOS.

The suite includes deterministic functional tests, stress tests (overload), and edge-case validation, featuring an **automated pass/fail verification logic**.

## How to Run

1. Open `config.h`.
2. Set the `USE_TESTS` macro to `1`.
3. Compile and run the project (using `make`).
4. Observe the output on the **UART** log file.

```c
// config.h
#define USE_TESTS 1
```

## Test Suite Description

The test suite generates 11 tests. In particular:
#### Four Deterministic Tests:
- Three of them will check how the scheduler behaves when there are only two tasks, with different priorities. They will verify the Preemption and each one of them will verify a different Policy.
- The fourth will check how the scheduler applies th Round Robin: it will verify the time slicing for same-priority tasks.
#### Load Safe test:
A simple test where 8 random, schedulable tasks are running. In this case, the expected behavior will be to not have any deadline miss.
#### Stress Tests:
Three tests, one for each policy, in which 8 random tasks are scheduled. The goal is to avoid priority inversion.
#### Edge Case Tests:
Three tests, one for each policy, where 8 random tasks, with some fixed parameters, are scheduled. The goal is again to avoid priority inversion and check the system stability with corner-case inputs.

## Validation Logic

The suite implements a self-verification mechanism (`vVerifyTestResult`) that analyzes task statistics after each test execution.
1. **Deterministic Verification**:
Compares the actual behavior against a theoretical model.
- **SKIP Policy**: Verifies that the low-priority task actually skips jobs (`Actual Releases` < `Theoretical Releases`).
- **KILL Policy**: Verifies that overruns are detected and the task is restarted.
- **CATCH UP Policy**: Verifies that we have the expected amount of job releases and that there are deadline misses and overruns.
- **ROUND ROBIN**: Verifies that both tasks advance and finish without starving each other.
2. **Load Safe Tests**: 
In this scenario, the tasks are proved to be schedulable: here the verification mechanism checks that no deadline misses happened.
3. **Overload Verification (Stress Tests)**: 
In overload scenarios, deadline misses are inevitable. The test passes if Priorities are respected: The logic calculates the Miss Rate for High Priority ($P \ge 3$) and Low Priority tasks.
- **PASS** Condition: High_Prio_Miss_Rate <= Low_Prio_Miss_Rate.
- **FAIL** Condition: Priority Inversion detected (High priority tasks miss more often than low priority ones).

## Implementation Details

#### Deterministic Tests:
In this case, to have a theoretical baseline, only two tasks with predefined values are used. 
- In the Round Robin scenario, the tasks have the same priority
- In the other tests, we have a task A with lower priority than the other task B.
#### Load Safe Test:
In this case, using the function `vGenerateRandTasks`, tasks with random parameters (Period, Deadline, WCET, etc.) are generated, but with a check on their schedulability. The algorithm to check the necessary condition for schedulability is implemented in `xCheckSchedulability`.
#### Overload Tests:
There are two different scenarios:
1. Stress tests: the function `vGenerateRandTasks` is used again to generate random tasks, but this time without any check on the schedulability
2. Edge Case tests: 
The `vGenerateEdgeCaseTasks` function specifically creates "impossible" tasks to test scheduler stability, such as:
- Tasks with Execution Time > Period.
- Tasks with Period < Deadline.
- Tasks with Execution Time > Deadline.

#### Randomness
The tests are generated procedurally using a seed derived from the system tick and compile time (`__TIME__`). This ensures that re-compiling the project generates a new set of random tasks, increasing the probability of finding hidden bugs during regression testing.
## Example of Output
An example of a possible output is here reported.
```c
==========================================================================================
                                 FINAL REGRESSION REPORT                                  
==========================================================================================
Test 1 - Test 1: DETERMINISTIC TEST - POLICY SKIP: PASSED
Test 2 - Test 2: DETERMINISTIC TEST - POLICY KILL: PASSED
Test 3 - Test 3: DETERMINISTIC TEST - POLICY CATCH UP: PASSED
Test 4 - Test 4: DETERMINISTIC TEST - ROUND ROBIN: PASSED
Test 5 - Test 5: LOAD SAFE TEST: PASSED
Test 6 - Test 6: OVERLOAD TEST - POLICY SKIP: PASSED
Test 7 - Test 7: OVERLOAD TEST - POLICY KILL: PASSED
Test 8 - Test 8: OVERLOAD TEST - POLICY CATCH UP: PASSED
Test 9 - Test 9: EDGE-CASES TEST - POLICY SKIP: FAILED (Priority Inversion: High Priority Miss Rate (0.80) > Low Priority Miss Rate (0.20))
...
==========================================================================================

```