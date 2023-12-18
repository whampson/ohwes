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
 *         File: sys/boot.h
 *      Created: March 21, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __BOOT_H
#define __BOOT_H


/*----------------------------------------------------------------------------*
 * A20 Enable Methods
 *----------------------------------------------------------------------------*/
#define A20_NONE            0       // A20 already enabled (emulators only)
#define A20_KEYBOARD        1       // A20 enabled via PS/2 keyboard controller
#define A20_FAST            2       // A20 enabled via IO port 92h
#define A20_BIOS            3       // A20 enabled via BIOS INT=15h,AX=2401h

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stddef.h>

enum HwFlagsVideoMode {
    HWFLAGS_VIDEOMODE_INVALID       = 0,
    HWFLAGS_VIDEOMODE_40x25         = 1,
    HWFLAGS_VIDEOMODE_80x25         = 2,
    HWFLAGS_VIDEOMODE_80x25_MONO    = 3,
};

typedef union HwFlags {
    struct {
        uint16_t HasDisketteDrive       : 1;
        uint16_t HasCoprocessor         : 1;
        uint16_t HasPs2Mouse            : 1;
        uint16_t /* (unused) */         : 1;
        uint16_t VideoMode              : 2; // video mode at stage1 start
        uint16_t NumOtherDisketteDrives : 2;
        uint16_t _Dma                   : 1;
        uint16_t NumSerialPorts         : 3;
        uint16_t HasGamePort            : 1;
        uint16_t _HasPrinterOrModem     : 1;
        uint16_t NumParallelPorts       : 2;
    };
    uint32_t _value;
} HwFlags;
_Static_assert(sizeof(HwFlags) == 4, "sizeof(HwFlags) == 4");

typedef struct AcpiMemoryMapEntry {
    uint64_t Base;
    uint64_t Length;
    uint32_t Type;
    uint32_t Attributes;
} AcpiMemoryMapEntry;
_Static_assert(sizeof(AcpiMemoryMapEntry) == 24, "sizeof(AcpiMemoryMapEntry) == 24");

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

//
// !!! KEEP OFFSETS IN-LINE WITH s_BootParams IN src/boot/stage2.h !!!
//
typedef struct BootParams {
    intptr_t m_KernelBase;
    uint32_t m_KernelSize;
    intptr_t m_Stage2Base;
    uint32_t m_Stage2Size;
    intptr_t m_StackBase;

    uint32_t m_RamLo_Legacy;    // 1K pages 0 to 640K (BIOS INT=12h)
    uint32_t m_RamHi_Legacy;    // 1K pages 1M to 16M (BIOS INT=15h,AX=88h)
    uint32_t m_RamLo_E801h;     // 1K pages 1M to 16M (BIOS INT=15h,AX=E801h)
    uint32_t m_RamHi_E801h;     // 64K pages 16M to 4G (BIOS INT=15h,AX=E801h)
    const AcpiMemoryMap *m_pMemoryMap;  // ACPI Memory Map (BIOS INT=15h,AX=E820h)

    HwFlags m_HwFlags;          // BIOS INT=11h
    uint32_t m_A20Method;
    uint32_t m_VideoMode;
    uint32_t m_VideoPage;
    uint32_t m_VideoCols;
    uint32_t m_CursorStartLine;
    uint32_t m_CursorEndLine;

    const void *m_pEbda;        // Extended BIOS Data Area
} BootParams;

#endif  // !__ASSEMBLER_

#endif  // __BOOT_H
