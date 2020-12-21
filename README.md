# Niobium Operating System
The **Niobium Operating System** (**NbOS**) is a small multi-tasking operating
system for Intel 386-family CPUs designed by Wes Hampson.

## Design Goals
- Keep it small; kernel and programs should fit on a 3.5in floppy (<1.44 MiB)
- Separate user-space and kernel-space
- Preemptive multitasking kernel
- Disk I/O; Support a common file system like FAT or ext2 (or both!)

## Building & Running
To build Niobium, first we need to set up the build environment, then we build
using `make`.  
First, switch to the Niobium base directory, then run the following:
```
$ source scripts/devenv.sh
$ make
```
This will generate a floppy disk image at `bin/img/niobium.img`. You can use
this image to boot Niobium on an emulator, or write it to a floppy disk and boot
Niobium on a real PC!

## TODO List
- [x] Boot Loader
    - [x] Basic FS driver
    - [x] Load Kernel
- [x] System Initialization
    - [x] Enter Protected Mode
    - [x] Enable Paging
    - [x] Setup GDT/LDT/TSS
- [ ] Interrupt & Exception Handling
    - [ ] Setup IDT
    - [ ] Set up 8259A PIC
    - [ ] Set up handlers for Intel exceptions
    - [ ] Enable Keyboard interrupts
- [ ] Terminal I/O
    - [ ] Keyboard driver
    - [x] VGA driver
    - [x] ANSI escape sequences
    - [ ] Input/Output buffering
    - [ ] RS232 support
- [ ] Disk I/O
    - [ ] Floppy Disk Driver
    - [ ] Hard Disk Driver
- [ ] File System
    - [ ] FAT
    - [ ] ext2
- [ ] Program loader
    - [ ] ELF binaries
- [ ] Memory Manager
    - [ ] Process memory virtualization & protection
    - [ ] Dynamic memory allocation
- [ ] Scheduler
- [ ] Signals
    - [ ] Basic signal support
