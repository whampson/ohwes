#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: boot/fat.S                                                          #
# Created: November 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# This is the boot sector for FAT12-formatted disks. This code is responsible  #
# for loading INIT.SYS (stage 2) and NBOS.SYS (the kernel) into memory, then   #
# passes control over to INIT.SYS to continue booting.                         #
#                                                                              #
# INIT.SYS is placed at 0x09000 and the kernel at 0x10000. This effectively    #
# limits the kernel code size to around 576 KiB because memory above 0x9FFFF   #
# is unusable in Real Mode. This shouldn't be a problem  becuase I don't       #
# expect the kernel ever to get that big. :)                                   #
#==============================================================================#

# TODO: this file is currently PACKED TO THE BRIM. We only have 512 bytes to
# work with, and if you subtract the space needed for the BPB, that leaves us
# with only 448 bytes of code/data to work with to load the rest of the system.
# THAT'S NOT A LOT, and it would be nice to have some space left over for FAT16
# support, or cool boot messages, or something else :)
# This code can easily be optimized by removing unnecessary subroutines and
# making more assumptions about the disk layout (e.g. sector size always 512B,
# INIT.SYS and NBOS.SYS never fragmented, first data sector always 33, etc.),
# after all, we /are/ in total control of the BIOS Parameter Block, and it's not
# my problem if you write this bootsector onto the wrong kind of disk!

.code16
#include <nb/boot.h>

.global entry
entry:
    jmp boot_code
    nop

# -----------------------------------------------------------------------------
# --------------------------- BIOS Parameter Block ----------------------------
# -----------------------------------------------------------------------------
# These are parameters for a 1440 KiB floppy disk (known as "1.44 MB")
oem_name:               .ascii      "OHWES0.1"
bytes_per_sector:       .short      512
sectors_per_cluster:    .byte       1
reserved_sectors:       .short      1
table_count:            .byte       2
root_dir_entries:       .short      224
total_sectors:          .short      2880
media_type:             .byte       0xF0
sectors_per_table:      .short      9
sectors_per_track:      .short      18
heads:                  .short      2
hidden_sectors:         .int        0
large_sectors:          .int        0
drive_number:           .byte       0
_reserved:              .byte       0
extended_boot_sig:      .byte       0x29
volume_id:              .int        0xCA55E77E
label:                  .ascii      "SYSTEM DISK"
file_system_type:       .ascii      "FAT12   "

# -----------------------------------------------------------------------------
# ------------------------- Useful Memory Addresses ---------------------------
# -----------------------------------------------------------------------------
FAT                                 = 0x1000
ROOTDIR                             = 0x2000

# -----------------------------------------------------------------------------
# --------------------------------- Boot Code ---------------------------------
# -----------------------------------------------------------------------------
boot_code:
    cli
    clc
    cld
    ljmpw   $BOOT_SEG, $continue    # ensure cs is correct

continue:
    push    %cs
    popw    %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %ss
    movw    $BOOT_STACK, %sp
    movb    %dl, drive_number

reset_disk:
    sti
    int     $0x13                   # Int 13h 00h: Reset Drive
    jc      error

greeting:
    leaw    s_loading, %si
    call    print

load_fat:
    movw    reserved_sectors, %ax   # FAT begins in sector following reserved area
    addw    sectors_per_table, %cx
    movw    $FAT, %bx
    call    read_sectors

load_root_dir:
    movw    root_dir_entries, %dx
    bsfw    bytes_per_sector, %cx
    shlw    $5, %dx                 # size_bytes = root_dir_entries * sizeof(root_dir_entry)
    shrw    %cl, %dx                # num_sects  = size_bytes / bytes_per_sector
    xchgw   %dx, %cx
    movzbw  table_count, %dx
    movw    reserved_sectors, %ax
_find_sector:
    addw    sectors_per_table, %ax  # root dir begins immediately following the FATs
    decw    %dx
    jz      _do_load
    jmp     _find_sector
