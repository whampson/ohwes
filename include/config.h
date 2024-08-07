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

#define MIN_KB_REQUIRED         639     // let's see how long this lasts!
#define PRINT_MEMORY_MAP        1
#define PRINT_IOCTL             0
#define PRINT_PAGE_MAP          1
#define E9_HACK                 1

#define BOOT_MEMMAP             0x1000
#define KERNEL_PGDIR            0x2000
#define KERNEL_PGTBL            0x3000
#define KERNEL_LMA              0x10000     // physical load address
#define KERNEL_INIT_STACK       KERNEL_LMA  // grows toward 0

#define NUM_CONSOLES            7

#define IDT_COUNT               256

/*----------------------------------------------------------------------------*
 * VGA Stuff
 *----------------------------------------------------------------------------*/
// http://www.ctyme.com/intr/rb-0069.htm
// https://www.stanislavs.org/helppc/int_10-0.html

// text mode enum
#define MODE_02h                0x02        // 80x25,B8000,16gray
#define MODE_03h                0x03        // 80x25,B8000,16
#define MODE_07h                0x07        // 80x25,B0000,mono

// font enum
#define VGA_FONT_80x28          1           // INT 10h,AX=1111h
#define VGA_FONT_80x50          2           // INT 10h,AX=1112h
#define VGA_FONT_80x25          4           // INT 10h,AX=1114h

// frame buffer select enum
#define VGA_FB_128K             0           // A0000-BFFFF
#define VGA_FB_64K              1           // A0000-AFFFF
#define VGA_FB_32K_LO           2           // B0000-B7FFF
#define VGA_FB_32K_HI           3           // B8000-BFFFF

// --------------------------------------------------------------------------

// VGA params
#define VGA_MODE_SELECT         MODE_03h
#define VGA_FONT_SELECT         VGA_FONT_80x28
#define VGA_FB_SELECT           VGA_FB_64K

#endif // __CONFIG_H
