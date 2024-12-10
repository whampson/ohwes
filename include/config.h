/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: include/config.h
 *      Created: July 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

//
// General Configuration
// ----------------------------------------------------------------------------

#define MIN_KB              512     // let's see how long this lasts!
#define PRINT_LOGO          0       // show a special logo at boot
#define PRINT_MEMORY_MAP    1       // show BIOS memory map at boot
#define PRINT_PAGE_MAP      0       // show initial page table mappings
#define PRINT_IOCTL         0       // show ioctl calls
#define E9_HACK             1       // tee console output to port 0xE9
#define HIGHER_GROUND       0       // map kernel in high virtual address space

//
// Important Memory Addresses
// All addresses are physical unless otherwise noted.
//
// Stacks are PAGE_SIZE bytes and /grow in the negative direction/ towards 0.
// Stack base addresses are offset by +4 bytes from the written data.
// ----------------------------------------------------------------------------

#define INITIAL_STACK       0x10000
#define INTERRUPT_STACK     0x11000
#define USER_STACK          0x12000
#define DOUBLE_FAULT_STACK  0x13000 // page must be present in kernel mode
#define KERNEL_PGDIR        0x13000 // global page directory
#define KERNEL_PGTBL        0x14000 // kernel page table
#define KERNEL_BASE         0x15000 // kernel image load address


// Kernel space base virtual address. The lower 1MB of physical memory is mapped.
#if HIGHER_GROUND // TOOD: move this toggle elsewhere
  #define KERNEL_VA_BASE    0xC0000000
#else
  #define KERNEL_VA_BASE    0x0
#endif

//
// Counts of Things
// ----------------------------------------------------------------------------

#define NR_CONSOLE          7           // number of virtual consoles
#define NR_TTY              NR_CONSOLE  // number of TTY devices
#define NR_SERIAL           4           // number of serial ports

//
// VGA Stuff
// ----------------------------------------------------------------------------
// http://www.ctyme.com/intr/rb-0069.htm
// https://www.stanislavs.org/helppc/int_10-0.html

// Modes:
//  - 2: 80x25,640x200,B8000,16gray
//  - 3: 80x25,640x200,B8000,16
//  - 7: 80x25,640x200,B0000,mono
#define VGA_MODE            3

// Fonts:
//  - 1: text mode 80x28
//  - 2: text mode 80x50
//  - 4: text mode 80x25
#define VGA_DIMENSION       1

// Frame Buffer:
//  - 0: 0xA0000-0xBFFFF 128k
//  - 1: 0xA0000-0xAFFFF 64k
//  - 2: 0xB0000-0xB7FFF 32k
//  - 3: 0xB8000-0xBFFFF 32k
#define VGA_FB_SELECT       0

#endif // __CONFIG_H
