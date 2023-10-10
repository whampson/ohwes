/* =============================================================================
 * Copyright (C) 2023 Wes Hampson. All Rights Reserved.
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
 *         File: boot/boot.h
 *      Created: March 21, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef BOOT_H
#define BOOT_H

#include <hw/x86.h>

/**
 * Boot loader code and stack addresses.
 */
#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         0x7E00
#define EARLY_STACK_BASE    (STAGE1_BASE)   // grows toward 0

/**
 * Initial kernel load address.
 *
 * We can't access memory above 1M until we switch to Protected Mode, but doing
 * so removes our ability to easily access the disk via the BIOS, so we need a
 * temporary place to put the kernel before switching into Protected Mode.
 */
#define EARLY_KERNEL_SEG    0x1000
#define EARLY_KERNEL_BASE   0x0000      // 1000:0000h = 64k = 0x010000

/**
 * Final kernel load address and entry point.
 */
#define KERNEL_BASE         0x100000    // = 1M
#define KERNEL_ENTRY        (KERNEL_BASE)

// /**
//  * Maximum kernel size.
//  *
//  * Addresses 0x80000-0xFFFFF are not safely accessible, so the kernel must not
//  * extend past this address at it's initial load point. If we do ever cross this
//  * boundary (hopefully not), we'll have to load the kernel in stages, which
//  * would require a working floppy disk driver (or Unreal Mode).
//  */
// #define MAX_KERNEL_SIZE     (0x80000-((EARLY_KERNEL_SEG<<4)+EARLY_KERNEL_BASE))

/**
 * BIOS Data Area
 * See https://stanislavs.org/helppc/bios_data_area.html
 */
#define BIOS_DATA_SEG       0x0040  // BIOS Data Area segment

// the following addresses are offsets relative to BIOS_DATA_SEG.
#define BIOS_EBDA           0x000E  // Extended BIOS Data Area segment address
#define BIOS_RESETFLAG      0x0072  // reset mode address

// values that can be written to address stored at BIOS_RESETFLAG
#define RESETFLAG_WARMBOOT  0x1234  // perform a warm boot (no memory test)
#define RESETFLAG_KEEPMEM   0x4321  // preserve memory
#define RESETFLAG_SUSPEND   0x5678  // suspend instead of reboot

/**
 * A20 enable methods.
 */
#define A20_NONE            0       // A20 already enabled (emulators only)
#define A20_KEYBOARD        1       // A20 enabled via PS/2 keyboard controller
#define A20_FAST            2       // A20 enabled via IO port 92h
#define A20_BIOS            3       // A20 enabled via BIOS INT=15h,AX=2401h

/**
 * VGA mode stuff.
 * See http://www.ctyme.com/intr/rb-0069.htm
 */
#define VGA_MODE            0x03    // 0x03 = text,CGA/EGA/VGA,16fg/8bg,0xB8000
#define VGA_CLEAR           0       // clear screen toggle

/**
 * Relevant FAT Directory Entry field offsets.
 */
#define DIRENTRY_LABEL      0       // file name/extension/label
#define DIRENTRY_CLUSTER    26      // index of first cluster in chain
#define DIRENTRY_SIZE       28      // file size in bytes

/**
 * Floppy drive read error retry count.
 */
#define RETRY_COUNT         3

/**
 * Interrupt Descriptor Table
 */
#define IDT_COUNT           256
#define IDT_BASE            0x0000
#define IDT_LIMIT           (IDT_BASE+(IDT_COUNT*DESC_SIZE-1))

/**
 * Global Descriptor Table
 */
#define GDT_COUNT           8
#define GDT_BASE            0x0800
#define GDT_LIMIT           (GDT_BASE+(GDT_COUNT*DESC_SIZE-1))
#define EARLY_CS            0x08    // code segment in early GDT
#define EARLY_DS            0x10    // data segment in early GDT

/**
 * Local Descriptor Table
 */
#define LDT_COUNT           2
#define LDT_BASE            0x0840
#define LDT_LIMIT           (LDT_BASE+(LDT_COUNT*DESC_SIZE-1))

/**
 * Task State Segment
 */
#define TSS_BASE            0x0900
#define TSS_LIMIT           (TSS_BASE+TSS_SIZE-1)

/**
 * ACPI Memory Map
 */
#define MEMMAP_BASE         0x0A00

/**
 * FAT Root Directory
 */
#define ROOTDIR_BASE        0x1000

// #ifndef __ASSEMBLER__
// _Static_assert(IDT_BASE+IDT_LIMIT <= GDT_BASE, "IDT_BASE+IDT_LIMIT <= GDT_BASE");
// _Static_assert(GDT_BASE+GDT_LIMIT <= LDT_BASE, "GDT_BASE+GDT_LIMIT <= LDT_BASE");
// _Static_assert(LDT_BASE+LDT_LIMIT <= TSS_BASE, "LDT_BASE+LDT_LIMIT <= TSS_BASE");
// _Static_assert(IDT_LIMIT+GDT_LIMIT+LDT_LIMIT+TSS_SIZE <= PAGE_SIZE, "IDT_LIMIT+GDT_LIMIT+LDT_LIMIT+TSS_SIZE <= PAGE_SIZE");
// #endif

// #define PAGE_SIZE           4096

#ifdef __ASSEMBLER__
// Assembler Macros

.macro PRINT StrLabel
    leaw    \StrLabel, %si
    call    PrintStr
.endm

#endif

#endif  // BOOT_H
