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
# Stage 1 boot loader, also known as the boot sector. This code lives in the
# first addressable sector of the boot disk. It's sole purpose is to load stage
# 2 of the boot process (which does more system initialization before loading
# the kernel). Space is important here, as we only have 512 bytes to work with,
# 64 of which are reserved by the FAT12 File System, leaving only 448 bytes for
# our boot code/data. Any labels defined globally here will be accessible by
# the 16-bit Real Mode portion of stage 2.
#
# Designed for a 1.44MB floppy with 512-byte sectors and a FAT12 file system.
# =============================================================================

#include <boot.h>
#include "x86_boot.h"

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
.globl g_Bpb
g_Bpb:
  m_OemName:            .ascii          "WHAM!SON"
  m_SectorSize:         .short          512
  m_SectorsPerCluster:  .byte           1
  m_ReservedSectors:    .short          1
  m_FatCount:           .byte           2
  m_RootDirCapacity:    .short          224
  m_SectorCount:        .short          2880
  m_MediaType:          .byte           0xF0
  m_SectorsPerFat:      .short          9
  m_SectorsPerTrack:    .short          18
  m_HeadCount:          .short          2
  m_HiddenSectors:      .int            0
  m_SectorCount32:      .int            0
  m_DriveNumber:        .byte           0
  _Reserved:            .byte           0
  m_Signature:          .byte           0x29
  m_VolumeId:           .int            0xCA55E77E
  m_Label:              .ascii          "OH-WES     "
  m_FsType:             .ascii          "FAT12   "

##
# Initial boot loader code.
# Gotta be tight; file must assemble to no more than 512 bytes!
#
# We start off executing at 0x7C00 in 16-bit Real Mode. We load BOOT.SYS at
# 0x7E00, immediately after the boot sector in memory, then jump to the stage 2
# code.
##
Stage1:
        cli                             # no interrupts!
        xorw    %ax, %ax                # set up segment regs
        movw    %ax, %ds                # ds = 0
        movw    %ax, %es                # es = 0
        movw    %ax, %fs                # fs = 0
        movw    $BDA_SEGMENT, %ax
        movw    %ax, %gs                # gs = (BIOS Data Area segment)
        movw    $STACK_SEGMENT, %ax
        movw    %ax, %ss                # ss = (Stack Segment)
        movw    $STACK_OFFSET, %bp
        movw    %bp, %sp

LoadRootDir:
        movw    m_RootDirCapacity, %cx  # calculate root dir size
        shlw    $5, %cx                 #   maxentries*32
        movw    m_SectorsPerFat, %ax    # calculate root dir start sector
        mulb    m_FatCount              #   nreserved+(nfatsectors*nfats)
        addw    m_ReservedSectors, %ax
        movw    %ax, g_FirstCluster     # store the sector index for later
        decw    %cx                     # caclulate num sectors to read
        shrw    $9, %cx                 #   ((nbytes-1)/512)+1
        incw    %cx                     # first data cluster immediately follows
        addw    %cx, g_FirstCluster     #   root dir, add nsectors here
        movw    $ROOTDIR_BASE, %bx      # root dir base addr
        call    LoadSectors             # read root dir

LoadStage2:
        leaw    sz_Stage2FileName, %si
        call    FindFile                # returns: di = dir entry ptr
        movl    DIRENTRY_FILESIZE(%di), %ecx
        pushl   %ecx                    # put stage2 size on stack
        movw    $STAGE2_BASE, %bx
        call    LoadFile                # load stage 2!

GoToStage2:
        jmp     Stage2                  # let's go...!

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# FindFile
# -----------------------------------------------------------------------------
# Locates the directory entry of a file in the root directory given a file name.
# If the file cannot be found, the system will enter an error state.
#
# WARNING: ASSUMES FILE IS NOT FRAGMENTED!
#          This routine does not follow the cluster chain by reading the FAT.
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
.globl FindFile
FindFile:
        pushw   %bp
        movw    %sp, %bp

        pushw   %es                     # bp-2: old es
        movw    %ds, %ax
        movw    %ax, %es                # ensure es == ds (for cmpsb)

        movw    $ROOTDIR_BASE, %di      # start at the top of the root dir
        movw    m_RootDirCapacity, %bx  # loop count
        cld                             # clear direction flag

