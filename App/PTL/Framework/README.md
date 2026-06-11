# PTL Configuration Framework

The PTL Framework is made on Python design to simplify the setup of periodic tasks, perform schedulability analysis and ultimately generate the FreeRTOS skeleton application.

## How to Run

1. Navigate to Project root ../group24/Project/App
2. Type make start
3. Select from the menu the operation you want to perform

```c
1) Create tasks.
2) Load test.
3) Check Schedulability
4) Create and run FreeRTOS skeleton.
5) Exit
```
Before select `Check Schedulability` or `Create and run FreeRTOS skeleton` you should select the scheduler from `Load test` or create one in `Create tasks`.

## Create tasks

Here we can define a set of tasks and configuration for the scheduler:

1. `Global configuration of scheduler` : option for trace the scheduler, option for a offset start, global policy, number of tasks.
2. `Task configuration` : for each task we can define the Name, Period, WCET, Deadline, Priority and personal Policy for the task; stack size is fixed at `configMINIMAL_STACK_SIZE * 2` to avoid errors.

At the end we get in folder ../Framework the `custom_scheduler.json` which contains the scheduler configuration.

## Load test

The Load Test function allows you to dynamically explore and select existing scheduler configurations stored in the `/PTL/Framework` directory, including custom configurations done manually or with `Create tasks`. After you select the scheduler configurations you can `Check Schedulability` or compile and run the scheduler on FreeRTOS.

## Check Schedulability

We check the schedulability of the selected scheduler checking first the total **Utilization Factor**. $$\sum_{i=1}^{n} \frac{C_i}{T_i} \leq 1$$
Then, to guarantee schedulability under fixed-priority preemptive scheduling (FreeRTOS native behavior), the framework performs a **Response Time Analysis** (RTA): this method calculates the worst-case response time ($R_i$) for each task $i$, accounting for the interference caused by higher-priority tasks.
The algorithm proceeds as follows:
- Priority Sorting: tasks are sorted by priority. In FreeRTOS, a higher numerical value indicates higher priority. Let $hp(i)$ be the set of tasks with priority higher than task $i$.
- Iterative Calculation: for each task $i$, the response time $R_i$ is computed iteratively using the following recursive equation: $$R_i^{(k+1)} = C_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j$$The iteration starts with $R_i^{(0)} = C_i$. The term $\lceil \frac{R_i^{(k)}}{T_j} \rceil$ represents the maximum number of times task $j$ can preempt task $i$ within the window $R_i^{(k)}$.
The iteration stops when the value converges ($R_i^{(k+1)} = R_i^{(k)}$).
- Pass Criterion: The task set is declared schedulable if and only if, for every task $i$:$$R_i \le D_i$$Where $D_i$ is the relative deadline (ulDeadline). If any task violates this condition ($R_i > D_i$), the framework flags a "Deadline Miss", meaning tasks are not schedulable.

## Create and run FreeRTOS skeleton

The framework executes three operations:

1. Reads the active scheduler json file and dynamically generates the `ptl_config.c` file in `PTL\Utils`. It maps the high-level configuration of the scheduler into a struct `SchedulerConfig_t` required by the PTL Scheduler.
2. The framework invokes the system's `make` command to automatically compile the project program from sources code.
3. Execute `make qemu_start` which start the emulator QEMU for the source code compilated and start our scheduler.