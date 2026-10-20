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
 *         File: i386/boot/bootldr.h
 *      Created: June 08, 2026
 *       Author: Wes Hampson
 *  Environment: C and ASM, Real Mode and Protected Mode
 * =============================================================================
 */

#ifndef __I386_BOOTLDR_H
#define __I386_BOOTLDR_H

#include <i386/boot.h>
#include <i386/paging.h>
#include <i386/x86.h>

/*----------------------------------------------------------------------------*
 * Memory
 *----------------------------------------------------------------------------*/

// --- Real Mode Memory Map ---
// 00000-004FF: reserved; IVT, BDA
// 00500-0FFFF: (free)
// 01000-01FFF: INT-15h,AX=E820h ACPI memory map
// 02000-03BFF: FAT root directory
// 03C00-079FF: (free)
// 07A00-07BFF: Real Mode stack
// 07C00-07DFF: stage 1 load/execute/data area
// 07E00-09FFF: stage 2 load/execute/data area
// 0A000-9FBFF: kernel image initial load area
// 9FC00-9FFFF: reserved; Extended BIOS Data Area (EBDA)
// A0000-FFFFF: reserved; ROM, hardware

// Absolute Base Addresses
#define MEMMAP_BASE         0x1000      // ACPI memory map
#define FAT_ROOTDIR_BASE    0x2000      // boot disk root directory
#define STAGE1_BASE         0x7C00      // stage1 load/execute address
#define STAGE2_BASE         0x7E00      // stage2 load/execute address
#define STACK_BASE          STAGE1_BASE // grows down, towards 0
#define KERNEL_LOAD         0xA000      // initial kernel load address

// Segment Addresses
#define BDA_SEGMENT         0x0040
#define KERNEL_SEGMENT      (KERNEL_LOAD >> 4)

/*----------------------------------------------------------------------------*
 * Boot Media
 *----------------------------------------------------------------------------*/

#if CONFIG_FLOPPY_BOOT
  #define DISK_MEDIA_TYPE   0xF0  // 3.5" high-density floppy (1.44 MB)
  #define DISK_SPT          18
  #define DISK_HEAD_COUNT   2
  #define DISK_DRIVE_NUMBER 0
#else // hard disk or USB, usually
  #define DISK_MEDIA_TYPE   0xF8  // fixed disk
  #define DISK_SPT          63    // sectors per track
  #define DISK_HEAD_COUNT   255
  #define DISK_DRIVE_NUMBER 0x80
#endif

// log2 of the sector size
#define SECTOR_SHIFT        9

// FAT12/16 Directory Entry Fields
#define DIRENTRY_CLUSTER    26   // first cluster in chain
#define DIRENTRY_FILESIZE   28   // file size in bytes

#define STAGE1_MAGIC        0xAA55
#define STAGE2_MAGIC        0xDECAF
#define FILL_CHAR           0x90

/*----------------------------------------------------------------------------*
 * Assembler Macros
 *----------------------------------------------------------------------------*/
#if defined(__ASSEMBLER__)

// put a string on screen
.macro PRINT str
    leaw    \str, %si
    call    print
.endm

#endif

#endif // __I386_BOOTLDR_H
