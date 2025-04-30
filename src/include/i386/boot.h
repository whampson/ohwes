/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 * A20 Modes
 *----------------------------------------------------------------------------*/
#define A20_NONE            0       // A20 already enabled (emulators only)
#define A20_KEYBOARD        1       // A20 enabled via PS/2 keyboard controller
#define A20_PORT92          2       // A20 enabled via IO port 92h
#define A20_BIOS            3       // A20 enabled via BIOS INT=15h,AX=2401h

// ----------------------------------------------------------------------------

#ifndef __ASSEMBLER__
// C-only defines from here on out!

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern struct boot_info *g_boot;

/*----------------------------------------------------------------------------*
 * Hardware Flags
 *----------------------------------------------------------------------------*/

/**
 * Hardware flags as returned by INT 11h "Get BIOS Equipment List".
 *
 * https://www.stanislavs.org/helppc/int_11.html
 * https://fragglet.github.io/dos-help-files/alang.hlp/11h_dot_des.html
 * http://www.ctyme.com/intr/rb-0575.htm
 */
struct hwflags {
    union {
        struct {
            uint16_t has_diskette_drive         : 1;
            uint16_t has_coprocessor            : 1;
            uint16_t has_ps2mouse               : 1;
            uint16_t                            : 1;    // unused
            uint16_t initial_video_mode         : 2;
            uint16_t num_other_diskette_drives  : 2;
            uint16_t _dma                       : 1;    // legacy
            uint16_t num_serial_ports           : 3;
            uint16_t has_gameport               : 1;
            uint16_t _has_printer_or_modem      : 1;    // legacy
            uint16_t num_parallel_ports         : 2;
        };
        uint32_t _value;
    };
};
static_assert(sizeof(struct hwflags) == 4, "sizeof(struct hwflags) == 4");

/**
 * Values for the "initial_video_mode" field in struct hwflags.
 */
enum hwflags_video_mode {
    HWFLAGS_VIDEOMODE_INVALID       = 0,
    HWFLAGS_VIDEOMODE_40x25         = 1,
    HWFLAGS_VIDEOMODE_80x25         = 2,
    HWFLAGS_VIDEOMODE_80x25_MONO    = 3,
};


/*----------------------------------------------------------------------------*
 * ACPI Memory Map
 *----------------------------------------------------------------------------*/

/**
 * ACPI Memory Types
 */
enum acpi_mmap_type {
    ACPI_MMAP_TYPE_INVALID  = 0,    // (invalid table entry, ignore)
    ACPI_MMAP_TYPE_USABLE   = 1,    // Available, free for use
    ACPI_MMAP_TYPE_RESERVED = 2,    // Reserved, do not use
    ACPI_MMAP_TYPE_ACPI     = 3,    // ACPI tables, can be reclaimed
    ACPI_MMAP_TYPE_ACPI_NVS = 4,    // ACPI non-volatile storage, do not use
    ACPI_MMAP_TYPE_BAD      = 5,    // Bad memory, do not use
    // Other values are reserved or OEM-specific, do not use
};


/**
 * ACPI Memory Map entry, as returned by INT 15h,AX=E820h.
 */
struct acpi_mmap_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
};
static_assert(sizeof(struct acpi_mmap_entry) == 24, "sizeof(struct acpi_mmap_entry) == 24");

/**
 * ACPI Memory Map
 * Memory map is an array of acpi_mmap_entry elements; final element is zeros.
 */
typedef struct acpi_mmap_entry acpi_mmap_t;


/*----------------------------------------------------------------------------*
 * System Boot Info
 *----------------------------------------------------------------------------*/

/**
 * System information collected during boot and passed onto the kernel.
 */
struct boot_info {
    //
    // !!! KEEP OFFSETS IN-LINE WITH src/boot/stage2.h !!!
    //

    // important memory addresses
    intptr_t ebda_base;             // Extended BIOS Data Area

    // memory size/layout information
    const acpi_mmap_t *mem_map;     // ACPI Memory Map (INT 15h,AX=E820h)
    uint32_t kb_low;                // 1K blocks 0 to 640K (INT 12h)
    uint32_t kb_high;               // 1K blocks 1M to 16M (INT 15h,AX=88h)
    uint32_t kb_high_e801h;         // 1K blocks 1M to 16M (INT 15h,AX=E801h)
    uint32_t kb_extended;           // 64K blocks 16M to 4G (INT 15h,AX=E801h)

    // hardware info
    struct hwflags hwflags;         // system hardware flags (INT 11h)
    uint32_t a20_method;            // method used to enable A20 line, one of A20_*

    // terminal info
    uint32_t vga_mode;              // VGA video mode (INT 10h,AH=0Fh)
    uint32_t vga_rows;              // VGA row count
    uint32_t vga_cols;              // VGA column count (INT 10h,AH=0Fh)
    uint32_t cursor_row;            // current cursor row (INT 10h,AH=03h)
    uint32_t cursor_col;            // current cursor column (INT 10h,AH=03h)
};

#endif  // __ASSEMBLER__

#endif  // __BOOT_H
