# =============================================================================
# Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
#         File: src/i386/boot/stage1.S
#      Created: March 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode
#
# Stage 1 boot loader. This code lives in the first addressable sector of the
# boot disk, also known as the boot sector. It's sole purpose is to load stage
# 2 of the boot loader (which collects system information before loading the
# kernel image). Space is important here, as we only have 512 bytes to work
# with, 64 of which are reserved by the FAT12 File System, leaving only 448
# bytes for our boot code/data. Any labels defined globally here will be
# accessible by the 16-bit Real Mode portion of stage 2.
#
# Designed for a 1.44MB floppy with 512-byte sectors formatted with FAT12.
# =============================================================================

#include "x86_boot.h"

.code16

# -----------------------------------------------------------------------------
# --------------------------------- Stage 1 -----------------------------------
# -----------------------------------------------------------------------------
.section .stage1, "ax", @progbits

##
# Boot loader entry point. This is where the BIOS drops us off.
#
# DO NOT MOVE, RENAME, OR MODIFY THIS FUNCTION IN ANY WAY.
# This exact function is required by the FAT specification in order to keep the
# FAT parameters in a known location. We are given exactly three bytes to jump
# to the next section of code which follows the parameter table.
##
.globl entry
entry:
        jmp             _startup
        nop

##
# BIOS Parameter Block for a 3.5" 1.44MB floppy disk formatted with FAT12.
# DO NOT MOVE THIS. Tweak with care.
##
.globl bios_param_block
bios_param_block:
  oem_name:             .ascii          "WHAM!SON"
  sector_size:          .short          512
  sectors_per_cluster:  .byte           1
  reserved_sectors:     .short          1
  fat_count:            .byte           2
  root_dir_capacity:    .short          224
  sector_count:         .short          2880
  media_type:           .byte           0xF0
  sectors_per_fat:      .short          9
  sectors_per_track:    .short          18
  head_count:           .short          2
  hidden_sectors:       .int            0
  sector_count32:       .int            0
  drive_number:         .byte           0
  _reserved:            .byte           0
  extended_boot_sig:    .byte           0x29
  volume_id:            .int            0xCA55E77E
  volume_label:         .ascii          "OH-WES     "
  fs_type:              .ascii          "FAT12   "

##
# Initial boot loader code (stage 1).
# Gotta be tight; file must assemble to exactly 512 bytes in order to fit in the
# boot sector!
#
# We start off executing at 0x7C00 in 16-bit Real Mode. We disable interrupts,
# set up our segment registers, then locate BOOT.SYS (stage 2) on disk. We load
# stage 2 immediately after stage1 at 0x7E00. It should be small enough to not
# bump into the kernel at 0x10000. :-)
##
_startup:
        ljmp            $0, $stage1             # set CS to a known value

stage1:
        # setup segment registers and stack
        xorw            %ax, %ax
        movw            %ax, %ss
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %fs
        LOAD_SEGMENT    $BDA_SEGMENT, %gs
        movw            $STACK_BASE, %bp
        movw            %bp, %sp

        # load the root directory
        movw            root_dir_capacity, %cx  # calculate root dir size:
        shlw            $DIRENTRY_SHIFT, %cx    #  capacity*sizeof(dir_entry)
        movw            sectors_per_fat, %ax    # calculate start sector:
        mulb            fat_count               #  nreserved+(nfatsectors*nfats)
        addw            reserved_sectors, %ax
        movw            %ax, first_cluster
        decw            %cx                     # caclulate num sectors to read:
        shrw            $SECTOR_SHIFT, %cx      #  nsectors=((nbytes-1)/512)+1
        incw            %cx
        addw            %cx, first_cluster      # 1st cluster follows root dir
        movw            $ROOTDIR_BASE, %bx
        call            load_sectors            # read root dir

        # load stage 2 binary
        leaw            sz_stage2_filename, %si
        call            find_file               # returns: di = dir entry ptr
        movw            $STAGE2_BASE, %bx
        call            load_file               # load stage 2!

        # continue...
        jmp             stage2                  # jump to stage 2!

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# find_file
# -----------------------------------------------------------------------------
# Given a file name, locates the directory entry of the file in the root
# directory. If the file cannot be found, the system will halt in an error
# state.
#
# WARNINGS:
#     - ASSUMES ROOT DIRECTORY IS ALREADY LOADED AT A KNOWN ADDRESS!
#     - ASSUMES FILE IS NOT FRAGMENTED! In order to preserve space, this
#       function does not read the cluster map, so a fragmented file cannot be
#       loaded.
#
# Inputs:
#       ds:si - file name ptr, e.g. "FILE    TXT"
#
# Outputs:
#       ds:di - dir entry ptr
#
# Clobbers:
#       ax, bx, cx, dx
##
.globl find_file
find_file:
        pushw           %bp
        movw            %sp, %bp

        pushw           %es                     # bp-2: old es
        movw            %ds, %ax
        movw            %ax, %es                # ensure es == ds (for cmpsb)

        movw            $ROOTDIR_BASE, %di      # start at top of root dir
        movw            root_dir_capacity, %bx  # loop count
        cld                                     # clear direction flag

