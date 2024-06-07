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

// --- Crude Memory Map ---
// 0x00000-0x004FF: reserved for Real Mode IVT and BDA.
// 0x00500-0x007FF: ACPI memory map buffer
// 0x00800-0x023FF: FAT root directory
// 0x02400-0x07BFF: (0x5800 bytes of free space)
// 0x07C00-0x07DFF: stage 1 boot loader
// 0x07E00-0x0????: stage 2 boot loader
// 0x0????-0x0FFFF: kernel stack (grows towards 0)
// 0x10000-(EBDA ): kernel and system

#define SEG2FLAT(seg,off)   (((seg)<<4)+(off))
#define FLAT2SEG(flat)      (((flat)&0xF0000)>>4)
#define FLAT2OFF(flat)      ((flat)&0xFFFF)

#define MEMMAP_BASE         0x500   // max 32 entries
#define ROOTDIR_BASE        0x800   // max 224 entries

#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         0x7E00

#define STACK_SEGMENT       0x0000
#define STACK_OFFSET        0x7C00  // grows toward 0

#define KERNEL_SEGMENT      FLAT2SEG(KERNEL_BASE)  // kernel segment address
#define KERNEL_OFFSET       FLAT2OFF(KERNEL_BASE)  // kernel segment offset

#define INIT_SEGMENT        FLAT2SEG(INIT_BASE)
#define INIT_OFFSET         FLAT2OFF(INIT_BASE)

/*----------------------------------------------------------------------------*
 * BIOS Data Area
 * See https://stanislavs.org/helppc/bios_data_area.html
 *----------------------------------------------------------------------------*/
#define BDA_SEGMENT         0x0040  // BIOS Data Area segment address
// BDA field offsets
#define BDA_EBDA_ADDR       0x000E  // Extended BIOS Data Area segment address
#define BDA_RESETFLAG       0x0072  // reset mode address

// values that can be written to BDA_RESETFLAG
#define RESETFLAG_WARMBOOT  0x1234  // perform a warm boot (no memory test)
#define RESETFLAG_KEEPMEM   0x4321  // preserve memory
#define RESETFLAG_SUSPEND   0x5678  // suspend instead of reboot


/*----------------------------------------------------------------------------*
 * Floppy Stuff
 *----------------------------------------------------------------------------*/
#define RETRY_COUNT         3


/*----------------------------------------------------------------------------*
 * FAT Stuff
 *----------------------------------------------------------------------------*/
// FAT Directory Entry member offsets.
#define DIRENTRY_LABEL      0       // file name/extension/label
#define DIRENTRY_CLUSTER    26      // index of first cluster in chain
#define DIRENTRY_FILESIZE   28      // file size in bytes
#define SIZEOF_DIRENTRY     32      // size of dir entry itself


/*----------------------------------------------------------------------------*
 * VGA Stuff
 * See http://www.ctyme.com/intr/rb-0069.htm
 *----------------------------------------------------------------------------*/
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
  .macro PRINT StrLabel
      leaw    \StrLabel, %si
      call    PrintStr
  .endm
#endif

#endif // __X86_BOOT
