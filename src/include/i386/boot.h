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
 *
 * A lot of the stuff in here is legacy BIOS stuff that probably isn't of much
 * use, but I think it's interesting to keep around.
 * =============================================================================
 */

#ifndef __BOOT_H
#define __BOOT_H

/*----------------------------------------------------------------------------*
 * A20 Modes
 *----------------------------------------------------------------------------*/
#define A20_NONE                0   // A20 already enabled (emulators only)
#define A20_KEYBOARD            1   // A20 enabled via PS/2 keyboard controller
#define A20_PORT92              2   // A20 enabled via IO port 92h
#define A20_BIOS                3   // A20 enabled via BIOS INT=15h,AX=2401h

/*----------------------------------------------------------------------------*
 * Struct Members and Sizes
 *----------------------------------------------------------------------------*/

// Field offsets into struct boot_info.
#define BI_EBDA_BASE            0x00
#define BI_MEM_MAP              0x04
#define BI_KB_LOW               0x08
#define BI_KB_HIGH              0x0C
#define BI_KB_HIGH_E801h        0x10
#define BI_KB_EXTENDED          0x14
#define BI_KERNEL_SIZE          0x18
#define BI_HWFLAGS              0x1C
#define BI_A20_METHOD           0x20
#define BI_VGA_MODE             0x24
#define BI_VGA_COLS             0x25
#define BI_VGA_PAGE             0x26
#define BI_HAS_VGABIOS_TABLE    0x27
#define BI_VGABIOS_TABLE_PTR    0x28
#define SIZEOF_BI               0x2C

#define SIZEOF_VGABIOS_TABLE    0x40

/*----------------------------------------------------------------------------*
 * Memory Regions
 *----------------------------------------------------------------------------*/
#define EBDA_TOP                0xA0000

// ============================================================================
//                              BEGIN C HEADER
// ============================================================================
#if !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)
// C-only defines from here on out!

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    ACPI_MMAP_TYPE_NVS      = 4,    // ACPI non-volatile storage, do not use
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
    uint32_t attr;
};
static_assert(sizeof(struct acpi_mmap_entry) == 24, "sizeof(struct acpi_mmap_entry) == 24");

#define mmap_valid(e)     ((e)->type != ACPI_MMAP_TYPE_INVALID)
#define mmap_bad(e)       ((e)->type == ACPI_MMAP_TYPE_BAD)
#define mmap_acpi(e)      ((e)->type == ACPI_MMAP_TYPE_ACPI)
#define mmap_usable(e)    ((e)->type == ACPI_MMAP_TYPE_USABLE || (e)->type == ACPI_MMAP_TYPE_ACPI)

/*----------------------------------------------------------------------------*
 * System Boot Info
 *----------------------------------------------------------------------------*/

// https://www.ctyme.com/intr/rb-0219.htm#Table39
enum vga_dcc : uint8_t {
    _DCC_NO_DISPLAY         = 0,
    _DCC_MONOCHROME         = 1,
    _DCC_CGA_COLOR          = 2,
    // 3 reserved
    _DCC_EGA_COLOR          = 4,
    _DCC_EGA_MONO           = 5,
    _DCC_PGA_COLOR          = 6,
    _DCC_VGA_MONO           = 7,
    _DCC_VGA_COLOR_ANALOG   = 8,
    // 9 reserved
    _DCC_MCGA_COLOR_DIGITAL = 10,
    _DCC_MCGA_MONO_ANALOG   = 11,
    _DCC_MCGA_COLOR_ANALOG  = 12,
    _DCC_UNKNOWN_DISPLAY    = 0xFF,
};

enum vga_misc_support_flags : uint16_t {
    _VGA_SUPPORT_ALL_MODES              = (1 << 0),
    _VGA_SUPPORT_GRAY_SUMMING           = (1 << 1),
    _VGA_SUPPORT_CHAR_FONT_LOADING      = (1 << 2),
    _VGA_SUPPORT_DEFAULT_PALETTE_LOAD_DISABLE = (1 << 3),
    _VGA_SUPPORT_CURSOR_EMULATION       = (1 << 4),
    _VGA_SUPPORT_EGA_PALETTE            = (1 << 5),
    _VGA_SUPPORT_COLOR_PALETTE          = (1 << 6),
    _VGA_SUPPORT_COLOR_REG_PAGING       = (1 << 7),
    _VGA_SUPPORT_LIGHT_PEN              = (1 << 8),
    _VGA_SUPPORT_SAVE_RESTORE           = (1 << 9),
    _VGA_SUPPORT_INTENSITY_BLINK        = (1 << 10),
    _VGA_SUPPORT_DCC                    = (1 << 11),
};

enum vga_saveptr_support_flags : uint8_t {
    _VGA_SUPPORT_512_CHARSET            = (1 << 0),
    _VGA_SUPPORT_DYNAMIC_SAVE           = (1 << 1),
    _VGA_SUPPORT_ALPHA_FONT_OVERRIDE    = (1 << 2),
    _VGA_SUPPORT_GFX_FONT_OVERRIDE      = (1 << 3),
    _VGA_SUPPORT_PALETTE_OVERRIDE       = (1 << 4),
    _VGA_SUPPORT_DCC_EXTENSION          = (1 << 5),
};

