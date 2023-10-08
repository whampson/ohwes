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
#      Created: March 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode
#
# Stage 1 boot loader. It's sole purpose is to load the Stage 2 boot loader.
# Designed for a 1.44MB floppy with 512-byte sectors and a FAT12 file system.
# =============================================================================

#include "boot.h"

.text
.code16

# -----------------------------------------------------------------------------
# --------------------------------- Stage 1 -----------------------------------
# -----------------------------------------------------------------------------

##
# Boot loader entry point. This is where the BIOS drops us off.
#
# DO NOT MOVE, RENAME, OR MODIFY THIS FUNCTION IN ANY WAY.
# This exact function is required by the FAT specification in order to keep the
# FAT parameters in a known location. We are given exactly three bytes to jump
# to the next section of code which follows the parameter table.
##
.globl Entry
Entry:
        jmp     Stage1
        nop

##
# BIOS Parameter Block for a FAT12-formatted 3.5" 1.44MB floppy disk.
# DO NOT MOVE THIS. Tweak with care.
##
OemName:                .ascii          "whampson"
SectorSize:             .short          512
SectorsPerCluster:      .byte           1
ReservedSectors:        .short          1
FatCount:               .byte           2
RootDirCapacity:        .short          224
SectorCount:            .short          2880
MediaType:              .byte           0xF0
SectorsPerFat:          .short          9
SectorsPerTrack:        .short          18
HeadCount:              .short          2
HiddenSectors:          .int            0
SectorCount32:          .int            0
DriveNumber:            .byte           0
_Reserved:              .byte           0
Signature:              .byte           0x29
VolumeId:               .int            0xCA55E77E
Label:                  .ascii          "OH-WES     "
FsType:                 .ascii          "FAT12   "

##
# Initial boot loader code.
# Gotta be tight; file must assemble to no more than 512 bytes!
#
# We start off executing at 0x7C00. We set the stack pointer to this address
# (remember, the stack grows towards 0!) and load BOOT.SYS at 0x7E00,
# immediately after the boot sector in memory. We then jump to the stage 2 code.
##
Stage1:
        cld                             # clear direction bit
        xorw    %ax, %ax                # set up segment regs and stack
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    $BIOS_DATA_SEG, %ax
        movw    %ax, %gs
        movw    $STACK_SEG, %ax
        movw    %ax, %ss
        movw    $0x0000, %bp
        movw    %bp, %sp
        PRINT   s_Hello1                # print a welcome message

LoadRootDir:
        movw    RootDirCapacity, %cx    # calculate root dir size
        shlw    $5, %cx                 #   maxentries*32
        movw    SectorsPerFat, %ax      # calculate root dir start sector
        mulb    FatCount                #   nreserved+(nfatsectors*nfats)
        addw    ReservedSectors, %ax
        movw    %ax, g_FirstCluster     # store the sector index for later
        decw    %cx                     # caclulate num sectors to read
        shrw    $9, %cx                 #   ((nbytes-1)/512)+1
        incw    %cx                     # first data cluster immediately follows
        addw    %cx, g_FirstCluster     #   root dir, add nsectors here
        movw    $ROOTDIR_BASE, %bx      # root dir base addr
        call    LoadSectors             # read root dir

LoadStage2:
        leaw    s_Stage2FileName, %si
        movw    $STAGE2_BASE, %bx       # stage2 addr
        call    LoadFile

GoToStage2:
        jmp     Stage2                  # go ahead and jump (jump!)

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

# edx = dir entry ptr
# es:bx = dest addr
# TODO: take a file name and search root dir for dir entry

##
# -----------------------------------------------------------------------------
# LoadFile
# -----------------------------------------------------------------------------
# Loads a file from the root directory on disk given a file name.
#
# NOTES:
#     - File must be contiguous! In order to preserve space, this function does
#       not read the cluster map, so a fragmented file cannot be loaded.
#     - File cannot exceed 64k in size or be large enough to cross a segment
#       boundary.
#
# Inputs:
#       es:bx - destination buffer
#       ds:si - file name ptr, e.g. "FILE    TXT"
#
# Clobbers:
#       (potentially everything due to BIOS call)
##
.globl LoadFile
LoadFile:
        // TODO: search for file name
        movw    26(%edx), %ax           # first cluster for file
        subw    $2, %ax                 # cluster indices 0 and 1 reserved
        movw    28(%edx), %cx           # file size in bytes

        decw    %cx                     # calculate num clusters to read
        shrw    $9, %cx                 #  ((nbytes-1)/512)+1
        incw    %cx
        xchgw   %ax, %cx                # convert nclusters to nsectors
        mulb    SectorsPerCluster
        xchgw   %ax, %cx
        addw    g_FirstCluster, %cx     # calculate absolute sector address

        call    LoadSectors             # read it in!

        ret

##
# -----------------------------------------------------------------------------
# LoadSectors
# -----------------------------------------------------------------------------
# Reads one or more sectors from the primary disk drive into a buffer.
# In this case, the primary disk drive is floppy drive 0.
#
# Inputs:
#       es:bx - buffer address
#          ax - logical block address of starting sector
#          cx - sector count
#
# Clobbers:
#       (potentially everything due to BIOS call)
##
.globl LoadSectors
LoadSectors:
        test            %cx, %cx
        jle             _LoadSectors_Error

        pushw           %bp
        movw            %sp, %bp

        pushw           %bx                     # bp-2: buffer
        pushw           %ax                     # bp-4: lba
        pushw           %cx                     # bp-6: count