_FindFile_Loop:
        pushw   %si                     # bp-4: file name to be searched for
        pushw   %di                     # bp-6: dir ptr, offset 0 is file name
        movw    $11, %cx                # file name length
   repe cmpsb                           # match?
        jz      _FindFile_Done          #  yep! we're done
        popw    %di                     # otherwise:
        popw    %si                     #   pop vars
        addw    $32, %di                #   advance ptr
        decw    %bx                     #   decrement loop counter
        jz      FileNotFound            # zero? file not found!
        jmp     _FindFile_Loop          # otherwise: keep searchin'

_FindFile_Done:
        movw    -6(%bp), %di            # output the dir entry ptr
        movw    -2(%bp), %ax            # restore old es
        movw    %ax, %es

        leave
        ret
##
# -----------------------------------------------------------------------------
# LoadFile
# -----------------------------------------------------------------------------
# Loads a file from disk given a directory entry.
#
# NOTES:
#     - File must be contiguous! In order to preserve space, this function does
#       not read the cluster map, so a fragmented file cannot be loaded.
#     - Destination buffer cannot cross a segment boundary. This means the
#       maximum loadable file size is 64k.
#
# Inputs:
#       es:bx - destination buffer
#       ds:di - dir entry ptr
#
# Clobbers:
#       ax, bx, ecx, dx
##
.globl LoadFile
LoadFile:
        pushw   %bp
        movw    %sp, %bp

        movw    DIRENTRY_CLUSTER(%di), %ax # first cluster for file
        subw    $2, %ax                 # cluster indices 0 and 1 reserved
        movl    DIRENTRY_FILESIZE(%di), %ecx # file size in bytes

        decw    %cx                     # calculate num clusters to read
        shrw    $9, %cx                 #  ((nbytes-1)/512)+1
        incw    %cx
        xchgw   %ax, %cx                # convert nclusters to nsectors
        mulb    m_SectorsPerCluster     #  nclusters = (nsectors*spt)
        xchgw   %ax, %cx
        addw    g_FirstCluster, %ax     # calculate absolute sector address

        call    LoadSectors             # read it in!

        leave
        ret

##
# -----------------------------------------------------------------------------
# LoadSectors
# -----------------------------------------------------------------------------
# Reads one or more sectors from the primary disk drive into a buffer.
# In this case, the primary disk drive is floppy drive 0.
#
# NOTE: Function will fail if the destination buffer crosses a segment boundary!
#
# Inputs:
#       es:bx - buffer address
#          ax - logical block address of starting sector
#          cx - sector count
#
# Clobbers:
#       ax, bx, cx, dx
##
.globl LoadSectors
LoadSectors:
        pushw   %bp
        movw    %sp, %bp

        pushw   %bx                     # bp-2: buffer
        pushw   %ax                     # bp-4: lba
        pushw   %cx                     # bp-6: count
        pushw   $RETRY_COUNT            # bp-8: nretries

_LoadSectors_Loop:
        decw    -8(%bp)                 # --nretries
        movw    -4(%bp), %ax            # ax = logical block address
        call    LbaToChs                # returns: cx=cyl, dh=head, dl=sect

        # combine cylinder and sector into cx like below:
        #   cx =       ---ch--- ---cl---
        #   cylinder : 76543210 98
        #   sector   :            543210
        xchgb   %cl, %ch
        rorb    $2, %cl
        orb     %dl, %cl

        movw    -2(%bp), %bx            # es:bx=buffer
        movb    m_DriveNumber, %dl      # dl=drive
        movb    $1, %al                 # al=count
        movb    $0x02, %ah              # ah=fn:LoadSectors
        int     $0x13                   # ch=cyl, dh=head, cl=sect
        jnc     _LoadSectors_Next       # read error if carry flag set

        cmpw    $0, -8(%bp)             # fail if no retries left
        jz      DiskError
        jmp     _LoadSectors_Loop

