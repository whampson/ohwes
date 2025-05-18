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

#include <i386/paging.h>

//
// General Configuration
// ----------------------------------------------------------------------------

// memory
#define MIN_KB              512 // let's see how long this lasts!
#define HIGHER_GROUND       1   // map kernel in high virtual address space

// printing
#define PRINT_LOGO          0   // show a special logo at boot
#define PRINT_MEMORY_MAP    1   // show BIOS memory map at boot
#define PRINT_PAGE_MAP      0   // show initial page table mappings
#define PRINT_IOCTL         1   // show ioctl calls
#define E9_HACK             1   // tee console output to port 0xE9

// debugging
#define SERIAL_DEBUGGING    1   // enable debugging over COM port
#define SERIAL_DEBUG_PORT   COM1_PORT
#define SERIAL_DEBUG_BAUD   BAUD_115200
#define ENABLE_CRASH_KEY    0   // test various crash scenarios w/ keystroke

// console assignments
#define SERIAL_CONSOLE_NUM  2         // serial console TTY number
#define SERIAL_OUTPUT_PORT  COM2_PORT // serial console I/O port number

//
// Counts of Things
// ----------------------------------------------------------------------------
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
// Important Memory Addresses
// All addresses are physical unless otherwise noted.
//
// Stacks are PAGE_SIZE bytes and /grow in the negative direction/ towards 0.
// Stack base addresses are offset by +4 bytes from the written data.
// ----------------------------------------------------------------------------

#define FRAME_SIZE          (PAGE_SIZE*2)

// memory regions
#define STACK_MEMORY        0x10000
#define STATIC_MEMORY       0x1C000

// stack memory
#define SETUP_STACK         (STACK_MEMORY+(FRAME_SIZE*0))   // = 0F000-0FFFF
#define DOUBLE_FAULT_STACK  (STACK_MEMORY+(FRAME_SIZE*1))   // = 10000-10FFF
#define INT_STACK_LIMIT     (DOUBLE_FAULT_STACK)
#define INT_STACK_BASE      (STACK_MEMORY+(FRAME_SIZE*4))
#define NR_INT_STACKS       ((INT_STACK_BASE - INT_STACK_LIMIT) / FRAME_SIZE)

// static memory
#define KERNEL_PGDIR        (STATIC_MEMORY+(PAGE_SIZE*0))   // = 1C000-1CFFF
#define KERNEL_PGTBL        (STATIC_MEMORY+(PAGE_SIZE*1))   // = 1D000-1DFFF
#define KERNEL_BASE         (STATIC_MEMORY+(PAGE_SIZE*4))   // = 20000-?????

#if !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)
static_assert(INT_STACK_BASE <= KERNEL_PGDIR, "Interrupt stacks overlap static data!");
static_assert(INT_STACK_LIMIT >= DOUBLE_FAULT_STACK, "Interrupt stacks overlap critical stacks!");
#endif

// kernel virtual address space
// the lower 1MB of physical memory is identity-mapped mapped
#if HIGHER_GROUND
  #define KERNEL_VA     0xC0000000
#else
  #define KERNEL_VA     0x0
#endif

//
// VGA Stuff
// ----------------------------------------------------------------------------
// See doc/vga.txt

// constants
#define _VGA_80x28 1
#define _VGA_80x50 2
#define _VGA_80x25 4

// params
#define VGA_MODE            3       // 3 = 80x25,B8000,16color
#define VGA_FB_SELECT       0       // 0 = 0xA0000-0xBFFFF 128k
#define VGA_DIMENSION      _VGA_80x28

#endif // __CONFIG_H