struct vgabios_static_state {
    uint8_t modes_supported[7];     // each bit corresponds to mode index
    uint8_t scan_lines_supported;   // [0]=200, [1]=350, [2]=400
    uint8_t num_char_blocks;
    uint8_t max_active_char_blocks;
    uint16_t misc_flags;            // vga_misc_support_flags
    uint16_t _reserved_0Ch;
    uint8_t saveptr_flags;          // vga_saveptr_support_flags
    uint8_t _reserved_0Fh;
} __pack __align(1);
static_assert(sizeof(struct vgabios_static_state) == 0x10);

// https://www.stanislavs.org/helppc/int_10-1b.html
// https://www.ctyme.com/intr/rb-0221.htm#Table43
struct vgabios_table {
    uint32_t static_state_addr; // address of vgabios_static_state [31:16]=segment, [15:0]=addr
    uint8_t video_mode;
    uint16_t num_cols;
    uint16_t regen_buf_size;
    uint16_t regen_buf_addr;
    uint16_t cursor_pos[8];     // per-page cursor position
    uint16_t cursor_shape;
    uint8_t curr_page;
    uint16_t crtc_port_addr;
    uint8_t port_3x8h_setting;
    uint8_t port_3x9h_setting;
    uint8_t max_row_index;
    uint16_t char_height;
    uint8_t curr_display_dcc;   // vga_dcc
    uint8_t alt_display_dcc;    // vga_dcc
    uint16_t num_colors;
    uint8_t num_pages;
    uint8_t num_scan_lines;     // 0=200, 1=350, 2=400, 3=480
    uint8_t primary_char_block;
    uint8_t secondary_char_block;
    uint8_t misc_flags;
    uint8_t non_vga_mode_support;
    uint16_t _reserved_2Fh;
    uint8_t vidmem_avail;       // 0=64K, 1=128K, 2=192K, 3=256K
    uint8_t saveptr_state_flags;
    uint8_t display_info_and_status;
    uint8_t _reserved_34h[12];
} __pack __align(1);
static_assert(sizeof(struct vgabios_table) == SIZEOF_VGABIOS_TABLE);

/**
 * System information collected during boot and passed onto the kernel.
 */
struct boot_info {

    // TODO: signature/magic

    // important memory addresses
    intptr_t ebda_base;             // Extended BIOS Data Area

    // memory size/layout information
    struct acpi_mmap_entry *mem_map;// ACPI Memory Map (INT 15h,AX=E820h)
    uint32_t kb_low;                // 1K blocks 0 to 640K (INT 12h)
    uint32_t kb_high;               // 1K blocks 1M to 16M (INT 15h,AX=88h)
    uint32_t kb_high_e801h;         // 1K blocks 1M to 16M (INT 15h,AX=E801h)
    uint32_t kb_extended;           // 64K blocks 16M to 4G (INT 15h,AX=E801h)
    uint32_t kernel_size;           // kernel size in bytes

    // hardware info
    struct hwflags hwflags;         // system hardware flags (INT 11h)
    uint32_t a20_method;            // method used to enable A20 line, one of A20_*

    // terminal info
    uint8_t vga_mode;               // VGA video mode (INT 10h,AH=0Fh)
    uint8_t vga_cols;               // VGA text mode columns
    uint8_t vga_page;               // current VGA page

    uint8_t has_vgabios_table;      // is the vgabios_table_ptr valid
    intptr_t vgabios_table_ptr;     // VGA BIOS functionality and state table
} __align(4);

static_assert(offsetof(struct boot_info, ebda_base) == BI_EBDA_BASE);
static_assert(offsetof(struct boot_info, mem_map) == BI_MEM_MAP);
static_assert(offsetof(struct boot_info, kb_low) == BI_KB_LOW);
static_assert(offsetof(struct boot_info, kb_high) == BI_KB_HIGH);
static_assert(offsetof(struct boot_info, kb_high_e801h) == BI_KB_HIGH_E801h);
static_assert(offsetof(struct boot_info, kb_extended) == BI_KB_EXTENDED);
static_assert(offsetof(struct boot_info, kernel_size) == BI_KERNEL_SIZE);
static_assert(offsetof(struct boot_info, hwflags) == BI_HWFLAGS);
static_assert(offsetof(struct boot_info, a20_method) == BI_A20_METHOD);
static_assert(offsetof(struct boot_info, vga_mode) == BI_VGA_MODE);
static_assert(offsetof(struct boot_info, vga_cols) == BI_VGA_COLS);
static_assert(offsetof(struct boot_info, vga_page) == BI_VGA_PAGE);
static_assert(offsetof(struct boot_info, has_vgabios_table) == BI_HAS_VGABIOS_TABLE);
static_assert(offsetof(struct boot_info, vgabios_table_ptr) == BI_VGABIOS_TABLE_PTR);
static_assert(sizeof(struct boot_info) == SIZEOF_BI);

#endif  // __ASSEMBLER__

#endif  // __BOOT_H
