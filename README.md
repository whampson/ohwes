# AFROS
**A**nother **FR**ee **O**perating **S**ystem

## About
**AFROS** is a free operating system designed by Wes Hampson.

## Design Goals
- Keep it small; kernel and programs should fit on a 3.5in floppy (<1.44 MiB)
- Separate user-space and kernel-space
- Preemptive multitasking kernel
- Support a common file system like FAT or ext2
- Target i386 (x86), add ARM support later on

## TODO List
- [ ] Boot Loader
    - [ ] Basic FS driver
    - [ ] Load kernel
    - [ ] Enter protected mode
- [ ] Interrupt & Exception handling
- [ ] Terminal I/O
    - [ ] Keyboard driver
    - [ ] VGA driver
    - [ ] Input buffering
    - [ ] ANSI escape sequences?
- [ ] File System
    - [ ] EXT2 filesystem
- [ ] Program loader
    - [ ] ELF binaries (basic support)
- [ ] Memory Manager
    - [ ] Process memory virtualization & protection
    - [ ] Dynamic memory allocation
- [ ] Scheduler
- [ ] Signals
    - [ ] Basic signal support