_do_load:
    movw    %cx, %dx
    addw    %ax, %dx
    movb    %dl, first_data_sector  # store first data sector number for later use
    movw    $ROOTDIR, %bx
    call    read_sectors

load_stage2:
    movw    $INIT_SEG, %ax          # INIT.SYS gets loaded at 0x9000
    leaw    s_init_file, %di
    call    load_sys_file

load_kernel:
    movw    $KERN_SEG, %ax          # NBOS.SYS gets loaded at 0x10000
    leaw    s_kern_file, %di
    call    load_sys_file

go_to_stage2:
    ljmpw   $INIT_SEG, $0
    # see ya!

error:
    leaw    s_error, %si
    call    print
retry_boot:
    xorb    %ah, %ah
    int     $0x16                   # Int 16h 00h: Wait for Next Keystroke
    int     $0x19                   # Int 19h 00h: Invoke Boot Loader


# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------
###
# Loads a system file into memory.
#
#   Inputs: ax - destination segment
#           di - file name pointer
#  Outputs: File contents at ax:0000.
# Clobbers: ax, bx, cx, dx, si, es, fs
###
load_sys_file:
    xorw    %bx, %bx
    movw    %ax, %fs
    pushw   %ds
    popw    %es
    call    read_file
    ret

####
# Reads a file from the boot disk.
#
#   Inputs: es:di - file name pointer
#           fs:bx - dest address
#  Outputs: File contents at fs:bx.
# Clobbers: ax, bx, cx, dx, si, es
####
read_file:
    pushw   %bp
    movw    %sp, %bp
    pushw   %di
    movw    root_dir_entries, %ax
    movw    $ROOTDIR, %dx
    cld
_find_file_loop:                    # look for the file name in the root dir
    movw    -2(%bp), %di
    movw    %dx, %si
    movw    $11, %cx                # filename length
    repz    cmpsb
    jz      _load_cluster
    addw    $32, %dx                # dir entry size
    decw    %ax
    jnz     _find_file_loop
    jmp     error                   # not found!
_load_cluster:
    movw    %dx, %di                # dir entry ptr
    movw    $FAT, %si
    movw    0x1A(%di), %ax          # first cluster number for file data
    pushw   %ax                     # initial cluster
    pushw   %bx                     # initial address
    pushw   %fs
    popw    %es
_load_cluster_loop:                 # load the next cluster in the chain
    movzbw  first_data_sector, %bx
    movzbw  sectors_per_cluster, %cx
    movw    -4(%bp), %ax            # current cluster
    subw    $2, %ax                 # sector num = first data sector + (cluster num - 2) * cluster size
    imulw   %cx, %ax                #   cluster numbers 0 and 1 are reserved and do not exist on the disk
    addw    %bx, %ax
    popw    %bx                     # addr
    call    read_sectors
    pushw   %bx                     # addr + cluster size
_find_next_cluster:
    movw    -4(%bp), %cx            # current cluster
    movw    %cx, %bx
    shrw    $1, %bx                 # index = (cluster / 2) + cluster
    addw    %cx, %bx                #   entry is 12 bits (hence FAT12), so we multiply by 1.5 to get index
    movw    (%bx,%si,1), %ax        # entry = FAT[index]
    testw   $0x01, %cx
    jnz      _odd_cluster
_even_cluster:
    andw    $0x0FFF, %ax            # even cluster number: mask-off upper 4 bits
    jmp     _check_entry_value
_odd_cluster:
    shrw    $4, %ax                 # odd cluster number: simply shift out bottom 4 bits
_check_entry_value:
    movw    %ax, -4(%bp)            # store next cluster number
    cmpw    $0xFF8, %ax             # check cluster number
    jae     _load_file_done         #   0x000: free cluster, should not be in chain
    cmpw    $0xFF0, %ax             #   0x001: reserved, should not be in chain
    jae     error                   #   0x002-0xFEF: valid cluster number
    cmpw    $0x002, %ax             #   0xFF0-0xFF7: reserved, should not be in chain
    jae     _load_cluster_loop      #   0xFF8-0xFFF: end of chain marker
    jmp     error
