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

#include <x86.h>

// #define PAGE_SIZE               4096

/**
 * System Descriptor Memory Mapping
 * The IDT, GDT, LDT, and TSS live within the first page of kernel-mode virtual
 * memory. The ordering of these structures is completely arbitrary;
 */

// #define NUM_IDT_ENTRIES         256
// #define NUM_GDT_ENTRIES         8
// #define NUM_LDT_ENTRIES         2

// #define IDT_BASE                (0x0000)
// #define IDT_SIZE                (NUM_IDT_ENTRIES*DESC_SIZE)

// #define GDT_BASE                (0x0800)
// #define GDT_SIZE                (NUM_GDT_ENTRIES*DESC_SIZE)

// #define LDT_BASE                (0x0840)
// #define LDT_SIZE                (NUM_LDT_ENTRIES*DESC_SIZE)

// #define TSS_BASE                (0x0900)

// #ifndef __ASSEMBLER__
// _Static_assert(IDT_BASE+IDT_SIZE <= GDT_BASE, "IDT_BASE+IDT_SIZE <= GDT_BASE");
// _Static_assert(GDT_BASE+GDT_SIZE <= LDT_BASE, "GDT_BASE+GDT_SIZE <= LDT_BASE");
// _Static_assert(LDT_BASE+LDT_SIZE <= TSS_BASE, "LDT_BASE+LDT_SIZE <= TSS_BASE");
// _Static_assert(IDT_SIZE+GDT_SIZE+LDT_SIZE+TSS_SIZE <= PAGE_SIZE, "IDT_SIZE+GDT_SIZE+LDT_SIZE+TSS_SIZE <= PAGE_SIZE");
// #endif

#define MEMMAP_BASE             0x1000

#define A20METHOD_NONE          0       /* A20 already enabled (emulators only) */
#define A20METHOD_KEYBOARD      1       /* A20 enabled via PS/2 keyboard controller */
#define A20METHOD_PORT92h       2       /* A20 enabled via IO port 92h */
#define A20METHOD_BIOS          3       /* A20 enabled via BIOS INT=15h,AX=2401h */

#endif  // BOOT_H