_find_file_loop:
        pushw           %si                     # bp-4: file name to look for
        pushw           %di                     # bp-6: dir ptr, ptr[0]=filename
        movw            $FILENAME_LENGTH, %cx
   repe cmpsb                                   # match?
        jz              _find_file_done         #  yep! we're done
        popw            %di                     # otherwise:
        popw            %si                     #  pop vars
        addw            $DIRENTRY_SIZE, %di     #  advance ptr
        decw            %bx                     #  decrement loop counter
        jz              disk_error              # zero? file not found!
        jmp             _find_file_loop         # otherwise: keep searchin'

_find_file_done:
        movw            -6(%bp), %di            # output dir entry ptr
        movw            -2(%bp), %ax            # restore old es
        movw            %ax, %es

        leave
        ret
##
# -----------------------------------------------------------------------------
# load_file
# -----------------------------------------------------------------------------
# Loads a file from disk given a directory entry.
#
# WARNINGS:
#     - ASSUMES FILE IS NOT FRAGMENTED! In order to preserve space, this
#       function does not read the cluster map, so a fragmented file cannot be
#       loaded.
#     - ASSUMES CLUSTER SIZE = 1 SECTOR.
#
# Inputs:
#       es:bx - destination buffer
#       ds:di - dir entry ptr
#
# Outputs:
#       [es:bx] - file data written to buffer
#
# Clobbers:
#       ax, bx, ecx, dx
##
.globl load_file
load_file:
        pushw           %bp
        movw            %sp, %bp

        movw            CLUSTER(%di), %ax       # first cluster for file
        subw            $CLUSTER_NUM_OFFSET, %ax
        movl            FILESIZE(%di), %ecx     # file size in bytes

        decl            %ecx                    # calculate num clusters to read
        shrl            $SECTOR_SHIFT, %ecx     #  nsectors=((nbytes-1)/512)+1
        incw            %cx
        // xchgw           %ax, %cx                # convert nclusters to nsectors
        // mulb            sectors_per_cluster     #  nclusters=(nsectors*spt)
        // xchgw           %ax, %cx
        addw            first_cluster, %ax      # calculate abs. sector address

        call            load_sectors            # read it in!

        leave
        ret

##
# -----------------------------------------------------------------------------
# load_sectors
# -----------------------------------------------------------------------------
# Reads one or more sectors from the boot disk into a buffer.
#
# Inputs:
#       es:bx - buffer address
#          ax - logical block address of starting sector
#          cx - sector count
#
# Outputs:
#       [es:bx] - sector data written to buffer
#
# Clobbers:
#       ax, bx, cx, dx
##
.globl load_sectors
load_sectors:
        pushw           %bp
        movw            %sp, %bp

        pushw           %bx                     # bp-2: buffer
        pushw           %ax                     # bp-4: lba
        pushw           %cx                     # bp-6: count
        pushw           $RETRY_COUNT            # bp-8: nretries

_load_sectors_loop:
        decw            -8(%bp)                 # --nretries
        movw            -4(%bp), %ax            # ax = lba
        call            lba2chs                 # ret: cx=cyl, dh=head, dl=sect

        # combine cylinder and sector into cx like below:
        #   cx =       ---ch--- ---cl---
        #   cylinder : 76543210 98
        #   sector   :            543210
        xchgb           %cl, %ch
        rorb            $2, %cl
        orb             %dl, %cl

        movw            -2(%bp), %bx            # es:bx=buffer
        movb            drive_number, %dl       # dl=drive
        movb            $1, %al                 # al=count
        movb            $0x02, %ah              # ah=fn (load_sectors)
        int             $0x13                   # cx=cyl/sect, dh=head
        jnc             _load_sectors_check_wrap# error if carry flag set
        cmpw            $0, -8(%bp)
        jz              disk_error              # fail if no retries left
        jmp             _load_sectors_loop      # otherwise try again!

_load_sectors_check_wrap:
        PRINT           sz_progress_char
        movw            -2(%bp), %bx            # save old buf
        movw            sector_size, %ax        # buf += sector_size
        addw            %ax, -2(%bp)
        cmpw            %bx, -2(%bp)            # if CF set, we wrapped
        jnc             _load_sectors_loop_tail # otherwise continue on!

