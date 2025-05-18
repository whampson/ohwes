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
 *         File: src/i386/boot/x86_boot.h
 *      Created: December 14, 2023
 *       Author: Wes Hampson
 *
 * x86 Boot Stuff
 * =============================================================================
 */

#ifndef __X86_BOOT
#define __X86_BOOT

#include <kernel/config.h>
#include <i386/boot.h>
#include <i386/paging.h>
#include <i386/x86.h>

/*----------------------------------------------------------------------------*
 * Memory
 *----------------------------------------------------------------------------*/

// --- Real Mode Memory Map ---
// 00000-004FF: reserved; IVT, BDA
// 00500-0FFFF: (free)
// 01000-01FFF: ACPI memory map
// 02000-03BFF: FAT root directory
// 03C00-079FF: (free)
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

#define MEMMAP_BASE         0x1000
#define ROOTDIR_BASE        0x2000
#define STACK_BASE          0x7C00
#define STAGE2_BASE         0x7E00

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
 * Assembler-only Stuff
 *----------------------------------------------------------------------------*/

#ifdef __ASSEMBLER__
  .macro PRINT str
      leaw    \str, %si
      call    print          # defined in boot/stage1.S
  .endm
#endif

#endif // __X86_BOOT
