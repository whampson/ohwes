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
#         File: i386/boot/stage1.S
#      Created: March 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode
#
# Stage 1 of the boot loader. This code lives in the first addressable sector of
# the boot disk, known as the "boot sector", and is automatically loaded at a
# known address by the BIOS. It's sole purpose is to load stage 2 of the boot
# loader, which continues the boot process. Code size is very important here, as
# we only have 512 bytes to work with, 64 of which are reserved by the FAT12
# Boot Parameter Block, leaving only 448 bytes for our boot code and data. Any
# labels declared .globl here will be accessible by the 16-bit Real Mode portion
# of stage 2, including any useful subroutines.
#
# This boot loader is designed for use on a high-density (1.44MB) floppy disk
# formatted with FAT12 using 512-byte sectors. It should also work with a USB
# drive on modern systems. See CONFIG_FLOPPY_BOOT in _config.h.
#
# BOOT LOADER LIMITATIONS:
#   * Disk MUST be formatted FAT12 with a sector size of 512 bytes.
#   * FRAGMENTATION NOT ALLOWED! Files loaded by the boot loader MUST be
#     contiguous on disk because the boot loader is incapable of reading the
#     FAT cluster chain at this time.
# =============================================================================

#include "bootldr.h"

# Subroutines callable from stage 2
.globl find_file
.globl load_file
.globl load_sectors
.globl print
.globl readkey
.globl readkey_then_reset
.globl reset

# =============================================================================
# --------------------------------- Stage 1 -----------------------------------
# =============================================================================
.section .stage1, "ax", @progbits
.code16

RETRY_COUNT  = 3    # per-sector floppy read attempts

# -----------------------------------------------------------------------------

##
# Boot loader stage 1 entry point. This is where the BIOS drops us off.
#
# *** DO NOT MOVE, RENAME, OR MODIFY THESE INSTRUCTIONS IN ANY WAY ***
#
# These exact instructions are required by the FAT specification in order to
# keep the FAT parameters in a known location (see below). We are given exactly
# three bytes to jump to the next section of code, which follows the parameter
# block.
##
entry:
        jmp         _load_cs
        nop

##
# FAT-12 BIOS Parameter Block
#
# ***DO NOT MOVE THIS***
#
# Currently configured with standard values for a 3.5" high-density floppy
# (1.44 MB). Hopefully your formatting tool agrees (or just use mine).
#
# Using Hungarian notation here because the type info is actually useful.
# Tweak with care.
##
                    .ascii      "WHAM!SON"          # OEM name (8 chars)
wSectorSize:        .short      (1<<SECTOR_SHIFT)   # sector size in bytes
                    .byte       1                   # sectors per cluster
wReservedSectors:   .short      1                   # reserved sectors
bNumFats:           .byte       2                   # number of FATs
wNumDirEntries:     .short      224                 # root dir capacity
                    .short      2880                # total sector count
                    .byte       DISK_MEDIA_TYPE     # media type magic
wSectorsPerFat:     .short      9                   # FAT size in sectors
wSectorsPerTrack:   .short      DISK_SPT            # track size in sectors
wHeadCount:         .short      DISK_HEAD_COUNT     # number of heads on disk
                    .long       0                   # hidden sector count
                    .long       0                   # sector count (32-bit vers)
bDriveNumber:       .byte       DISK_DRIVE_NUMBER   # boot drive number
                    .byte       0                   # (reserved)
bExtendedBootSig:   .byte       0x29                # ext. boot sig
                    .long       0xCA55E77E          # volume number
                    .ascii      "OH-WES     "       # volume label (11 chars)
                    .ascii      "FAT12   "          # file system type (8 chars)

##
# The /Real/ stage 1 entry point. Gotta be tight! This file must assemble to
# exactly 512 bytes in order to fit in the boot sector!
#
# We start off executing at 0x7C00 in 16-bit Real Mode. We set up our segment
# registers, locate BOOT.SYS (stage 2) on disk, then load stage 2 in memory
# immediately after stage 1 at 0x7E00. It should be small enough to not bump
# into the kernel, which is eventually loaded above stage 2.
##
_load_cs:
        cli
        ljmp        $0, $_continue
_continue:                                  # setup segment registers and stack
        xorw        %ax, %ax
        movw        %ax, %ds
        movw        %ax, %es
        movw        %ax, %ss
        movw        $STACK_BASE, %sp
        movw        %sp, %bp
        PRINT       szLoadMsg

        # load the root directory
        movw        wNumDirEntries, %cx     # calculate root dir size:
        shlw        $FAT_DIRENTRY_SHIFT,%cx #  capacity*sizeof(DirEntry)
        movw        wSectorsPerFat, %ax     # calculate start sector:
        mulb        bNumFats                #  nreserved+(nfatsectors*nfats)
        addw        wReservedSectors, %ax
        movw        %ax, wFirstDataSector   # store 1st cluster sector num
        decw        %cx                     # calculate num sectors to read:
        shrw        $SECTOR_SHIFT, %cx      #  nsectors=((nbytes-1)/512)+1
        incw        %cx
        addw        %cx, wFirstDataSector   # 1st cluster follows root dir
        movw        $FAT_ROOTDIR_BASE, %bx
        call        load_sectors            # read root dir

        # load and execute stage 2
        leaw        sStage2FileName, %si
        call        find_file               # returns: di = dir entry ptr
        pushl       DIRENTRY_FILESIZE(%di)  # store stage2 size on the stack
        movw        $STAGE2_BASE, %bx       # bx = load addr
        call        load_file               # load it!
        jmp         stage2                  # jump to stage 2!

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

