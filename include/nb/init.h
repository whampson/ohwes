/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/nb/init.h                                                 *
 * Created: December 11, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __INIT_H
#define __INIT_H

#define PAGE_SIZE       4096

/* Page 0: Reserved for Real Mode IVT & BIOS */

/* Page 1: CPU Tables and Console Info */
#define CPUTABLES       0x1000
#define IDT_BASE        (CPUTABLES)         /* Interrupt Descriptor Table */
#define IDT_SIZE        (256*8)
#define GDT_BASE        (IDT_BASE+IDT_SIZE) /* Global Descriptor Table */
#define GDT_SIZE        (8*8)
#define IDT_PTR         (GDT_BASE+GDT_SIZE) /* IDT base/limit (for LGDT) */
#define GDT_PTR         (IDT_PTR+8)         /* GDT base/limit (for LIDT) */
#define CONINFO         0x1C00
#define CURSOR_ROW      (CONINFO+0)         /* Cursor position; column */
#define CURSOR_COL      (CONINFO+1)         /* Cursor position; row */
#define CURSOR_BEG      (CONINFO+2)         /* Cursor shape; scan line begin */
#define CURSOR_END      (CONINFO+3)         /* Cursur shape; scan line end */
#if (GDT_PTR+8>CONINFO)
#error "CPU Tables conflict with Console Info!"
#endif

/* Page 2: Memory Info */
#define MEMINFO         0x2000
#define MEMINFO_1       (MEMINFO+0x10)  /* INT 15h AX=E820h result */
#define MEMINFO_2A      (MEMINFO+0x08)  /* INT 15h AX=E801h result 1 */
#define MEMINFO_2B      (MEMINFO+0x0A)  /* INT 15h AX=E801h result 2 */
#define MEMINFO_3       (MEMINFO+0x00)  /* INT 15h AH=88h result */

/* Page 3: Page Directory */
#define PGDIR           0x3000

/* Page 4-12: Page Tables */
/* Each Page Table can address 4M of memory, for a total of 32M */
#define PGTBL0          0x4000
#define PGTBL1          0x5000
#define PGTBL2          0x6000
#define PGTBL3          0x7000
#define PGTBL4          0x8000
#define PGTBL5          0x9000
#define PGTBL6          0xA000
#define PGTBL7          0xB000

/* Kernel */
#define KERNEL_BASE     0x10000
#define KERNEL_ENTRY    (KERNEL_BASE)   
#define KERNEL_STACK    (KERNEL_BASE)   /* grows downward towards 0 */

#endif  /* __INIT_H */
