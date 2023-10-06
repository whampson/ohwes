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

#
# TODO: read BOOT.SYS (stage2) and OHWES.SYS (kernel) into memory from FAT
#

RESETMODE               = 0x0472                # Soft reset mode address
RESETMODE_NOMEMTEST     = 0x1234                # Skip memory test on soft reset

.text
.code16

#
# Global entry point for boot loader.
# DO NOT MOVE, RENAME, OR MODIFY THIS FUNCTION IN ANY WAY.
#
.globl _start
_start:
        jmp             Stage1
        nop

#
# BIOS Parameter Block for a FAT12-formatted 3.5" 1.44MB floppy disk.
# Do NOT move this. Tweak with care.
#
OemName:                .ascii                  "whampson"
SectorSize:             .short                  512
SectorsPerCluster:      .byte                   1
ReservedSectors:        .short                  1
FatCount:               .byte                   2
RootDirCapacity:        .short                  224
SectorCount:            .short                  2880
MediaType:              .byte                   0xF0
SectorsPerFat:          .short                  9
SectorsPerTrack:        .short                  18
HeadCount:              .short                  2
HiddenSectors:          .int                    0
SectorCount32:          .int                    0
DriveNumber:            .byte                   0
_Reserved:              .byte                   0
Signature:              .byte                   0x29
VolumeId:               .int                    0xCA55E77E
Label:                  .ascii                  "OH-WES     "
FsType:                 .ascii                  "FAT12   "

.macro PRINT str
        leaw \str, %si
        call bios_print
.endm

#
# Initial boot loader code.
# Gotta be tight; file must assemble to exactly 512 bytes!
#
# We start off executing at 0x7C00. We set the stack pointer to this address
# (remember, the stack grows towards 0!) and load BOOT.SYS at 0x7E00,
# immediately after the boot sector in memory.
#
Stage1:
        #
        # init
        #
        cld                                     # clear direction bit
        xorw            %ax, %ax                # zero segment regs
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %ss
        movw            $0x7C00, %bp            # set up stack
        movw            %bp, %sp
        PRINT           s_hello                 # print a welcome message

        #
        # load root dir
        #
        movw            RootDirCapacity, %cx    # calculate root dir size
        shlw            $5, %cx                 #   maxentries*32
        subw            %cx, %sp                # make stack space
        movw            %sp, %bx                # ex:bx = pDest = sp
        movw            SectorsPerFat, %ax      # calculate root dir start sector
        mulb            FatCount                #   nreserved+(nfatsectors*nfats)
        addw            ReservedSectors, %ax
        pushw           %ax                     # save start sector for later
        decw            %cx                     # caclulate num sectors to read
        shrw            $9, %cx                 #   ((nbytes-1)/512)+1
        incw            %cx
        pushw           %cx                     # save sector count for later
        call            read_sectors            # read root dir

        #
        # load BOOT.SYS
        # todo: scan root dir for file? or ensure it's always the first entry in the dir
        #
        popw            %ax                     # grab root dir sector count
        popw            %bx                     # grab root dir start sector
        addw            %bx, %ax                # calculate first data cluster address
        movzwl          %sp, %ebx               # ebx = root dir entry pointer
        movw            26(%ebx), %dx           # first cluster for file
        subw            $2, %dx                 # cluster indices 0 and 1 reserved
        addw            %dx, %ax                # sector address of first cluster in file
        movw            28(%ebx), %cx           # file size in bytes
        decw            %cx                     # calculate num clusters to read
        shrw            $9, %cx                 #  ((nbytes-1)/512)+1
        incw            %cx
        xchgw           %ax, %cx
        mulb            SectorsPerCluster
        xchgw           %ax, %cx

        movw            $0x7E00, %bx            # dest = 0x07E00
        call            read_sectors
        PRINT           s_newline

        jmp             stage2                  # onwards!

readkey_reset:
        call            bios_readkey
reset:
        movw            $RESETMODE_NOMEMTEST, RESETMODE
        ljmp            $0xFFFF, $0x0000

halt:
        hlt
        jmp             halt

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# read_sectors
# -----------------------------------------------------------------------------
# Reads one or more sectors from the primary disk drive into a buffer.
# In this case, the primary disk drive is (likely) floppy drive 0.
#
# Inputs:
#       es:bx - buffer address
#       ax - logical block address (starting sector)
#       cx - sector count
#
# Clobbers:
#       (everything, probably)
#
read_sectors:
        test            %cx, %cx
        jle             _read_sectors_error

        pushw           %bp
        movw            %sp, %bp

        pushw           %bx                     # bp-2: buffer
        pushw           %ax                     # bp-4: lba
        pushw           %cx                     # bp-6: count

_read_sectors_loop:
        movw            -4(%bp), %ax            # ax = logical block address
        call            lba2chs                 # returns: cx=cylinder, dh=head, dl=sector

        # need to combine cylinder and sector into cx like below:
        #   cx =       ---ch--- ---cl---
        #   cylinder : 76543210 98
        #   sector   :            543210
        xchgb           %cl, %ch
        rorb            $2, %cl
        orb             %dl, %cl

        movw            -2(%bp), %bx            # es:bx=buffer
        movb            DriveNumber, %dl       # dl=drive
        movb            $1, %al                 # al=count
        movb            $0x02, %ah              # ah=fn:read_sectors
        int             $0x13                   # ch=cyl, dh=head, cl=sect
        jc              _read_sectors_error             # TODO: retry on error?

        leaw            s_progress, %si
        call            bios_print

        movw            SectorSize, %ax
        addw            %ax, -2(%bp)            # buf += SectorSize
        incw            -4(%bp)                 # lba++
        decw            -6(%bp)                 # count--

        jz              _read_sectors_done
        jmp             _read_sectors_loop

_read_sectors_done:
        leave
        ret

_read_sectors_error:
        leaw            s_diskerr, %si
        call            bios_print
        jmp             readkey_reset

# -----------------------------------------------------------------------------
# lba2chs
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
#
lba2chs:
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
        idivw           HeadCount              # dx = dx:ax mod hpc (ax = lba / spt)
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
#       ds:si - address of string to print
# Clobbers:
#       (potentially everything due to BIOS call)
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
#       al - key pressed
# Clobbers:
#       (potentially everything due to BIOS call)
#
.globl bios_readkey
bios_readkey:
        xorb            %ah, %ah
        int             $0x16
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------
.globl kernel_sectors

s_hello:
        .asciz          "Loading BOOT.SYS"

s_diskerr:
        .ascii          "\r\nDiskette read error!"
        .asciz          "\r\nPress any key to try again..."
s_newline:
        .asciz          "\r\n"
s_progress:
        .asciz          "."

.space          0x1FE - (. - _start), 0xCC      # padding
        .short          0xAA55                          # boot Signature
