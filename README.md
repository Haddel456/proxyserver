## ThreadPool in C

A simple thread pool implementation in C that allows multiple threads to execute tasks concurrently. This project simulates a fixed-size thread pool similar to what is used in high-performance systems like web servers or parallel computing frameworks.

# Overview

A thread pool is a collection of worker threads that efficiently execute multiple tasks concurrently. Instead of creating a new thread for every task, tasks are added to a queue and worker threads pick up the tasks from the queue.

**This implementation provides:**

- A fixed-size pool of threads.
- A queue to hold tasks (work_t structures).
- Proper synchronization using mutexes and condition variables.
- Graceful shutdown that waits for all tasks to complete before destroying threads.

## Features

- Fixed maximum number of threads (MAXT_IN_POOL).
- Thread-safe queue for task management.
- Supports dispatching tasks dynamically to the pool.
- Proper shutdown handling with dont_accept and shutdown flags.
- Demonstrates fundamental multi-threading concepts in C with pthread library.

## Structure

The project contains the following:

- threadpool.h – Header file declaring structures, function prototypes, and thread pool constants.
- threadpool.c – Implementation of thread pool functions.

**Key structures:**

- work_t: Represents a task with a function pointer and an argument.
- threadpool: Represents the thread pool, holding threads, the task queue, and synchronization primitives.

## Compile & Run

**Compile the code:**
gcc -o threadpool threadpool.c -lpthread
**Run the program:**
./threadpool
