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

#define A20_NONE            0   // A20 already enabled (emulators only)
#define A20_KEYBOARD        1   // A20 enabled via PS/2 keyboard controller
#define A20_PORT92h         2   // A20 enabled via IO port 92h
#define A20_BIOS            3   // A20 enabled via BIOS INT=15h,AX=2401h

#define IDT_COUNT           256
#define GDT_COUNT           8
#define LDT_COUNT           2

#define IDT_BASE            0x0000
#define IDT_LIMIT           (IDT_BASE+(IDT_COUNT*DESC_SIZE-1))

#define GDT_BASE            0x0800
#define GDT_LIMIT           (GDT_BASE+(GDT_COUNT*DESC_SIZE-1))

#define LDT_BASE            0x0840
#define LDT_LIMIT           (LDT_BASE+(LDT_COUNT*DESC_SIZE-1))

#define TSS_BASE            0x0900
#define TSS_LIMIT           (TSS_BASE+TSS_SIZE-1)

#define MEMMAP_BASE         0x0A00

#define ROOTDIR_BASE        0x1000

#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         0x7E00

#define KERNEL_SEG_EARLY    0x1000
#define KERNEL_BASE_EARLY   0x0000      // 64k = 0x010000
#define KERNEL_BASE         0x100000    // 1M  = 0x100000
#define KERNEL_ENTRY        (KERNEL_BASE)

// #ifndef __ASSEMBLER__
// _Static_assert(IDT_BASE+IDT_LIMIT <= GDT_BASE, "IDT_BASE+IDT_LIMIT <= GDT_BASE");
// _Static_assert(GDT_BASE+GDT_LIMIT <= LDT_BASE, "GDT_BASE+GDT_LIMIT <= LDT_BASE");
// _Static_assert(LDT_BASE+LDT_LIMIT <= TSS_BASE, "LDT_BASE+LDT_LIMIT <= TSS_BASE");
// _Static_assert(IDT_LIMIT+GDT_LIMIT+LDT_LIMIT+TSS_SIZE <= PAGE_SIZE, "IDT_LIMIT+GDT_LIMIT+LDT_LIMIT+TSS_SIZE <= PAGE_SIZE");
// #endif

// #define PAGE_SIZE           4096

#ifdef __ASSEMBLER__

.macro PRINT StrLabel
    leaw    \StrLabel, %si
    call    PrintStr
.endm

.macro PRINTLN StrLabel
    leaw    \StrLabel, %si
    call    PrintStr
    leaw    s_NewLine, %si
    call    PrintStr
.endm

#endif

#endif  // BOOT_H