_LoadSectors_Next:
        PRINT   sz_ProgressChar          # visual feedback that it's working!

        movw    -2(%bp), %bx            # save old buf
        movw    m_SectorSize, %ax
        addw    %ax, -2(%bp)            # buf += m_SectorSize
        cmpw    %bx, -2(%bp)            # did we wrap?
        jbe     DiskError               #   yep, fail! file is too large!
                                        #   TODO: increment segment reg?
        incw    -4(%bp)                 # ++lba
        decw    -6(%bp)                 # --count
        movw    $RETRY_COUNT, -8(%bp)   # reset nretries (flags not affected)

        jz      _LoadSectors_Done       # done if count == 0
        jmp     _LoadSectors_Loop

_LoadSectors_Done:
        leave
        ret

##
# -----------------------------------------------------------------------------
# LbaToChs
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
.globl LbaToChs
LbaToChs:
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
        movw    %ax, %cx                # save lba for later
        movw    m_SectorsPerTrack, %bx  # bx = spt
        imulw   m_HeadCount, %bx        # bx = hpc * spt
        xorw    %dx, %dx                # clear dx
        idivw   %bx                     # ax = dx:ax / bx
        pushw   %ax                     # save cylinder for later

        # H = (LBA / SPT) mod HPC
        # S = (LBA mod SPT) + 1
        xorw    %dx, %dx                # clear dx
        movw    %cx, %ax                # ax = lba
        idivw   m_SectorsPerTrack       # ax = dx:ax / spt, dx = dx:ax mod spt
        incw    %dx                     # +1
        movb    %dl, %cl                # save head for later
        xorw    %dx, %dx                # clear dx
        idivw   m_HeadCount             # dx = dx:ax mod hpc (ax = lba / spt)
        movb    %dl, %dh                # dh = head
        movb    %cl, %dl                # dl = sector
        popw    %cx                     # cx = cylinder

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
#
# Clobbers:
#       ax, bx, si
##
.globl PrintStr
PrintStr:
        xorw    %ax, %ax
        movb    $0x0E, %ah
        movw    $0x07, %bx
        lodsb
        andb    %al, %al
        jz      _PrintStr_done
        int     $0x10
        jmp     PrintStr
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
#
# Clobbers:
#       ah
##
.globl ReadKey
ReadKey:
        xorb    %ah, %ah
        int     $0x16
        ret

##
# -----------------------------------------------------------------------------
# ReadKeyThenReset
# -----------------------------------------------------------------------------
# Waits for keyboard input, then resets the machine.
##
.globl ReadKeyThenReset
ReadKeyThenReset:
        PRINT   sz_KeyPress
        call    ReadKey
        jmp     Reset

##
# -----------------------------------------------------------------------------
# Reset
# -----------------------------------------------------------------------------
# Performs a warm boot by jumping to the CPU reset vector.
##
.globl Reset
Reset:
        movw    $RESETFLAG_WARMBOOT, %gs:BDA_RESETFLAG
        ljmp    $0xFFFF, $0x0000

##
# -----------------------------------------------------------------------------
# FileNotFound
# -----------------------------------------------------------------------------
# Prints a file not found error message, waits for a key press, then resets the
# system.
##
.globl FileNotFound
FileNotFound:
        PRINT   sz_FileNotFound
        jmp     ReadKeyThenReset

##
# -----------------------------------------------------------------------------
# DiskError
# -----------------------------------------------------------------------------
# Prints a disk error message, waits for a key press, then resets the system.
##
.globl DiskError
DiskError:
        PRINT   sz_DiskError
        jmp     ReadKeyThenReset


# -----------------------------------------------------------------------------
# ------------------------------ Padding & Data -------------------------------
# -----------------------------------------------------------------------------

g_FirstCluster:
        .word   0
sz_ProgressChar:
        .asciz  "."
sz_Stage2FileName:
        .asciz  "BOOT    SYS"
sz_FileNotFound:
        .asciz  "\r\nFile not found!"
sz_DiskError:
        .asciz  "\r\nDiskette read error!"
sz_KeyPress:
        .asciz  "\r\nPress any key to try again..."

# pad the remaining space up to the size of a sector (512 bytes)
.space          0x1FE - (. - Entry), 0xCC       # pad with debugbreak opcode
.short          0xAA55                          # boot sector indicator
