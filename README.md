# Linux-Programming-Summative-Project

This repository contains five programs demonstrating core systems programming and architecture concepts, including ELF binary reverse engineering, x86_64 assembly file I/O, Python C extensions, POSIX multithreading synchronization, and TCP socket programming.

## Project Structure

* elf_analysis.c: A C program specifically constructed for compilation, symbol stripping, and low-level static/dynamic ELF analysis.
* temperature.asm: An x86_64 Assembly utility that reads a text file into memory, handles varying line endings, and parses valid versus empty temperature readings.
* vibration.c: A custom Python C-extension module built for high-performance, zero-allocation statistical analysis of industrial vibration data.
* baggage_handler.c: A multithreaded simulation of an airport baggage handling system demonstrating the producer-consumer problem using POSIX threads, mutexes, and condition variables.
* server.c & client.c: A concurrent real-time TCP client-server system simulating a distributed digital library reservation platform.

## Compilation Instructions

Use the following commands to compile the respective programs:

### 1. Reverse Engineering Binary
* gcc -Wall -O0 -fno-inline -o elf elf_analysis.c
* strip elf

### 2. x86_64 Assembly File Processor
* nasm -f elf64 temperature.asm -o temperature.o
* gcc temperature.o -o temperature -no-pie -lc

### 3. Python C-Extension
* python3 setup.py build_ext --inplace

### 4. Baggage Handling Simulation
* gcc -Wall -pthread -o baggage_handler.c baggage_handler.c

### 5. Digital Library TCP Server/Client
* gcc -Wall -pthread -o server server.c
* gcc -Wall -o client client.c

## Execution and Testing

### 1. Reverse Engineering ELF Binary (elf_analysis.c)
Execute the target program to generate the baseline output:
* ./elf

To perform static analysis on the stripped binary:
* readelf -a elf
* objdump -d elf

To trace system calls and runtime behavior:
* strace ./elf
* gdb ./elf

### 2. x86_64 Assembly File Processor (temperature.asm)
Before running, ensure a sample data file named **temperature_data.txt** is located in the same directory:
* echo -e "22.5\n\n23.1\n21.8\n" > temperature_data.txt

Execute the assembly program to process the file and output the counts:
* ./temp_proc

### 3. Python C-Extension (vibration.c)
Execute the Python test script, which imports the compiled C module and passes it a tuple of floating-point data:
* python3 test_vibration.py

### 4. Baggage Handling Simulation (baggage_handler.c)
Execute the multithreaded baggage loader and aircraft consumer simulation:
* ./baggage_handler

### 5. Digital Library TCP Server (server.c & client.c)
Start the library server first to listen for incoming connections:
* ./server

In a separate terminal (or multiple terminals to test concurrency), run the client to authenticate and reserve books:
* ./client

### Active User Library IDs
* LIB101
* LIB102
* LIB103
