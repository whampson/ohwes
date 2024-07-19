/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: boot/x86_boot.h
 *      Created: December 14, 2023
 *       Author: Wes Hampson
 *
 * x86 Boot Stuff
 * =============================================================================
 */

#ifndef __X86_BOOT
#define __X86_BOOT

#include <kernel.h>
#include <x86.h>

/*----------------------------------------------------------------------------*
 * Memory
 *----------------------------------------------------------------------------*/

// --- Real Mode Memory Map ---
// 00000-004FF: reserved; IVT, BDA
// 00500-0FFFF: (free)
// 01000-02BFF: FAT root directory
// 02C00-02FFF: ACPI memory map
// 03000-079FF: (free)
// 07A00-07BFF: real mode stack
// 07C00-07DFF: stage 1
// 07E00-0FFFF: stage 2
// 10000-9FBFF: kernel image
// 9FC00-9FFFF: reserved; EBDA
// A0000-FFFFF: reserved; ROM, hardware

#define seg2flat(seg,off)   (((seg)<<4)+(off))

#define SEGMENT_SHIFT       16
#define SEGMENT_SIZE        (1 << SEGMENT_SHIFT)

#define BDA_SEGMENT         0x0040
#define KERNEL_SEGMENT      (KERNEL_BASE >> 4)

#define BOOT_CS             0x08
#define BOOT_DS             0x10

#define ROOTDIR_BASE        0x1000

/*----------------------------------------------------------------------------*
 * BIOS Data Area
 * See https://stanislavs.org/helppc/bios_data_area.html
 *----------------------------------------------------------------------------*/

// BDA field offsets
#define BDA_EBDA_ADDR       0x000E  // Extended BIOS Data Area segment address
#define BDA_RESETFLAG       0x0072  // reset mode address

// values that can be written to BDA_RESETFLAG
#define WARMBOOT            0x1234  // perform a warm boot (no memory test)
#define KEEPMEM             0x4321  // preserve memory
#define SUSPEND             0x5678  // suspend instead of reboot


/*----------------------------------------------------------------------------*
 * Floppy Stuff
 *----------------------------------------------------------------------------*/

#define RETRY_COUNT         3

#define SECTOR_SHIFT        9
#define SECTOR_SIZE         (1 << SECTOR_SHIFT)


/*----------------------------------------------------------------------------*
 * FAT Stuff
 *----------------------------------------------------------------------------*/

#define FILENAME_LENGTH     11

#define DIRENTRY_SHIFT      5
#define DIRENTRY_SIZE       (1 << DIRENTRY_SHIFT)

#define CLUSTER_NUM_OFFSET  2       // clusters 0 and 1 are reserved

// FAT Directory Entry member offsets.
#define LABEL               0       // file name/extension/label
#define CLUSTER             26      // index of first cluster in chain
#define FILESIZE            28      // file size in bytes


/*----------------------------------------------------------------------------*
 * VGA Stuff
 * See http://www.ctyme.com/intr/rb-0069.htm
 *----------------------------------------------------------------------------*/
// https://www.stanislavs.org/helppc/int_10-0.html

// Text Modes
#define MODE_02h          0x02        // 80x25,B8000,16gray
#define MODE_03h          0x03        // 80x25,B8000,16
#define MODE_07h          0x07        // 80x25,B0000,mono

#define VGA_MODE          MODE_03h

/*----------------------------------------------------------------------------*
 * Assembler-only Stuff
 *----------------------------------------------------------------------------*/

#ifdef __ASSEMBLER__
  .macro PRINT str
      leaw    \str, %si
      call    print_string          # defined in boot/stage1.S
  .endm
#endif


#endif // __X86_BOOT
