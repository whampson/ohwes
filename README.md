# OHWES
**OHWES** is a small multi-tasking operating system for Intel 386-family CPUs
designed by Wes Hampson.

## Design Goals
- Keep it small; kernel and programs should fit on a floppy disk
- Separate user-space and kernel-space
- Create a pre-emptive multi-tasking kernel
- Disk I/O; support a common file system like FAT or ext2 (or both!)

## Building & Running
To build **OHWES**, first set up the build environment, then build using `make`.
```
$ source scripts/devenv.sh
$ make
```
This will generate a floppy disk image at `bin/ohwes.img`. You can then boot
**OHWES** on an i386 emulator, or write the image to a floppy disk and boot
**OHWES** on a real PC!

## TODO List
- [x] Boot Loader
    - [x] Basic filesystem driver
    - [x] Load the kernel into memory
- [x] System Initialization
    - [x] Enter Protected Mode
    - [x] Enable Paging
    - [x] Setup GDT/LDT/TSS
- [x] Interrupt & Exception Handling
    - [x] Setup IDT
    - [x] Setup 8259A PIC
    - [x] Setup handlers for Intel exceptions
- [ ] Terminal I/O
    - [x] Keyboard driver
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
