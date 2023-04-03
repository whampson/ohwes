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

#
# TODO: read BOOT.SYS (stage2) and OHWES.SYS (kernel) into memory from FAT
#

RESETMODE               = 0x0472                # Soft reset mode address
RESETMODE_NOMEMTEST     = 0x1234                # Skip memory test on soft reset

.text
.code16

#
# Global entry point for boot loader.
# Do NOT move or add to this.
#
.globl _start
_start:
        jmp             boot
        nop

#
# BIOS Parameter Block for a FAT12-formatted 3.5" 1.44MB floppy disk.
# Do NOT move this. Tweak with care.
#
oem_name:               .ascii                  "whampson"
sector_size:            .short                  512
sectors_per_cluster:    .byte                   1
reserved_sectors:       .short                  1
table_count:            .byte                   2
root_dir_capacity:      .short                  224
sector_count:           .short                  2880
media_type:             .byte                   0xF0
sectors_per_table:      .short                  9
sectors_per_track:      .short                  18
head_count:             .short                  2
hidden_sectors:         .int                    0
sector_count32:         .int                    0
drive_number:           .byte                   0
_reserved:              .byte                   0
signature:              .byte                   0x29
volume_id:              .int                    0xCA55E77E
label:                  .ascii                  "OH-WES     "
fs_type:                .ascii                  "FAT12   "

#
# Initial boot loader code.
# Gotta be tight; file must assemble to exactly 512 bytes!
#
boot:
        cli                                     # interrupts off
        cld                                     # ensure DF=0
        xorw            %ax, %ax                # zero some segment regs
        movw            %ax, %ds
        movw            %ax, %es

        # TODO: locate BOOT.SYS and OHWES.SYS within FAT file system

test_lba2chs:
        # 2 = 0,0,3
        movw            $2, %ax
        call            lba2chs
        cmpw            $0, %cx
        jne             _test_lba2chs_fail
        cmpb            $0, %dh
        jne             _test_lba2chs_fail
        cmpb            $3, %dl
        jne             _test_lba2chs_fail

        # 62 = 0,0,63
        movw            $62, %ax
        call            lba2chs
        cmpw            $0, %cx
        jne             _test_lba2chs_fail
        cmpb            $0, %dh
        jne             _test_lba2chs_fail
        cmpb            $63, %dl
        jne             _test_lba2chs_fail

        // # 63 = 0,1,1
        // movw            $63, %ax
        // call            lba2chs
        // cmpw            $0, %cx
        // jne             _test_lba2chs_fail
        // cmpb            $1, %dh
        // jne             _test_lba2chs_fail
        // cmpb            $1, %dl
        // jne             _test_lba2chs_fail

        // # 945 = 0,15,1
        // movw            $945, %ax
        // call            lba2chs
        // cmpw            $0, %cx
        // jne             _test_lba2chs_fail
        // cmpb            $15, %dh
        // jne             _test_lba2chs_fail
        // cmpb            $1, %dl
        // jne             _test_lba2chs_fail

        // # 1007 = 0,15,63
        // movw            $1007, %ax
        // call            lba2chs
        // cmpw            $0, %cx
        // jne             _test_lba2chs_fail
        // cmpb            $15, %dh
        // jne             _test_lba2chs_fail
        // cmpb            $63, %dl
        // jne             _test_lba2chs_fail

        // # 1008 = 1,0,1
        // movw            $1008, %ax
        // call            lba2chs
        // cmpw            $1, %cx
        // jne             _test_lba2chs_fail
        // cmpb            $0, %dh
        // jne             _test_lba2chs_fail
        // cmpb            $1, %dl
        // jne             _test_lba2chs_fail

        // # 32256 = 32,0,1
        // movw            $32256, %ax
        // call            lba2chs
        // cmpw            $32, %cx
        // jne             _test_lba2chs_fail
        // cmpb            $0, %dh
        // jne             _test_lba2chs_fail
        // cmpb            $1, %dl
        // jne             _test_lba2chs_fail

_test_lba2chs_pass:
        leaw            s_lba2chs_pass, %si
        call            bios_print
        jmp             readkey_reset

_test_lba2chs_fail:
        leaw            s_lba2chs_fail, %si
        call            bios_print
        jmp             readkey_reset

//         # hardcoded sector for now
//         movw            $STAGE2_BASE, %bx       # dest
//         movb            $1, %ax                 # sector
//         movw            $64, %cx                # sector count

// read_sectors:
//         movb            drive_number, %dl
//         movb            $0x02, %ah
//         int             $0x13
//         jc              _boot_diskerror

//         jmp             stage2                  # onward!

// _boot_diskerror:
//         leaw            s_diskerr, %si
//         call            bios_print
//         jmp             readkey_reset

readkey_reset:
        call            bios_readkey
reset:
        movw            $RESETMODE_NOMEMTEST, RESETMODE
        ljmp            $0xFFFF, $0x0000

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# lba2chs
# -----------------------------------------------------------------------------
# Converts a Logical Block Address into a Cylinder-Head-Sector address.
#
# Returns:
#      cx - cylinder
#      dh - head
#      dl - sector
# Clobbers:
#      ax, bx, cx, dx
#
lba2chs:
        # C = LBA / (HPC * SPT)
        movw            %ax, %cx                # save lba for later
        movw            sectors_per_track, %bx  # bx = spt
        imulw           head_count, %bx         # bx = hpc * spt
        xorw            %dx, %dx                # clear dx
        idivw           %bx                     # ax = dx:ax / bx
        pushw           %ax                     # store cylinder for later

        # H = (LBA / SPT) mod HPC
        # S = (LBA mod SPT) + 1
        xorw            %dx, %dx                # clear dx
        movw            %cx, %ax                # ax = lba
        idivw           sectors_per_track       # ax = dx:ax / spt, dx = dx:ax mod spt
        incw            %dx                     # +1
        movb            %dl, %cl                # store head
        xorw            %dx, %dx                # clear dx
        idivw           head_count              # dx = dx:ax mod hpc (ax = lba / spt)
        movb            %dl, %dh                # dh = head
        movb            %cl, %dl                # dl = sector
        popw            %cx                     # cx = cylinder
        ret

# -----------------------------------------------------------------------------
# bios_print
# -----------------------------------------------------------------------------
# Prints a zero-terminated string at the current curusor location and
# advances the cursor.
#
# Inputs:
#      ds:si - address of string to print
# Clobbers:
#      (potentially everything due to BIOS call)
#
.globl bios_print
bios_print:
        xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              _bios_print_done
        int             $0x10
        jmp             bios_print
_bios_print_done:
        ret

.globl bios_println
bios_println:
        call            bios_print
        leaw            s_newline, %si
        call            bios_print
        ret

# -----------------------------------------------------------------------------
# bios_readkey
# -----------------------------------------------------------------------------
# Waits for keyboard input.
#
# Returns:
#      al - key pressed
# Clobbers:
#      (potentially everything due to BIOS call)
#
.globl bios_readkey
bios_readkey:
        xorb            %ah, %ah
        int             $0x16
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

s_lba2chs_fail:
        .asciz          "\r\nLBA2CHS test failed!"
s_lba2chs_pass:
        .asciz          "\r\nLBA2CHS test passed!"

s_diskerr:
        .ascii          "\r\nDiskette read error!"
        .asciz          "\r\nPress any key to try again..."
s_newline:
        .asciz          "\r\n"
s_dot:
        .asciz          "."

        .space          0x1FE - (. - _start), 0xCC     # padding
        .short          0xAA55                          # boot signature
