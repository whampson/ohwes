/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: src/include/_config.h
 *      Created: July 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <i386/paging.h>

// TODO: make a lot of this runtime configurable

//
// ----------------------------------------------------------------------------
// General Configuration
//

// booting
#define CONFIG_FLOPPY_BOOT          1   // (bool) configure for boot from floppy disk

// memory
#define CONFIG_HIGHER_GROUND        1   // (bool) map kernel in high virtual address space
#define CONFIG_STACK_SIZE           8   // (pages) stack frame size

// crash behavior
#define CONFIG_SHOW_CRASH_SCREEN    1   // (bool) display a visceral screen upon system crash
#define CONFIG_CRASH_KEY            1   // (bool) test various crash scenarios w/ keystroke

// log messages
#define CONFIG_PRINT_LOGO           1   // (bool) show a special logo at boot
#define CONFIG_PRINT_PAGE_MAPPINGS  1   // (bool) show updates to page tables
#define CONFIG_PRINT_IOCTL          1   // (bool) show ioctl calls

// kernel log behavior; log messages written to consoles
#define CONFIG_KLOG_LEVEL           5   // (0-5) kernel console log level (4=INFO)
#define CONFIG_KLOG_SHOW_TIME       1   // (bool) show timestamp in kernel log messages
#define CONFIG_KLOG_USE_COLOR       1   // (bool) print a colorful kernel log
#define CONFIG_KLOG_RING_SIZE       4   // (pages) log ring buffer size

// console devices
#define CONFIG_VT_CONSOLE           1   // (bool) use a virtual terminal as a console device
#define CONFIG_VT_CONSOLE_NUM       0   // (0-NR_TERMINAL) console virtual terminal select, 0 for active terminal
#define CONFIG_SERIAL_CONSOLE       1   // (bool) use a serial port as a console device
#define CONFIG_SERIAL_CONSOLE_COM   2   // (1-NR_COM) select COM port to use for serial console
#define CONFIG_SERIAL_CONSOLE_BAUD  BAUD_115200 // baud rate, one of BAUD_* enums
#define CONFIG_INITIAL_VT           1   // (1-NR_TERMINAL) virtual terminal shown at boot
#define CONFIG_EARLY_PRINT          1   // (bool) allow console printing before TTYs are available
#define CONFIG_E9_HACK              1   // (bool) tee output to I/O port 0xE9
#define CONFIG_E9_HACK_CONSOLE      1   // (bool) register 0xE9 as a console interface
#define CONFIG_E9_HACK_VT           1   // (bool) tee writes to active terminal to 0xE9

// debugging over COM port
#define CONFIG_SERIAL_DEBUG         1   // (bool) enable debugging over a COM port
#define CONFIG_SERIAL_DEBUG_COM     1   // (1-NR_COM) select COM port to use for debugging, cannot be shared
#define CONFIG_SERIAL_DEBUG_BAUD    BAUD_115200 // baud rate, one of BAUD_* enums

//
// ----------------------------------------------------------------------------
// Counts of Things
//

// memory
#define MAX_NR_POOLS            256    // max num concurrent pools

// filesystem
#define MAX_NR_INODES           4096  // max num inodes
#define MAX_NR_DENTRIES         4096  // max num directory entries
#define MAX_NR_TOTAL_OPEN       64    // max num open files on system
#define MAX_NR_IO_RESERVATIONS  8     // max num reserved I/O port ranges

// i/o
#define NR_TERMINAL             8     // number of virtual terminals
#define NR_COM                  4     // number of serial (COM) ports
#define NR_TTY                  (NR_TERMINAL+NR_COM)  // number of TTY endpoints
#define TTY_BUFFER_SIZE         1024  // rx/tx buffer size
#define TTY_THROTTLE_THRESH     128   // start/stop threshold (buffer pressure)

//
// ----------------------------------------------------------------------------
// Important Memory Addresses
//

// kernel base physical address
#define KERNEL_BASE             (1 * MB)

// kernel virtual address space
// the lower 1MB of physical memory is identity-mapped mapped
#if CONFIG_HIGHER_GROUND
  #define KERNEL_VA             0xC0000000
#else
  #define KERNEL_VA             0x0
#endif

//
// ----------------------------------------------------------------------------
// OS version info strings
//
#define OS_NAME                 "OH-WES"
#define OS_VERSION              "0.1"
#define OS_AUTHOR               "Wes Hampson"
#define OS_COPYRIGHT            "Copyright (C) 2020-2026 " OS_AUTHOR ". " \
                                "All Rights Reserved."

//
// ----------------------------------------------------------------------------
// Parameter bounds checking
//

#if !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)

static_assert(CONFIG_INITIAL_VT >= 1 && CONFIG_INITIAL_VT <= NR_TERMINAL,
    "invalid CONFIG_INITIAL_VT value");
static_assert(CONFIG_VT_CONSOLE_NUM >= 0 && CONFIG_VT_CONSOLE_NUM <= NR_TERMINAL,
    "invalid CONFIG_VT_CONSOLE_NUM value");
#endif  // !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)

#endif  // __CONFIG_H
