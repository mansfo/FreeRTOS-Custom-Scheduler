# FreeRTOS Custom Scheduler
This repository contains a FreeRTOS-based project that adds a custom periodic-task scheduler layer called PTL (Periodic Task Layer). The goal is to model and evaluate periodic real-time tasks with deadlines, overruns, and priority-aware scheduling inside an embedded FreeRTOS environment.
## Project overview
The project is organized around two main parts:
- `App/` — the application layer and custom scheduler framework
- `FreeRTOS/` — the FreeRTOS source tree and supporting libraries
The application code builds a scheduler that can:
- define periodic task configurations
- enforce deadlines and release timing
- detect overruns and apply policies such as skip, kill, or catch-up
- validate schedulability through analysis and tests
- run the resulting embedded application under QEMU
## Main components
### App/
The application contains the PTL implementation and supporting files.
- `main.c` — entry point, initializes the scheduler and starts the RTOS
- `PTL/Framework/` — Python-based configuration and schedulability tools
- `PTL/API_TASKS/` — scheduler runtime logic for periodic tasks, deadlines, and policies
- `PTL/Tests/` — validation and test suite for edge cases and overload behavior
- `PTL/Utils/` — config structures, task metadata, and generated scheduler data
### FreeRTOS/
This directory contains the upstream FreeRTOS kernel and related libraries used by the custom scheduler. It is included as a repository submodule tree and provides the underlying OS services.
## Scheduler features
The PTL layer is designed to manage periodic tasks in a FreeRTOS environment by tracking:
- task period and deadline
- worst-case execution time and priority
- release timing and job activation
- scheduler overrun handling
- deadline miss detection
Supported policies include:
- `SKIP` — ignore the new release while the prior job is still active
- `KILL` — terminate the currently running job and start the new one
- `CATCH_UP` — release the new job immediately and mark the previous one as missed
## Framework workflow
The `App/PTL/Framework` layer allows users to define task sets interactively and generate a scheduler configuration. Typical workflow:
1. Start the framework with `make start`
2. Create or load a scheduler configuration
3. Check schedulability (utilization and response-time analysis)
4. Generate and run the FreeRTOS skeleton application
## Build and run
Before building, initialize submodules:
```bash
git submodule update --init --recursive
```
From the project root:
```bash
cd App
make
```
To run the application in QEMU:
```bash
make qemu_start
```
To launch the interactive scheduler configuration tool:
```bash
make start
```
Useful maintenance targets:
```bash
make clean
make qemu_debug
gdb_start
```
## Hardware target
The build is configured for an ARM Cortex-M3 target using the MPS2 platform and GNU ARM toolchain. The project is intended for embedded development and emulation rather than desktop execution.
## Repository layout
```text
.
├── App/
│   ├── main.c
│   ├── Makefile
│   ├── PTL/
│   └── ...
├── FreeRTOS/
│   └── ...
├── .gitignore
├── .gitmodules
├── LICENSE.md
└── README.md
```
## Notes
This project is best understood as a teaching and experimentation framework for scheduler design in embedded real-time systems. It combines FreeRTOS, task-model configuration, and timing analysis into a single demonstrator for periodic task scheduling and deadline behavior.
