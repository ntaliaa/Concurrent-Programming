# Concurrent-Programming
This repository contains solutions for Concurrent Programming assignments, implemented in C using pthreads. The assignments focus on thread synchronization, process coordination, and parallel computing techniques, using binary semaphores, monitors, and multi-threaded programming.
📂 Repository Structure

📦 concurrent_programming  
 ┣ 📂 assignment_1  
 ┃ ┣ 📜 fifo_pipe.c  
 ┃ ┣ 📜 fifo_pipe.h  
 ┃ ┣ 📜 primality_tester.c  
 ┃ ┣ 📜 external_mergesort.c  
 ┃ ┗ 📜 Makefile  
 ┣ 📂 assignment_2  
 ┃ ┣ 📜 mysem.h  
 ┃ ┣ 📜 mysem.c  
 ┃ ┣ 📜 bridge_simulation.c  
 ┃ ┣ 📜 roller_coaster.c  
 ┃ ┗ 📜 Makefile  
 ┣ 📂 assignment_3  
 ┃ ┣ 📜 monitor_semaphore.c  
 ┃ ┣ 📜 primality_monitor.c  
 ┃ ┣ 📜 bridge_monitor.c  
 ┃ ┣ 📜 roller_coaster_monitor.c  
 ┃ ┗ 📜 Makefile  
 ┗ 📜 README.md

🔹 Assignment 1 – Thread Synchronization & Data Processing
1.1 FIFO Pipes

    Implement a single-direction FIFO pipe for inter-thread communication using a ring buffer.
    The program reads a file and transfers it between two threads, creating input.copy and input.copy2.

1.2 Multi-threaded Primality Test

    A multi-threaded program that checks if given numbers are prime, using N worker threads.
    Performance analysis based on N and input values.

1.3 External MergeSort

    A parallelized external mergesort implementation using pthreads.
    Each recursive call creates two new threads for sorting before merging.

🔹 Assignment 2 – Binary Semaphores & Synchronization
2.1 Custom Binary Semaphores

    Implement custom binary semaphores (mysem_t) using System V semaphores.
    Includes functions for initialization, up/down operations, and destruction.

2.2 Multi-threaded Primality Test (with Semaphores)

    Modifies 1.2 by replacing busy-wait synchronization with custom binary semaphores.

2.3 Narrow Bridge (Traffic Control)

    Simulates a one-lane bridge where only N vehicles can pass in one direction at a time.
    Uses custom binary semaphores to synchronize traffic without a central controller thread.

2.4 Roller Coaster Simulation

    Models a roller coaster where passengers board and exit in a synchronized manner.
    Synchronization is handled exclusively with binary semaphores.

🔹 Assignment 3 – Monitor-based Synchronization
3.1 Binary Semaphores (with Monitor)

    Implements binary semaphores using a monitor-based synchronization mechanism.
    Tested in 2.3 (Narrow Bridge) or 2.4 (Roller Coaster).

3.2 Multi-threaded Primality Test (with Monitor)

    Replaces binary semaphores in 2.2 with a monitor-based approach using mutexes and condition variables.

3.3 Narrow Bridge (with Monitor)

    Modifies 2.3 by implementing monitor-based synchronization instead of semaphores.

3.4 Roller Coaster (with Monitor)

    Modifies 2.4 to use monitors instead of binary semaphores.