_load_file_done:
    leave
    ret

####
# Reads a contiguous line of sectors from the boot disk.
#
# NOTE: For floppies, a multi-sector read may stop after the last sector of a
# track is read.
#
#   Inputs: ax - linear sector number
#           cx - number of sectors
#           es:bx - destination buffer address, should be 512-byte aligned
#  Outputs: Sector contents in memory at original es:bx address
#           es:bx - incremented to end of buffer (ready for next read)
# Clobbers: ax, bx, cx, dx, es
####
read_sectors:
    pushw   %bp
    movw    %sp, %bp
    pushw   %cx
_read_sectors_loop:
    pushw   %bx
    call    read_sector
    popw    %bx
    addw    bytes_per_sector, %bx
    jnc     _read_sectors_next
    movw    %es, %dx
    addw    $0x1000, %dx
    movw    %dx, %es
_read_sectors_next:
    incw    %ax
    decw    -2(%bp)
    jnz     _read_sectors_loop
_read_sectors_done:
    leave
    ret

####
# Reads a single sector from the boot disk.
#
#   Inputs: ax - linear sector number
#           es:bx - destination buffer address
#  Outputs: Sector contents in memory at es:bx.
# Clobbers: cx, dx, es
####
read_sector:
    pushw   %ax
    pushw   %bx
    call    lba2chs
    xchgb   %ch, %cl
    shlb    $6, %cl
    andb    $0x3F, %dl
    orb     %dl, %cl
    popw    %bx
    movb    drive_number, %dl
    movb    $1, %al
    movb    $0x02, %ah
    int     $0x13                   # Int 13h 02h: Read Sectors from Disk
    jc      error
    movw    $0x07, %bx
    movb    $0x0E, %ah
    movb    $0x2E, %al              # print a dot for each sector read
    int     $0x10                   # Int 10h 0Eh: Write Character as TTY
    popw    %ax
    ret

####
# Converts a Logical Block Address into a Cylinder-Head-Sector address.
#
#   Inputs: ax - logical block address (linear sector number)
#  Outputs: cx - cylinder
#           dh - head
#           dl - sector
# Clobbers: ax, bx
####
lba2chs:
    # C = LBA / (HPC * SPT)
    movw    %ax, %cx
    movw    sectors_per_track, %bx
    imulw   heads, %bx
    xorw    %dx, %dx
    idivw   %bx
    pushw   %ax                     # store cylinder for later

    # S = (LBA mod SPT) + 1
    # H = (LBA ÷ SPT) mod HPC
    movw    %cx, %ax
    xorw    %dx, %dx
    idivw   sectors_per_track
    incw    %dx
    movb    %dl, %cl
    xorw    %dx, %dx
    idivw   heads

    movb    %dl, %dh                # head
    movb    %cl, %dl                # sector
    popw    %cx                     # cylinder
    ret

####
# Prints a zero-terminated string.
#
#   Inputs: ds:si - address of string
#  Outputs: (none)
# Clobbers: ax, bx, si
####
print:
    movb    $0x0E, %ah
    movw    $0x07, %bx
_print_loop:
    lodsb
    andb    %al, %al
    jz      _print_done
    int     $0x10                   # Int 10h 0Eh: Write Character as TTY
    jmp     _print_loop
_print_done:
    ret


# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------
first_data_sector:      .byte       -1
s_init_file:            .ascii      "INIT    SYS"
s_kern_file:            .ascii      "NBOS    SYS"
s_loading:              .asciz      "Loading"
s_error:                .asciz      "\r\nInvalid boot disk!\r\n"
pad:                    .space      0x1FE - (. - entry)
boot_sig:               .short      0xAA55
