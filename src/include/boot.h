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
 *         File: include/boot.h
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

// ----------------------------------------------------------------------------

#ifndef __ASSEMBLER__
// C-only defines from here on out!

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*
 * Hardware Flags
 *----------------------------------------------------------------------------*/

/**
 * Known values for the "videoMode" field in HwFlags.
 */
enum HwFlagsVideoMode {
    HWFLAGS_VIDEOMODE_INVALID       = 0,
    HWFLAGS_VIDEOMODE_40x25         = 1,
    HWFLAGS_VIDEOMODE_80x25         = 2,
    HWFLAGS_VIDEOMODE_80x25_MONO    = 3,
};

/**
 * Hardware flags as returned by INT 11h "Get BIOS Equipment List".
 */
typedef union HwFlags {
    struct {
        uint16_t hasDisketteDrive       : 1;
        uint16_t hasCoprocessor         : 1;
        uint16_t hasPs2Mouse            : 1;
        uint16_t /* (unused) */         : 1;
        uint16_t videoMode              : 2; // video mode at stage1 start
        uint16_t numOtherDisketteDrives : 2;
        uint16_t _dma                   : 1;
        uint16_t numSerialPorts         : 3;
        uint16_t hasGamePort            : 1;
        uint16_t _hasPrinterOrModem     : 1;
        uint16_t numParallelPorts       : 2;
    };
    uint32_t _value;
} HwFlags;
static_assert(sizeof(HwFlags) == 4, "sizeof(HwFlags) == 4");

/*----------------------------------------------------------------------------*
 * ACPI Memory Map
 *----------------------------------------------------------------------------*/

/**
 * Values for the "type" field in an AcpiMemoryMapEntry.
 */
enum AcpiMemoryMapType {
    ACPI_MMAP_TYPE_INVALID  = 0,    // (invalid table entry, ignore)
    ACPI_MMAP_TYPE_USABLE   = 1,    // Available, free for use
    ACPI_MMAP_TYPE_RESERVED = 2,    // Reserved, do not use
    ACPI_MMAP_TYPE_ACPI     = 3,    // ACPI tables, can be reclaimed
    ACPI_MMAP_TYPE_ACPI_NVS = 4,    // ACPI non-volatile storage, do not use
    ACPI_MMAP_TYPE_BAD      = 5,    // Bad memory, do not use
    // Other values are reserved or OEM-specific, do not use
};

/**
 * Entry for ACPI Memory Map, as returned by INT 15h,AX=E820h.
 */
typedef struct AcpiMemoryMapEntry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
} AcpiMemoryMapEntry;
static_assert(sizeof(AcpiMemoryMapEntry) == 24, "sizeof(AcpiMemoryMapEntry) == 24");

/**
 * ACPI Memory Map
 * Memory map is an array of AcpiMemoryMapEntry elements; final element is zeros.
 */
typedef AcpiMemoryMapEntry AcpiMemoryMap;

/*----------------------------------------------------------------------------*
 * System Boot Info
 *----------------------------------------------------------------------------*/

/**
 * System information collected during boot and passed onto the kernel.
 */
typedef struct BootInfo {
    //
    // !!! KEEP OFFSETS IN-LINE WITH s_BootInfo IN src/boot/stage2.h !!!
    //
    intptr_t kernelBase;            // kernel image base address
    uint32_t kernelSize;            // kernel image size bytes
    intptr_t stage2Base;            // stage2 image base address
    uint32_t stage2Size;            // stage2 image size bytes
    intptr_t stackBase;             // stack base upon leaving stage2

    uint32_t ramLo_Legacy;          // 1K pages 0 to 640K (BIOS INT 12h)
    uint32_t ramHi_Legacy;          // 1K pages 1M to 16M (BIOS INT 15h,AX=88h)
    uint32_t ramLo_E801h;           // 1K pages 1M to 16M (BIOS INT 15h,AX=E801h)
    uint32_t ramHi_E801h;           // 64K pages 16M to 4G (BIOS INT 15h,AX=E801h)
    const AcpiMemoryMap *pMemoryMap;// ACPI Memory Map (BIOS INT 15h,AX=E820h)

    HwFlags hwFlags;                // system hardware flags (BIOS INT 11h)
    uint32_t a20Method;             // method used to enable A20 line, one of A20_*
    uint32_t videoMode;             // VGA video mode (BIOS INT 10h,AH=0Fh)
    uint32_t videoPage;             // VGA active display page (BIOS INT 10h,AH=0Fh)
    uint32_t videoCols;             // VGA column count (BIOS INT 10h,AH=0Fh)
    uint32_t cursorStartLine;       // VGA cursor scan line top
    uint32_t cursorEndLine;         // VGA cursor scan line bottom

    const void *m_pEbda;            // Extended BIOS Data Area
} BootInfo;

#endif  // __ASSEMBLER__

#endif  // __BOOT_H
