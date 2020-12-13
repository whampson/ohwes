# NbOS: Niobium Operating System
The **Niobium Operating System**, or **NbOS** for short, is a free operating
system for Intel 386-family CPUs designed by Wes Hampson.

## Design Goals
- Keep it small; kernel and programs should fit on a 3.5in floppy (<1.44 MiB)
- Separate user-space and kernel-space
- Preemptive multitasking kernel
- Disk I/O; Support a common file system like FAT or ext2 (or both!)

## TODO List
- [x] Boot Loader
    - [x] Basic FS driver
    - [x] Load kernel into memory
    - [x] Enter protected mode and call kernel
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
