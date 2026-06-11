# Priority-based scheduler for Periodic Tasks in FreeRTOS

This project has the aim to reinforce the management of periodicity and deadlines in the FreeRTOS environment. This is done by creating a new layer called PTL: "`Periodic Task Layer`". 
This layer is structured by the following modules.

## The framework
The entry point of the code is the framework, where the user can establish how many tasks they want to create, specifing some parameters like the period, the deadline, the priority, the specific policy of and the global one, if they want to enable tracing in order to track what's going on with the flow of tasks.
The user also have other options: loading the previous configuration without doing every time the same process or loading some pre-defined tests.


## API_TASKS
The framework, after receivng all this information, communicates to the api, that is the real core of the application. Its role is to create and run the tasks, supervisioning if every task is respecting the deadline, the period and the specified policy on overrun. 
The policy determinets how the releasing of a new task makes overrun with another one: it is skipped, the older one is killed and the new one is released or it is just suspended?


## Tests
In order to validate the right functioning, the PTL comes with a test suite, that execute some tests to, for instance, control the overload or for edge cases.


## Utils
The folder contains some functional and useful file for the realization of the project: it contains the configuration of the application, for instance used to switch from test to normal execution; it shows the tasks that we have created and their specifications. 


# How can we use it?
All files are compiled and linked together by the use of a makefile, in which there are specified some targets. 
- To access to the framework, we use the command `make start`, here we can set our preferences.
- To do debugging, there are the two command target `qemu debug` and `gdb_start`.
- To clean all outputs, there is the command `clean`.
- To only compile the project there is `make`
- To execute che ELF directly, there is `make qemu_start`.