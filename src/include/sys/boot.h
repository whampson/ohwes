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

#include <hw/x86.h>


/**
 * Boot loader code and stack addresses.
 */
#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         0x7E00
#define BOOT_STACK          (STAGE1_BASE)   // grows toward 0


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
 * Final kernel load address, entry point, and stack ptr.
 */
#define KERNEL_BASE         0x100000    // = 1M
#define KERNEL_ENTRY        (KERNEL_BASE)


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

#ifdef __ASSEMBLER__        // Assembler-only macros

.macro PRINT StrLabel
    leaw    \StrLabel, %si
    call    PrintStr
.endm

#else

#include <stdbool.h>
#include <stdint.h>

typedef union HwFlags {
    struct {
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
    };
    uint32_t _value;
} HwFlags;
_Static_assert(sizeof(HwFlags) == 4, "sizeof(HwFlags) == 4");

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

// struct BootInfo
// {
//     HwFlags     m_hwFlags;
//     uint16_t    m_a20Method;
//     uint16_t    m_ramCapacity_LoLegacy; /* Contiguous 1K pages up to 640K (BIOS INT=12h) */
//     uint16_t    m_ramCapacity_HiLegacy; /* Contiguous 1K pages 1M to 16M (BIOS INT=15h,AX=E801h) */
//     uint16_t    m_ramCapacity_LoE801h;  /* Contiguous 1K pages 1M to 16M or 64M (BIOS INT=15h,AX=E801h) */
//     uint16_t    m_ramCapacity_HiE801h;  /* Contiguous 64K pages 16M to 4G (BIOS INT=15h,AX=E801h) */

//     const AcpiMemoryMapEntry *m_pMemoryMap;
// };


typedef struct BootParams {
    HwFlags                     m_HwFlags;
    uint32_t                    m_A20Method;
    uint32_t                    m_VideoCols;
    uint32_t                    m_VideoMode;
    uint32_t                    m_VideoPage;
    uint32_t                    m_CursorStartLine;
    uint32_t                    m_CursorEndLine;
    uint32_t                    m_CursorRow;
    uint32_t                    m_CursorColumn;
    uint32_t                    m_HasAcpiMemoryMap;
    const AcpiMemoryMapEntry    *m_pAcpiMemoryMap;
    uint32_t                    m_RamLo_Legacy;
    uint32_t                    m_RamHi_Legacy;
    uint32_t                    m_RamLo_E801h;
    uint32_t                    m_RamHi_E801h;
    uint32_t                    m_KernelSize;
} BootParams;
// TODO: offset checks

extern BootParams *g_pBootParams;

#endif


/**
 * ----------------------------------------------------------------------------
 * -------------------------------- Memory Map --------------------------------
 * ----------------------------------------------------------------------------
 */


/**
 * ---------------------------------- Page 0 ----------------------------------
 */


// (reserved for Real Mode IDT and BIOS Data Area)


/**
 * ---------------------------------- Page 1 ----------------------------------
 */


/**
 * Interrupt Descriptor Table
 */
#define IDT_COUNT           256
#define IDT_BASE            0x1000
#define IDT_LIMIT           (IDT_BASE+(IDT_COUNT*DESC_SIZE-1))
#define IDT_SIZE            (IDT_LIMIT+1)


/**
 * Global Descriptor Table
 */
#define GDT_COUNT           8
#define GDT_BASE            0x1800
#define GDT_LIMIT           (GDT_BASE+(GDT_COUNT*DESC_SIZE-1))
#define GDT_SIZE            (GDT_LIMIT+1)

#define EARLY_CS            0x08    // code segment in early GDT
#define EARLY_DS            0x10    // data segment in early GDT


/**
 * Local Descriptor Table
 */
#define LDT_COUNT           2
#define LDT_BASE            0x1840
#define LDT_LIMIT           (LDT_BASE+(LDT_COUNT*DESC_SIZE-1))
#define LDT_SIZE            (LDT_LIMIT+1)


/**
 * Task State Segment
 */
#define TSS_BASE            0x1880
#define TSS_LIMIT           (TSS_BASE+TSS_SIZE-1)


/**
 * ---------------------------------- Page 2 ----------------------------------
 */


/**
 * ACPI Memory Map
 */
#define MEMMAP_BASE         0x2000


/**
 * ---------------------------------- Page 3 ----------------------------------
 */


/**
 * FAT Root Directory
 */
#define ROOTDIR_BASE        0x3000


#endif  // __BOOT_H
