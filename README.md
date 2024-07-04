# Counter-Device_Driver-Project

Overview
This project implements a Unix character device driver that simulates a counter device (/dev/counter). The device behaves similarly to /dev/zero, but instead of providing an endless supply of zero-valued bytes, 
it produces a value that increments or decrements by a specified step size. This project demonstrates the basics of writing a Linux kernel module, including handling file operations and ioctl calls.

Project Structure
Makefile: Automates the compilation and cleaning of the project.
ctl.c: A utility program for interacting with the counter device through ioctl calls.
counter.c: The main implementation of the counter device driver.
build.sh: A script for building and installing the driver into the kernel.

Features
Device Operations
Write: Primes the counter with a starting value.
Read: Returns the current value and increments it by the step size. Handles overflow and underflow (e.g., 0xff + 2 = 0x01).

Ioctl Calls:
DEV_IOC_RST: Resets the counter value to zero and the step size to one.
DEV_IOC_GET: Returns the current counter value without changing it.
DEV_IOC_STP: Sets the step size for the counter.


Example Usage

Prime the counter with a starting value of 5 and read 8 bytes:
printf "\x05" > /dev/counter1
dd if=/dev/counter1 count=8 bs=1 | xxd -g 1

Output:
00000000: 05 06 07 08 09 0a 0b 0c                          ........


Set the counter to 11 and get its value:
printf "\x0b" > /dev/counter2
./ctl /dev/counter2 get

Output:
11

Set the step size to 2, prime the counter with 23, and read 8 bytes:
./ctl /dev/counter3 stp 2
printf "\x17" > /dev/counter3
dd if=/dev/counter3 count=8 bs=1 | xxd -g 1

Output:
00000000: 17 19 1b 1d 1f 21 23 25                          ........


Building and Installing

To build and install the driver, run the build.sh script:

./build.sh

This script:

Removes existing counter devices and modules.
Compiles the driver.
Inserts the module into the kernel.
Creates device nodes for the counter devices.

Code Files

Makefile
Handles the compilation and cleaning of the project.

ctl.c
Utility program for interacting with the counter device via ioctl calls.

counter.c
Main implementation of the counter device driver.

build.sh
Build script for compiling and installing the driver into the kernel.
