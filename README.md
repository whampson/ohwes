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
The **OHWES** console supports numerous VT52/VT100/Linux terminal control characters and escape sequences, plus some OHWES-specific escape sequences, for controlling color, cursor position, and other console attributes.

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
| `ESC 3`           |       | Disable Blink     | Disables blinking text (not supported on all hardware). |
| `ESC 4`           |       | Enable Blink      | Enables blinking text (not supported on all hardware). |
| `ESC 5`           |       | Hide Cursor       | Makes the cursor invisible (not supported on all hardware). |
| `ESC 6`           |       | Show Cursor       | Makes the cursor visible (not supported on all hardware). |
| `ESC 7`           | DECSC | Save State        | Saves the console state (cursor properties, graphics attribute, blink status). |
| `ESC 8`           | DECRC | Restore State     | Restores the console state most-recently saved by `ESC 7`. |
| `ESC c`           | RIS   | Reset Console     | Resets the console to the initial state.      |
| `ESC E`           |       | Newline           | Performs a newline (CR-LF).                   |
| `ESC I`           |       | Reverse Linefeed  | Performs a reverse-linefeed.                  |
| `ESC M`           |       | Linefeed          | Performs a linefeed (LF).                     |
| `ESC T`           |       | Set Tab Stop      | Sets a tab stop at the current column.        |
| `ESC t`           |       | Clear Tab Stop    | Clears the tab stop at the current column, if present. |
| `ESC [`           | CSI   | Control Sequence Introducer | Starts a control sequence.          |

### Control Sequences
| Sequence          | Short |Name               | Effect                                         |
| :---------------- | :---- |:----------------- | ---------------------------------------------- |
| `CSI` *n* `A`     | CUU   | Cursor Up         | Moves the cursor up *n* rows (default 1).      |
| `CSI` *n* `B`     | CUD   | Cursor Down       | Moves the cursor down *n* rows (default 1).    |
| `CSI` *n* `C`     | CUF   | Cursor Forward    | Moves the cursor right *n* columns (default 1).|
| `CSI` *n* `D`     | CUB   | Cursor Back       | Moves the cursor left *n* columns (default 1). |
| `CSI` *n* `E`     | CNL   | Cursor Next Line  | Moves the cursor to the beginning of the line *n* rows down (default 1). |
| `CSI` *n* `F`     | CPL   | Cursor Previous Line | Moves the cursor to the beginning of the line *n* rows up (default 1). |
| `CSI` *n* `G`     | CHA   | Cursor Horizontal Absolute | Moves the cursor to column *n* (default 1). |
| `CSI` *n* `;` *m* `H` | CUP   | Cursor Position   | Moves the cursor to row *n*, column *m* (default 1,1). |
| `CSI` *n* `I`     |       | Repeat Reverse-Linefeed   | Performs *n* reverse-linefeeds. |
| `CSI` *n* `J`     | ED    | Erase in Display  | Clears part of the screen. If *n* is 0 (default), screen is cleared from cursor to end. If *n* is 1, screen is cleared from cursor to beginning. If *nI is 2, entire screen is cleared. Cursor does not move. |
| `CSI` *n* `K`     | EL    | Erase in Line     | Erases part of the line. If *n* is 0 (default), line is erased from cursor to end. If *n* is 1, line is erased from cursor to beginning. If *nI is 2, entire line is erased. Cursor does not move. |
| `CSI` *n* `M`     |       | Repeat Linefeed   | Performs *n* linefeeds. |
| `CSI` *n* `S`     | SU    | Scroll            | Shifts screen contents up *n* lines. Lines are added at the bottom. |
| `CSI` *n* `T`     | SD    | Reverse-Scroll    | Shifts screen contents down *n* lines. Lines are added at the top. |
| `CSI` *n* `m`     | SGR   | Set Graphics Attribute | Sets text attributes for subsequent characters. |
| `CSI s`           | SCP   | Save Cursor Position | Saves the current cursor position.         |
| `CSI u`           | RCP   | Restore Cursor Position | Restores the cursor position previously saved by `CSI s`. |

#### 'Set Graphics Attribute' Parameters
Multiple parameters may be passed in a given sequence, each separated by a `;`.
| Code      | Name              | Effect                                        |
| :-------- | :---------------- | --------------------------------------------- |
| `0`       | Reset             | All attributes off/defaults.                  |
| `1`       | Bright Foreground | Use bright foreground colors.                 |
| `2`       | Dim Foreground    | Dark gray foreground (simulate dim text).     |
| `4`       | Highlight         | Bright cyan foreground (simulate highlighted text). |
| `5`       | Bright Background/Blink | Use bright background colors, or blink the foreground text if blink is enabled. |
| `7`       | Invert            | Invert foreground and background colors.      |
| `21`      | Normal Foreground | Use normal foreground colors.                 |
| `22`      | No Dim            | Turn off dim text.                            |
| `24`      | No Highlight      | Turn off highlight.                           |
| `25`      | Normal Background/No Blink | Use normal background colors, or stop blinking the foreground text if blink is enabled. |
| `27`      | Invert Off        | Turn off color inversion.                     |
| `30`      | Foreground: <span style="color:#000000">Black</span>  | Set the foreground color to black. |
| `31`      | Foreground: <span style="color:#AA0000">Red</span>    | Set the foreground color to red. |
| `32`      | Foreground: <span style="color:#00AA00">Green</span>  | Set the foreground color to green. |
| `33`      | Foreground: <span style="color:#AA5500">Yellow</span> | Set the foreground color to yellow. |
| `34`      | Foreground: <span style="color:#0000AA">Blue</span>   | Set the foreground color to blue. |
| `35`      | Foreground: <span style="color:#AA00AA">Magenta</span>| Set the foreground color to magenta. |
| `36`      | Foreground: <span style="color:#00AAAA">Cyan</span>   | Set the foreground color to cyan. |
| `37`      | Foreground: <span style="color:#AAAAAA">White</span>  | Set the foreground color to white. |
| `40`      | Background: <span style="background-color:#000000">Black</span>  | Set the background color to black. |
| `41`      | Background: <span style="background-color:#AA0000">Red</span>    | Set the background color to red. |
| `42`      | Background: <span style="background-color:#00AA00">Green</span>  | Set the background color to green. |
| `43`      | Background: <span style="background-color:#AA5500">Yellow</span> | Set the background color to yellow. |
| `44`      | Background: <span style="background-color:#0000AA">Blue</span>   | Set the background color to blue. |
| `45`      | Background: <span style="background-color:#AA00AA">Magenta</span>| Set the background color to magenta. |
| `46`      | Background: <span style="background-color:#00AAAA">Cyan</span>   | Set the background color to cyan. |
| `47`      | Background: <span style="background-color:#AAAAAA">White</span>  | Set the background color to white. |
