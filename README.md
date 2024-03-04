# Multithreaded ASCII Drawing with POSIX Threads

## Overview
This C program demonstrates a multithreaded approach to drawing a simple geometric figure on the console. It utilizes POSIX threads (pthreads) for parallel execution, showcasing synchronization mechanisms such as mutexes, condition variables, and barriers to coordinate the drawing process among multiple threads. The project is part of practical work assigned in the "Architecture of Operating Systems" class during the 3d year of my Bachelor's degree (Computer Science degree at University of Strasbourg). It showcases thread synchronization mechanisms like mutexes, condition variables, and barriers to coordinate access to a shared buffer between multiple producer and consumer threads.

The initial code framework was provided by the instructors. The goal was to deepen my understanding of thread synchronization in C using pthreads.

## Features
- **Multithreading**: Leverages multiple threads to perform drawing tasks in parallel.
- **Synchronization**: Uses pthread mutexes, condition variables, and barriers to ensure that drawing operations are performed in the correct order and without data races.

## Usage
This program was tested on Linux only.

Compile the program with:
```
gcc -o thr_sync thr_sync.c -lpthread
```

The program requires three command-line arguments to run:
1. `<max-delay>`: The maximum delay (in milliseconds) that a thread will wait before starting its part of the drawing.
2. `<nb-threads>`: The number of threads to be created for the drawing operation.
3. `<square-size>`: The size of a side of the square figure to be drawn.

The correct command to run the program is :
```
./thr_sync <max-delay> <nb-threads> <square-size>
```

## Example
Given the following command, the program will:
- Start by creating 10 threads.
- Each thread will wait for a random period (up to 750ms) before starting.
- The threads will then collaboratively draw a square figure with a side length of 10 units on the console.
```
./thr_sync 750 10 10
```

## Additional Notes

- This README.md was created with the assistance of ChatGPT 3.5.

- This program was tested on Linux only.
