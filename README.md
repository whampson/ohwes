# Niobium Operating System
**Niobium** is a free operating system designed by Wes Hampson.

## Design Goals
- Keep it small; kernel and programs should fit on a 3.5in floppy (<1.44 MiB)
- Separate user-space and kernel-space
- Preemptive multitasking kernel
- Disk I/O; Support a common file system like FAT or ext2
- Target i386, add ARM support later on

## TODO List
- [-] Boot Loader
    - [x] Basic FS driver
    - [x] Load kernel into memory
    - [ ] Enter protected mode and call kernel
- [ ] Interrupt & Exception handling
- [ ] Terminal I/O
    - [ ] Keyboard driver
    - [ ] VGA driver
    - [ ] Input buffering
    - [ ] ANSI escape sequences?
- [-] File System
    - [-] FAT
    - [ ] ext2
- [ ] Program loader
    - [ ] ELF binaries
- [ ] Memory Manager
    - [ ] Process memory virtualization & protection
    - [ ] Dynamic memory allocation
- [ ] Scheduler
- [ ] Signals
    - [ ] Basic signal support
- [-] Tools
    - [-] fatfs - FAT File System Maniuplator
    - [ ] ext2fs - ext2 File System Manipulator
