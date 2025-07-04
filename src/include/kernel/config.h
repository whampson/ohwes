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
 *         File: src/include/kernel/config.h
 *      Created: July 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <i386/paging.h>    // for PAGE_SIZE

//
// ----------------------------------------------------------------------------
// General Configuration
//

// OS version info strings
#define OS_NAME                 "OH-WES"
#define OS_VERSION              "0.1"
#define OS_AUTHOR               "Wes Hampson"
#define OS_COPYRIGHT            "Copyright (C) 2020-2025 " OS_AUTHOR ". All Rights Reserved."

// memory
#define RAM_KBYTES              512 // let's see how long this lasts!
#define HIGHER_GROUND           1   // map kernel in high virtual address space

// terminal
#define DEFAULT_VT              1   // initial virtual terminal activated
#define VT_CONSOLE_NUM          0   // (0) print kernel messages to active virtual terminal

// serial console
#define SERIAL_CONSOLE          1   // use a serial port as a console interface
#define SERIAL_CONSOLE_NUM      1   // serial console COM port
#define SERIAL_CONSOLE_BAUD     BAUD_9600
#define SERIAL_CONSOLE_DEFAULT  0   // make the serial console the default console

// printing
#define PRINT_LOGO              0   // show a special logo at boot
#define PRINT_PAGE_MAP          0   // show initial page table mappings
#define PRINT_IOCTL             1   // show ioctl calls
#define E9_HACK                 1   // tee console output to I/O port 0xE9
#define EARLY_PRINT             1   // register default console when first char is printed

// kernel log
#define KERNEL_LOG_SIZE         (2*PAGE_SIZE)

// debugging
#define SERIAL_DEBUGGING        0   // enable debugging over COM port
#define SERIAL_DEBUG_PORT       COM1_PORT
#define SERIAL_DEBUG_BAUD       BAUD_115200
#define ENABLE_CRASH_KEY        1   // test various crash scenarios w/ keystroke

//
// ----------------------------------------------------------------------------
// Counts of Things
//

// memory
#define MAX_NR_POOLS        32    // max num concurrent pools
#define MAX_NR_POOL_ITEMS   256   // max pool memory capacity across all pools

// filesystem
#define MAX_NR_INODES       64    // max num inodes
#define MAX_NR_DENTRIES     64    // max num directory entries
#define MAX_NR_TOTAL_OPEN   64    // max num open files on system
#define MAX_NR_IO_RANGES    32    // max num I/O range reservations

// i/o
#define NR_TERMINAL         7     // number of virtual terminals
#define NR_SERIAL           4     // number of serial ports
#define MAX_PRINTBUF        4096  // max num chars in print buffer

//
// ----------------------------------------------------------------------------
// Important Memory Addresses
// All addresses are physical unless otherwise noted.
//
// Stacks are PAGE_SIZE bytes and /grow in the negative direction/ towards 0.
// Stack base addresses are offset by +4 bytes from the written data.
//

#define FRAME_SIZE          (PAGE_SIZE*2)

// memory regions
#define IDT_BASE            0x0F000
#define STACK_MEMORY        0x10000
#define STATIC_MEMORY       0x1C000

// stack memory
#define KERNEL_STACK        (STACK_MEMORY+(FRAME_SIZE*1))   // = 11FFF-10000
#define USER_STACK          (STACK_MEMORY+(FRAME_SIZE*2))   // = 13FFF-12000
#define USER_KERNEL_STACK   (STACK_MEMORY+(FRAME_SIZE*3))   // = 15FFF-14000

// static memory
#define KERNEL_PGDIR        (STATIC_MEMORY+(PAGE_SIZE*0))   // = 1C000-1CFFF
#define KERNEL_PGTBL        (STATIC_MEMORY+(PAGE_SIZE*1))   // = 1D000-1DFFF
#define KERNEL_LOG          (STATIC_MEMORY+(PAGE_SIZE*2))   // = 1E000-1FFFF
#define KERNEL_BASE         (STATIC_MEMORY+(PAGE_SIZE*4))   // = 20000-?????

// kernel virtual address space
// the lower 1MB of physical memory is identity-mapped mapped
#if HIGHER_GROUND
  #define KERNEL_VA     0xC0000000
#else
  #define KERNEL_VA     0x0
#endif

//
// ----------------------------------------------------------------------------
// Parameter bounds checking
//

#if !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)

static_assert(DEFAULT_VT >= 1 && DEFAULT_VT <= NR_TERMINAL,
    "invalid DEFAULT_VT value");
static_assert(VT_CONSOLE_NUM >= 0 && VT_CONSOLE_NUM <= NR_TERMINAL,
    "invalid VT_CONSOLE_NUM value");

static_assert(KERNEL_LOG + KERNEL_LOG_SIZE <= KERNEL_BASE,
    "kernel log buffer is too large!");

#endif  // !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)

#endif  // __CONFIG_H
