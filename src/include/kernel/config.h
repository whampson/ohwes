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

//
// General Configuration
// ----------------------------------------------------------------------------

#define MIN_KB              512 // let's see how long this lasts!
#define PRINT_LOGO          0   // show a special logo at boot
#define PRINT_MEMORY_MAP    1   // show BIOS memory map at boot
#define PRINT_PAGE_MAP      0   // show initial page table mappings
#define PRINT_IOCTL         0   // show ioctl calls
#define E9_HACK             1   // tee console output to port 0xE9
#define HIGHER_GROUND       0   // map kernel in high virtual address space

//
// Counts of Things
// ----------------------------------------------------------------------------
//
#define MAX_NR_POOLS        32  // max num concurrent pools
#define MAX_NR_POOL_ITEMS   256 // max pool memory capacity across all pools

#define MAX_NR_INODES       64  // max num inodes
#define MAX_NR_DENTRIES     64  // max num directory entries
#define MAX_NR_TOTAL_OPEN   64  // max num open files on system

#define NR_CONSOLE          7   // number of virtual consoles
#define NR_SERIAL           4   // number of serial ports

//
// Important Memory Addresses
// All addresses are physical unless otherwise noted.
//
// Stacks are PAGE_SIZE bytes and /grow in the negative direction/ towards 0.
// Stack base addresses are offset by +4 bytes from the written data.
// ----------------------------------------------------------------------------

#define FRAME_SIZE          PAGE_SIZE

// memory regions
#define STACK_MEMORY        0x10000
#define STATIC_MEMORY       0x1C000

// stack memory
#define SETUP_STACK         (STACK_MEMORY+(FRAME_SIZE*0))
#define INTERRUPT_STACK     (STACK_MEMORY+(FRAME_SIZE*1))
#define DOUBLE_FAULT_STACK  (STACK_MEMORY+(FRAME_SIZE*2))

// static memory
#define KERNEL_PGDIR        (STATIC_MEMORY+(PAGE_SIZE*0))
#define KERNEL_PGTBL        (STATIC_MEMORY+(PAGE_SIZE*1))
#define KERNEL_BASE         (STATIC_MEMORY+(PAGE_SIZE*4)) // kernel image addr


// Kernel space base virtual address. The lower 1MB of physical memory is mapped.
#if HIGHER_GROUND // TOOD: move this toggle elsewhere
  #define KERNEL_VA_BASE    0xC0000000
#else
  #define KERNEL_VA_BASE    0x0
#endif

//
// VGA Stuff
// ----------------------------------------------------------------------------
// See doc/vga.txt

#define VGA_MODE            3       // 3 = 80x25,B8000,16color
#define VGA_DIMENSION       1       // 1 = text mode 80x28
#define VGA_FB_SELECT       0       // 0 = 0xA0000-0xBFFFF 128k

#endif // __CONFIG_H