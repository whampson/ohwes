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

#include <config.h>
#include <x86.h>

/*----------------------------------------------------------------------------*
 * Memory
 *----------------------------------------------------------------------------*/

#define SEG2FLAT(seg,off)   (((seg)<<4)+(off))

// --- Boot Loader Memory Map ---
// 0x00000-0x004FF: reserved (Real Mode IVT, stack, and BIOS Data Area)
// 0x00500-0x0FFFF: reserved
// 0x01000-0x02BFF: FAT root directory
// 0x02C00-0x02FFF: ACPI memory map
// 0x03000-0x07BFF: (free)
// 0x07C00-0x07DFF: stage 1
// 0x07E00-0x0FFFF: stage 2
// 0x10000-0x9FBFF: kernel
// 0x9FC00-0x9FFFF: reserved (Extended BIOS Data Area)
// 0xA0000-0xFFFFF: reserved (hardware)

#define SEGMENT_SHIFT       16
#define SEGMENT_SIZE        (1 << SEGMENT_SHIFT)

#define BDA_SEGMENT         0x0040
#define KERNEL_SEGMENT      (KERNEL_BASE >> 4)

#define ROOTDIR_BASE        0x1000
#define MEMMAP_BASE         0x2C00
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
 * VGA Stuff
 * See http://www.ctyme.com/intr/rb-0069.htm
 *----------------------------------------------------------------------------*/
// TODO: allow other modes for fun
// https://www.stanislavs.org/helppc/int_10-0.html

#define VGA_MODE            0x03// 0x03=text,CGA/EGA,80x25,9x16,16fg/8bg,0xB8000
#define VGA_CLEAR           0   // clear screen toggle


/*----------------------------------------------------------------------------*
 * GDT Segment Descriptors
 *----------------------------------------------------------------------------*/

#define BOOT_CS             0x08    // code segment in early GDT
#define BOOT_DS             0x10    // data segment in early GDT


/*----------------------------------------------------------------------------*
 * Assembler Stuff
 *----------------------------------------------------------------------------*/

#ifdef __ASSEMBLER__
  .macro PRINT str
      leaw    \str, %si
      call    print_string          # defined in boot/stage1.S
  .endm
#endif


#endif // __X86_BOOT
