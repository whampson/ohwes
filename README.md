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

## Console Codes
The **OHWES** console supports numerous VT52/VT100 terminal control characters and escape sequences, plus some OHWES-specific escape sequences, for controlling color, cursor position, and other console attributes.

### Control Characters
| Character         | Short | Name              | Effect                                        |
| :---------------- | :---- | :---------------- | --------------------------------------------- |
| `^H`              | BS    | Backspace         | Moves the cursor back one column and erases the character in that position. The cursor does not move back beyond the beginning of the line. |
| `^I`              | HT    | Horizontal Tab    | Moves the cursor ahead to the next tab stop. The cursor does not move past the end of the line.   |
| `^J`, `^K`, `^L`  | LF    | Line Feed         | Moves the cursor down one row. If the cursor is initially in the bottom row, the cursor will remain in the bottom row, the screen contents will shift up one row, and the bottom row will be blanked, creating a scrolling effect. |
| `^M`              | CR    | Carriage Return   | Moves the cursor to the beginning of the current line. |
| `^X`              | CAN   | Cancel            | Aborts the current escape sequence. |
| `^[`              | ESC   | Escape            | Starts an escape sequence. |
| `^?`              | DEL   | Delete            | Deletes the character beneath the cursor (forward delete). |

### Escape Sequences
| Sequence          | Short |Name               | Effect                                         |
| :---------------- | :---- |:----------------- | ---------------------------------------------- |
| `ESC 3`           |       | Disable Blink     | Disables blinking text.                       |
| `ESC 4`           |       | Enable Blink      | Enables blinking text.                        |
| `ESC 5`           |       | Hide Cursor       | Makes the cursor invisible.                   |
| `ESC 6`           |       | Show Cursor       | Makes the cursor visible.                     |
| `ESC 7`           | DECSC | Save State        | Saves the console state (cursor position, graphics attribute, blink status). |
| `ESC 8`           | DECRC | Restore State     | Restores the console state most-recently saved with `ESC 7`. |
| `ESC E`           |       | Newline           | Performs a newline (CR-LF).                   |
| `ESC I`           |       | Reverse Linefeed  | Moves the cursor up one row. If the cursor is initially in the top row, the cursor will remain in the top row, the screen contents will shift down one row, and the top row will be blanked, creating a scrolling effect. |
| `ESC M`           |       | Linefeed          | Performs a linefeed (LF).                     |
| `ESC [`           | CSI   | Control Sequence Introducer | Starts a control sequence.          |

### Control Sequences
| Sequence          | Short |Name              | Effect                                         |
| :---------------- | :---- |:---------------- | ---------------------------------------------- |
| `CSI` *n* `A`| CUU   | Cursor Up        | Moves the cursor up *n* rows (default 1).      |
