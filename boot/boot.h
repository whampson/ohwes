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
 *      Created: Mar 21, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef BOOT_H
#define BOOT_H

#define IDT_BASE                0x0000
#define GDT_BASE                0x0800
#define MEMMAP_BASE             0x1000
#define STAGE1_BASE             0x7C00
#define STAGE2_BASE             0x7E00

#define A20METHOD_KEYBOARD      1
#define A20METHOD_PORT          2
#define A20METHOD_BIOS          3

#ifndef __ASSEMBLER__

#include <stdint.h>

extern uint8_t g_a20_method;

extern uint16_t g_ramsize_lo;        // contiguous RAM <1M in 1K blocks
extern uint16_t g_ramsize_hi;        // contiguous RAM >1M in 1K blocks, up to 15M or 64M
extern uint16_t g_ramsize_e801_lo;   // contiguous RAM >1M in 1K blocks, up to 16M
extern uint16_t g_ramsize_e801_hi;   // contiguous RAM >16M in 64K blocks

extern char g_has_memory_map;
extern struct acpi_memory_map_entry *g_acpi_memory_map;

extern uint16_t g_hwflags;

struct equipment_flags
{
    uint16_t has_diskette_drive: 1;
    uint16_t has_coprocessor   : 1;
    uint16_t has_ps2_mouse     : 1;
    uint16_t                   : 1;
    uint16_t video_mode        : 2;     // 00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono
    uint16_t num_other_diskette: 2;     // num diskette drives attached less 1
    uint16_t _dma              : 1;
    uint16_t num_serial_ports  : 3;
    uint16_t has_game_port     : 1;
    uint16_t _printer_or_modem : 1;
    uint16_t num_parallel_ports: 2;
};
_Static_assert(sizeof(struct equipment_flags) == 2, "sizeof(struct equipment_flags)");

struct acpi_memory_map_entry
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
};
_Static_assert(sizeof(struct acpi_memory_map_entry) == 24, "sizeof(struct acpi_memory_map_entry)");

#endif  // __ASSEMBLER__

#endif  // BOOT_H
