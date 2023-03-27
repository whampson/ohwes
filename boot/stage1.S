# =============================================================================
# Copyright (C) 2023 Wes Hampson. All Rights Reserved.
#
# This file is part of the OH-WES Operating System.
# OH-WES is free software; you may redistribute it and/or modify it under the
# terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------
#         File: boot/stage1.S
#      Created: Mar 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode
#
# Stage 1 boot loader. It's sole purpose is to load the Stage 2 boot loader.
# Designed for a 1.44MB floppy with 512-byte sectors and a FAT12 file system.
# =============================================================================

#include "boot.h"

RESETMODE               = 0x0472                # Soft reset mode address
RESETMODE_NOMEMTEST     = 0x1234                # Skip memory test on soft reset

.text
.code16

#
# Global entry point for boot loader.
# Do NOT move or add to this.
#
.globl Entry16
Entry16:
        jmp             Boot
        nop

#
# BIOS Parameter Block for a FAT12-formatted 3.5" 1.44MB floppy disk.
# Do NOT move this. Tweak with care.
#
OemName:                .ascii                  "whampson"
SectorSize:             .short                  512
SectorsPerCluster:      .byte                   1
ReservedSectorCount:    .short                  1
TableCount:             .byte                   2
RootDirCapacity:        .short                  224
SectorCount:            .short                  2880
MediaType:              .byte                   0xF0
SectorsPerTable:        .short                  9
SectorsPerTrack:        .short                  18
HeadCount:              .short                  2
HiddenSectorCount:      .int                    0
SectorCountLarge:       .int                    0
DriveNumber:            .byte                   0
_Reserved:              .byte                   0
Signature:              .byte                   0x29
VolumeId:               .int                    0xCA55E77E
Label:                  .ascii                  "OH-WES     "
FsType:                 .ascii                  "FAT12   "

#
# Initial boot loader code.
# Gotta be tight; file must assemble to exactly 512 bytes!
#
Boot:
        cli                                     # interrupts off
        cld                                     # ensure DF=0
        xorw            %ax, %ax                # zero some segment regs
        movw            %ax, %ds
        movw            %ax, %es

        # TODO: CHS to LBA
        # TODO: locate BOOT.SYS within FAT file system

        # hardcoded sector for now
_Boot_LoadStage2:
        movw            $STAGE2_BASE, %bx       # dest
        movb            $0, %dh                 # head
        movb            $0, %ch                 # cylinder
        movb            $2, %cl                 # sector (indexed at 1)
        movb            $16, %al                # sector count
        movb            DriveNumber, %dl
        movb            $0x02, %ah
        int             $0x13
        jc              _Boot_DiskError

_Boot_GoToStage2:
        jmp             Stage2                  # onward!

_Boot_DiskError:
        leaw            s_DiskError, %si
        call            Bios_Print
        call            Bios_ReadKey
        movw            $RESETMODE_NOMEMTEST, RESETMODE
        ljmp            $0xFFFF, $0x0000

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Bios_Print
# -----------------------------------------------------------------------------
# Prints a zero-terminated string at the current curusor location and
# advances the cursor.
#
# Inputs:
#      ds:si - address of string to print
# Clobbers:
#      (potentially everything due to BIOS call)
#
.globl Bios_Print
Bios_Print:
        xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              _Bios_Print_Done
        int             $0x10
        jmp             Bios_Print
_Bios_Print_Done:
        ret

.globl Bios_PrintLn
Bios_PrintLn:
        call            Bios_Print
        leaw            s_Newline, %si
        call            Bios_Print
        ret

# -----------------------------------------------------------------------------
# Bios_ReadKey
# -----------------------------------------------------------------------------
# Waits for keyboard input.
#
# Returns:
#      al - key pressed
# Clobbers:
#      (potentially everything due to BIOS call)
#
.globl Bios_ReadKey
Bios_ReadKey:
        xorb            %ah, %ah
        int             $0x16
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

s_DiskError:
        .ascii          "\r\nDiskette read error!"
        .asciz          "\r\nPress any key to try again..."
s_Newline:
        .asciz          "\r\n"

        .space          0x1FE - (. - Entry16), 0xCC     # padding
        .short          0xAA55                          # boot signature
