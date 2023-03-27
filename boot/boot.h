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

extern uint8_t g_A20Method;

extern uint16_t g_RamCapacityLo;        // contiguous RAM <1M in 1K blocks
extern uint16_t g_RamCapacityHi;        // contiguous RAM >1M in 1K blocks, up to 15M or 64M
extern uint16_t g_RamCapacityLo_e801;   // contiguous RAM >1M in 1K blocks, up to 16M
extern uint16_t g_RamCapacityHi_e801;   // contiguous RAM >16M in 64K blocks

extern char g_bHasMemoryMap;
extern void *g_pAcpiMemoryMap;

extern uint16_t g_EquipmentFlags;

struct EquipmentFlags
{
    uint16_t DisketteDrive     : 1;
    uint16_t Coprocessor       : 1;
    uint16_t Ps2Mouse          : 1;
    uint16_t                   : 1;
    uint16_t VideoMode         : 2;     // 00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono
    uint16_t NumOtherDiskette  : 2;     // num diskette drives attached less 1
    uint16_t _Dma              : 1;
    uint16_t NumSerialPorts    : 3;
    uint16_t GamePort          : 1;
    uint16_t _PrinterOrModem   : 1;
    uint16_t NumParallelPorts  : 2;
};
_Static_assert(sizeof(struct EquipmentFlags) == 2, "sizeof(struct EquipmentFlags)");

struct MemoryMapEntry
{
    uint64_t Base;
    uint64_t Length;
    uint32_t Type;
    uint32_t ExtendedAttributes;
};
_Static_assert(sizeof(struct MemoryMapEntry) == 24, "sizeof(struct MemoryMapEntry)");

#endif  // __ASSEMBLER__

#endif  // BOOT_H