_load_sectors_inc_segreg:
        movw            %es, %ax
        addw            $SEGMENT_SIZE>>4, %ax
        movw            %ax, %es

_load_sectors_loop_tail:
        movw            $RETRY_COUNT, -8(%bp)   # reset nretries
        incw            -4(%bp)                 # ++lba
        decw            -6(%bp)                 # --count
        jz              _load_sectors_done      # done if count == 0
        jmp             _load_sectors_loop

_load_sectors_done:
        leave
        ret

##
# -----------------------------------------------------------------------------
# lba2chs
# -----------------------------------------------------------------------------
# Converts a Logical Block Address into a Cylinder-Head-Sector address.
#
# Inputs:
#       ax - logical block address
#
# Outputs:
#       cx - cylinder
#       dh - head
#       dl - sector
#
# Clobbers:
#       ax, bx, cx, dx
##
.globl lba2chs
lba2chs:
        # TEST DATA -- disk with 2 heads, 80 tracks, and 18 sectors per track:
        #  LBA = C,H,S
        #   0  = 0,0,1
        #   1  = 0,0,2
        #   17 = 0,0,18
        #   18 = 0,1,1
        #   35 = 0,1,18
        #   36 = 1,0,1
        #   71 = 1,1,18
        #   72 = 2,0,1
        # 2879 = 79,1,18
        # 2880 = 80,0,1

        # C = LBA / (HPC * SPT)
        movw            %ax, %cx                # save lba for later
        movw            sectors_per_track, %bx  # bx = spt
        imulw           head_count, %bx         # bx = hpc*spt
        xorw            %dx, %dx                # clear dx
        idivw           %bx                     # ax = dx:ax/bx
        pushw           %ax                     # save cylinder for later

        # H = (LBA / SPT) mod HPC
        # S = (LBA mod SPT) + 1
        xorw            %dx, %dx                # clear dx
        movw            %cx, %ax                # ax = lba
        idivw           sectors_per_track       # ax = lba/spt, dx=lba%spt
        incw            %dx                     # +1
        movb            %dl, %cl                # save head for later
        xorw            %dx, %dx                # clear dx
        idivw           head_count              # dx = (lba/spt)%hpc
        movb            %dl, %dh                # dh = head
        movb            %cl, %dl                # dl = sector
        popw            %cx                     # cx = cylinder

        ret

##
# -----------------------------------------------------------------------------
# print
# -----------------------------------------------------------------------------
# Prints a zero-terminated string at the current curusor location and
# advances the cursor.
#
# Inputs:
#       ds:si - address of string to print
#
# Clobbers:
#       ax, bx, si
##
.globl print
print:
        xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
#if E9_HACK
        outb            %al, $0xE9
#endif
        andb            %al, %al
        jz              _print_done
        int             $0x10
        jmp             print

_print_done:
        ret

##
# -----------------------------------------------------------------------------
# readkey
# -----------------------------------------------------------------------------
# Waits for keyboard input.
#
# Returns:
#       al - key pressed
#
# Clobbers:
#       ah
##
.globl readkey
readkey:
_readkey_flush:
        movb            $0x01, %ah
        int             $0x16
        jz              _readkey_read
        xorb            %ah, %ah
        int             $0x16
        jmp             _readkey_flush
_readkey_read:
        xorb            %ah, %ah
        int             $0x16
        ret

##
# -----------------------------------------------------------------------------
# readkey_then_reset
# -----------------------------------------------------------------------------
# Waits for keyboard input, then resets the machine.
##
.globl readkey_then_reset
readkey_then_reset:
        PRINT           sz_try_again
        call            readkey
        jmp             reset

##
# -----------------------------------------------------------------------------
# reset
# -----------------------------------------------------------------------------
# Performs a soft reset by jumping to the CPU reset vector.
##
.globl reset
reset:
        movw            $WARMBOOT, %gs:BDA_RESETFLAG
        ljmp            $0xFFFF, $0x0000

##
# -----------------------------------------------------------------------------
# disk_error
# -----------------------------------------------------------------------------
# Prints a disk error message, waits for a key press, then resets the system.
##
.globl disk_error
disk_error:
        PRINT           sz_disk_error
        jmp             readkey_then_reset

# -----------------------------------------------------------------------------
# ------------------------------ Padding & Data -------------------------------
# -----------------------------------------------------------------------------

first_cluster:
        .word           0
sz_progress_char:
        .asciz          "."
sz_stage2_filename:
        .asciz          "BOOT    SYS"
sz_disk_error:
        .asciz          "\r\nDiskette read error!"
sz_try_again:
        .asciz          "\r\nPress any key to reboot..."

.space                  0x1FE - (. - entry), 0xCC       # pad with debugbreak
.short                  0xAA55                          # boot sector indicator