# FAT12 Filesystem Constants
FAT_FILENAME_LENGTH = (8+3)
FAT_DIRENTRY_SHIFT  = 5  # 1<<5 = 32
FAT_DIRENTRY_SIZE   = (1 << FAT_DIRENTRY_SHIFT)
FAT_CLUSTER_OFFSET  = 2  # clusters 0 and 1 are reserved

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
#       ds:si - ptr to 8+3 file name, e.g. "FILE    TXT"
#
# Outputs:
#       ds:di - found dir entry ptr
#
# Clobbers:
#       ax, bx, cx, dx
##
find_file:
        cld
        pushw       %es                     # [bp-2]: old es
        movw        %ds, %ax                # es = ds (for cmpsb)
        movw        %ax, %es
        movw        $FAT_ROOTDIR_BASE, %di  # di = dir ptr
        movw        wNumDirEntries, %bx     # loop count
_find_file_loop:
        pushw       %si                     # [bp-4]: file name to look for
        pushw       %di                     # [bp-6]: dir ptr, ptr[0]=filename
        movw        $FAT_FILENAME_LENGTH, %cx
   repe cmpsb                               # match?
        jz          _find_file_done         #  yep! we're done
        popw        %di                     # otherwise:
        popw        %si                     #  restore vars
        addw        $FAT_DIRENTRY_SIZE, %di #  advance ptr
        decw        %bx                     #  decrement loop counter
        jz          disk_error              # zero? file not found!
        jmp         _find_file_loop         # otherwise: keep searchin'
_find_file_done:
        popw        %di                     # output dir entry ptr
        popw        %si                     # restore si
        popw        %es                     # restore es
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
#       es:bx - destination buffer address
#       ds:di - directory entry ptr
#
# Outputs:
#       [es:bx] - file data written to buffer (pre-clobber)
#
# Clobbers:
#       ax, bx, ecx, edx, es
##
load_file:
        movw        DIRENTRY_CLUSTER(%di), %ax      # ax = 1st cluster
        subw        $FAT_CLUSTER_OFFSET, %ax        # apply offset
        movl        DIRENTRY_FILESIZE(%di), %ecx    # file size in bytes
        decl        %ecx                            # num clusters to read:
        shrl        $SECTOR_SHIFT, %ecx             #  nsect=((nbytes-1)/512)+1
        incl        %ecx
        addw        wFirstDataSector, %ax           # abs. sector address
        call        load_sectors                    # read it in!
        ret

##
# -----------------------------------------------------------------------------
# load_sectors
# -----------------------------------------------------------------------------
# Reads one or more sectors from the boot disk into a buffer.
#
# Inputs:
#       es:bx - destination buffer address
#          ax - logical block address of starting sector
#          cx - sector count
#
# Outputs:
#       [es:bx] - sector data written to buffer (pre-clobber)
#
# Clobbers:
#       ax, bx, cx, dx, es
##
load_sectors:
        # setup stack frame
        pushw       %bp
        movw        %sp, %bp
        pushw       %bx                     # [bp-2] = buffer
        pushw       %ax                     # [bp-4] = lba
        pushw       %cx                     # [bp-6] = count
        pushw       $RETRY_COUNT            # [bp-8] = nretries

_load_sectors_loop:
        # convert LBA to CHS
        movw        -4(%bp), %ax            # ax = lba
        call        lba2chs                 # ret: cx=cyl, dh=head, dl=sect

        # combine cylinder and sector into cx like below:
        #   cx =       ---ch--- ---cl---
        #   cylinder : 76543210 98
        #   sector   :            543210
        xchgb       %cl, %ch
        rorb        $2, %cl
        orb         %dl, %cl

        # call BIOS to load sector
        decw        -8(%bp)                 # --nretries
        movw        -2(%bp), %bx            # es:bx=buffer
        movb        bDriveNumber, %dl       # dl=drive
        movb        $1, %al                 # al=count
        movb        $0x02, %ah              # ah=load_sectors()
        int         $0x13                   # ret: cx=cyl/sect, dh=head
        jc          _load_sectors_error     # error if carry flag set
        PRINT       szProgressChar

        # advance buffer and make sure segment address didn't wrap
        movw        -2(%bp), %bx            # bx = buf
        movw        wSectorSize, %ax        # ax = sector_size
        addw        %ax, -2(%bp)            # buf += sector_size
        cmpw        %bx, -2(%bp)            # if buf < old_buf: we wrapped!
        jnc         _load_sectors_loop_next # CF indicates wrap
        movw        %es, %ax                # es += 0x1000
        addw        $(1<<(16-4)), %ax       #   1<<12=0x1000, 0x1000<<4=0x10000
        movw        %ax, %es                # store ES