_LoadSectors_Loop:
        movw            -4(%bp), %ax            # ax = logical block address
        call            LbaToChs                # returns: cx=cylinder, dh=head, dl=sector

        # combine cylinder and sector into cx like below:
        #   cx =       ---ch--- ---cl---
        #   cylinder : 76543210 98
        #   sector   :            543210
        xchgb           %cl, %ch
        rorb            $2, %cl
        orb             %dl, %cl

        movw            -2(%bp), %bx            # es:bx=buffer
        movb            DriveNumber, %dl        # dl=drive
        movb            $1, %al                 # al=count
        movb            $0x02, %ah              # ah=fn:LoadSectors
        int             $0x13                   # ch=cyl, dh=head, cl=sect
        jc              _LoadSectors_Error
        # TODO: retry read on error?

        PRINT           s_ProgressChar

        movw            SectorSize, %ax
        addw            %ax, -2(%bp)            # buf += SectorSize
        # TODO: detect wraparound!
        incw            -4(%bp)                 # lba++
        decw            -6(%bp)                 # count--

        jz              _LoadSectors_Done
        jmp             _LoadSectors_Loop

_LoadSectors_Done:
        leave
        ret

_LoadSectors_Error:
        PRINT           s_DiskError
        jmp             ReadKeyThenReset

##
# -----------------------------------------------------------------------------
# LbaToChs
# -----------------------------------------------------------------------------
# Converts a Logical Block Address into a Cylinder-Head-Sector address.
#
# Inputs:
#       ax - logical block address
# Returns:
#       cx - cylinder
#       dh - head
#       dl - sector
# Clobbers:
#       ax, bx, cx, dx
##
.globl LbaToChs
LbaToChs:
        # For a disk with 2 heads, 80 tracks, and 18 sectors per track:
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
        movw            SectorsPerTrack, %bx    # bx = spt
        imulw           HeadCount, %bx          # bx = hpc * spt
        xorw            %dx, %dx                # clear dx
        idivw           %bx                     # ax = dx:ax / bx
        pushw           %ax                     # save cylinder for later

        # H = (LBA / SPT) mod HPC
        # S = (LBA mod SPT) + 1
        xorw            %dx, %dx                # clear dx
        movw            %cx, %ax                # ax = lba
        idivw           SectorsPerTrack         # ax = dx:ax / spt, dx = dx:ax mod spt
        incw            %dx                     # +1
        movb            %dl, %cl                # save head for later
        xorw            %dx, %dx                # clear dx
        idivw           HeadCount               # dx = dx:ax mod hpc (ax = lba / spt)
        movb            %dl, %dh                # dh = head
        movb            %cl, %dl                # dl = sector
        popw            %cx                     # cx = cylinder
        ret

##
# -----------------------------------------------------------------------------
# PrintStr
# -----------------------------------------------------------------------------
# Prints a zero-terminated string at the current curusor location and
# advances the cursor.
#
# Inputs:
#       ds:si - address of string to print
# Clobbers:
#       (potentially everything due to BIOS call)
##
.globl PrintStr
PrintStr:
        xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              _PrintStr_done
        int             $0x10
        jmp             PrintStr
_PrintStr_done:
        ret

##
# -----------------------------------------------------------------------------
# ReadKey
# -----------------------------------------------------------------------------
# Waits for keyboard input.
#
# Returns:
#       al - key pressed
# Clobbers:
#       (potentially everything due to BIOS call)
##
.globl ReadKey
ReadKey:
        xorb            %ah, %ah
        int             $0x16
        ret

##
# -----------------------------------------------------------------------------
# ReadKeyThenReset
# -----------------------------------------------------------------------------
# Waits for keyboard input, then resets the machine.
##
.globl ReadKeyThenReset
ReadKeyThenReset:
        call            ReadKey
        jmp             Reset

##
# -----------------------------------------------------------------------------
# Reset
# -----------------------------------------------------------------------------
# Performs a warm boot by jumping to the CPU reset vector.
##
.globl Reset
Reset:
        movw            $RESETFLAG_WARM, %gs:BIOS_RESETFLAG
        ljmp            $0xFFFF, $0x0000

##
# -----------------------------------------------------------------------------
# Halt
# -----------------------------------------------------------------------------
# Puts the system into an idle loop.
##
.globl Halt
Halt:
        hlt
        jmp             Halt

# -----------------------------------------------------------------------------
# ------------------------------ Padding & Data -------------------------------
# -----------------------------------------------------------------------------

.globl g_FirstCluster
g_FirstCluster:
        .word   0

.globl s_NewLine
s_NewLine:
        .asciz  "\r\n"

.globl s_NewLine
s_ProgressChar:
        .asciz  "."

.globl s_Stage2FileName
s_Stage2FileName:
        .asciz  "BOOT    SYS"

.globl s_KernelFileName
s_KernelFileName:
        .asciz  "OHWES   SYS"

.globl s_Hello1
s_Hello1:
        .asciz  "\r\nLoading System"

.globl s_NewLine
s_DiskError:
        .ascii  "\r\nDiskette read error!"
        .asciz  "\r\nPress any key to try again..."

# pad the remaining space up to the size of a sector (512 bytes)
.space          0x1FE - (. - Entry), 0xCC       # pad with debugbreak opcode
.short          0xAA55                          # boot sector indicator
