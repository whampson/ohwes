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

#define IDT_BASE                0x0000
#define IDT_SIZE                (256*8)

#define GDT_BASE                (IDT_BASE+IDT_SIZE)
#define GDT_SIZE                (8*8)

#define LDT_BASE                (GDT_BASE+GDT_SIZE)
#define LDT_SIZE                (2*8)

#define TSS_BASE                (LDT_BASE+LDT_SIZE)
#define TSS_SIZE                (108)

#define MEMMAP_BASE             0x1000

#define A20METHOD_NONE          0       /* A20 already enabled (emulators only) */
#define A20METHOD_KEYBOARD      1       /* A20 enabled via PS/2 keyboard controller */
#define A20METHOD_PORT92h       2       /* A20 enabled via IO port 92h */
#define A20METHOD_BIOS          3       /* A20 enabled via BIOS INT=15h,AX=2401h */

#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stdint.h>

_Static_assert(TSS_BASE + TSS_SIZE < MEMMAP_BASE, "TSS overlaps memory map!");

typedef struct HwFlags {
    uint16_t HasDisketteDrive       : 1;
    uint16_t HasCoprocessor         : 1;
    uint16_t HasPs2Mouse            : 1;
    uint16_t                        : 1;
    uint16_t VideoMode              : 2;
    uint16_t NumOtherDisketteDrives : 2;
    uint16_t _Dma                   : 1;
    uint16_t NumSerialPorts         : 3;
    uint16_t HasGamePort            : 1;
    uint16_t _HasPrinterOrModem     : 1;
    uint16_t NumParallelPorts       : 2;
} HwFlags;
_Static_assert(sizeof(HwFlags) == 2, "sizeof(HwFlags)");

enum HwFlagsVideoMode {
    HWFLAGS_VIDEOMODE_INVALID       = 0,
    HWFLAGS_VIDEOMODE_40x25         = 1,
    HWFLAGS_VIDEOMODE_80x25         = 2,
    HWFLAGS_VIDEOMODE_80x25_MONO    = 3,
};

typedef struct AcpiMemoryMapEntry {
    uint64_t Base;
    uint64_t Length;
    uint32_t Type;
    uint32_t Attributes;
} AcpiMemoryMapEntry;
_Static_assert(sizeof(AcpiMemoryMapEntry) == 24, "sizeof(AcpiMemoryMapEntry)");

typedef AcpiMemoryMapEntry AcpiMemoryMap;

enum AcpiMemoryMapType {
    ACPI_MMAP_TYPE_INVALID  = 0,    /* (invalid table entry, ignore) */
    ACPI_MMAP_TYPE_USABLE   = 1,    /* Available, free for use */
    ACPI_MMAP_TYPE_RESERVED = 2,    /* Reserved, do not use */
    ACPI_MMAP_TYPE_ACPI     = 3,    /* ACPI tables, can be reclaimed */
    ACPI_MMAP_TYPE_ACPI_NVS = 4,    /* ACPI non-volatile storage, do not use */
    ACPI_MMAP_TYPE_BAD      = 5,    /* Bad memory, do not use */
    /* Other values are reserved or OEM-specific, do not use */
};

extern uint16_t g_HwFlags;

extern uint8_t g_A20Method;
extern bool g_HasAcpiMemoryMap;

extern uint16_t g_RamLo_Legacy;         /* Contiguous 1K pages up to 640K (BIOS INT=12h) */
extern uint16_t g_RamHi_Legacy;         /* Contiguous 1K pages 1M to 16M (BIOS INT=15h,AX=E801h) */

extern uint16_t g_RamLo_E801h;          /* Contiguous 1K pages 1M to 16M or 64M (BIOS INT=15h,AX=E801h) */
extern uint16_t g_RamHi_E801h;          /* Contiguous 64K pages 16M to 4G (BIOS INT=15h,AX=E801h) */

extern const AcpiMemoryMapEntry *g_AcpiMemoryMap;

#endif  // __ASSEMBLER__

#endif  // BOOT_H