_load_sectors_loop_next:
        movw        $RETRY_COUNT, -8(%bp)   # reset nretries
        incw        -4(%bp)                 # ++lba
        decw        -6(%bp)                 # --count
        jz          _load_sectors_done      # done if count == 0
        jmp         _load_sectors_loop

_load_sectors_error:
        cmpw        $0, -8(%bp)             # did we run out of retries?
        jz          disk_error              # we did! halt
        jmp         _load_sectors_loop      # otherwise try again

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
        # H = (LBA / SPT) mod HPC
        # S = (LBA mod SPT) + 1

        movw        %ax, %cx                # save LBA for later
        movw        wSectorsPerTrack, %ax   # ax = SPT
        mulw        wHeadCount              # result in dx:ax
        movw        %ax, %bx                # bx = HPC*SPT
        movw        %cx, %ax                # dx:ax = LBA
        xorw        %dx, %dx                # clear dx
        divw        %bx                     # dx:ax = LBA/(HPC*SPT)
        push        %ax                     # save cylinder for later
        xorw        %dx, %dx                # clear dx
        movw        %cx, %ax                # dx:ax = LBA
        divw        wSectorsPerTrack        # dx:ax = LBA/SPT, dx=LBA%SPT
        incw        %dx                     # dx = (LBA%SPT)+1
        movb        %dl, %cl                # save head for later
        xorw        %dx, %dx                # clear dx
        divw        wHeadCount              # dx = (LBA/SPT)%HPC
        movb        %dl, %dh                # dh = head
        movb        %cl, %dl                # dl = sector
        popw        %cx                     # cx = cylinder
        ret

##
# -----------------------------------------------------------------------------
# print
# -----------------------------------------------------------------------------
# Prints a zero-terminated string at the current cursor location and
# advances the cursor.
#
# Inputs:
#       ds:si - address of string to print
#
# Clobbers:
#       ax, bx, si
##
print:
        movb        $0x0E, %ah      # 0Eh = teletype output
        movw        $0x07, %bx      #   param: bl = color, bh = video page
        lodsb                       #   param: al = char, (lodsb: [ds:si++]->al)
        andb        %al, %al        # NUL test
        jz          _print_done
#if CONFIG_E9_HACK
        outb        %al, $0xE9
#endif
        int         $0x10
        jmp         print
_print_done:
        ret

##
# -----------------------------------------------------------------------------
# readkey
# -----------------------------------------------------------------------------
# Waits for keyboard input. Flushes any queued characters before reading.
#
# Returns:
#       al - ASCII code of key pressed
#       ah - scan code of key pressed
##
readkey:
_readkey_flushkb:
        movb        $0x01, %ah      # 01h = read input status
        int         $0x16           #   ret: ZF = no key press
        jz          _readkey_waitkb
        xorb        %ah, %ah        # 00h = read key press
        int         $0x16           #   ret: ah = scan code, al = ASCII char
        jmp         _readkey_flushkb
_readkey_waitkb:
        xorb        %ah, %ah        # now that we've flushed
        int         $0x16           #   block til next keypress
        ret

##
# -----------------------------------------------------------------------------
# readkey_then_reset
# -----------------------------------------------------------------------------
# Prints a message, waits for a key to be pressed, then resets the machine.
##
readkey_then_reset:
        PRINT       szTryAgain
        call        readkey
        jmp         reset

##
# -----------------------------------------------------------------------------
# reset
# -----------------------------------------------------------------------------
# Performs a soft reset by jumping to the CPU reset vector.
##
reset:
        # Values that can be written to BDA_RESETMODE.
        RESETMODE_WARMBOOT = 0x1234     # perform a warm boot (no memory test)
        RESETMODE_KEEPMEM  = 0x4321     # preserve memory
        RESETMODE_SUSPEND  = 0x5678     # suspend instead of reboot

        # System Reset Vector Behavior
        BDA_RESETMODE      = 0x0072     # reset mode address

        # do the reset
        movw        $BDA_SEGMENT, %ax   # write reset mode into BDA
        movw        %ax, %gs
        movw        $RESETMODE_WARMBOOT, %gs:BDA_RESETMODE
        ljmp        $0xFFFF, $0x0000    # long jump to reset vector

##
# -----------------------------------------------------------------------------
# disk_error
# -----------------------------------------------------------------------------
# Prints a disk error message, waits for a key press, then resets the system.
##
disk_error:
        PRINT       szDiskReadError
        jmp         readkey_then_reset

# =============================================================================
# ---------------------------------- Data -------------------------------------
# =============================================================================

wFirstDataSector:   .short      0
sStage2FileName:    .ascii      "BOOT    SYS"
szProgressChar:     .asciz      "."
szLoadMsg:          .asciz      "Loading OH-WES"
szDiskReadError:    .asciz      "\r\nDiskette read error!"
szTryAgain:         .asciz      "\r\nPress any key to restart your computer..."
