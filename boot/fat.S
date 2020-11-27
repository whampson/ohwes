#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of AFROS - Another FRee Operating System.                  #
#                                                                              #
# AFROS is free software. You can redistribute it and/or modify it under the   #
# terms of the MIT License as written in the LICENSE file in the top-level     #
# directory of this codebase.                                                  #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
#     File: fat.S                                                              #
#   Author: Wes Hampson                                                        #
#     Date: Nov. 22, 2020                                                      #
#     Desc: Boot sector for FAT12-formatted disks. This code is responsible    #
#           for loading INIT.SYS and KERNEL.SYS into memory and passing        #
#           control onto INIT.SYS.                                             #
#==============================================================================#

.code16

.global entry
entry:
    jmp boot_code
    nop

# -----------------------------------------------------------------------------
# --------------------------- BIOS Parameter Block ----------------------------
# -----------------------------------------------------------------------------
oem_name:               .ascii      "AFROS0.1"
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
volume_id:              .int        0xAFAFAFAF
label:                  .ascii      "SYSTEM DISK"
file_system_type:       .ascii      "FAT12   "

# --------------------------------------------------------------------------
# -------------------------------- Constants -------------------------------
# --------------------------------------------------------------------------
FAT_ADDR                            = 0x0500
ROOT_DIR_ADDR                       = 0x2000
INIT_SEGMENT                        = 0x0000
INIT_ENTRY                          = 0x8000
KERNEL_SEGMENT                      = 0x1000
KERNEL_ENTRY                        = 0x0000
SIZEOF_DIR_ENTRY                    = 32
FILE_NAME_LENGTH                    = 11

# -----------------------------------------------------------------------------
# --------------------------------- Boot Code ---------------------------------
# -----------------------------------------------------------------------------
boot_code:
    cli
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %ss
    movl    $0x7C00, %esp

reset_disk:
    sti
    int     $0x13                   # Int 13h 00h: Reset Drive
    jc      error

load_fat:
    movw    reserved_sectors, %ax   # start of FAT = sector following reserved area
    addw    sectors_per_table, %cx  # sector count
    movw    $FAT_ADDR, %bx
    call    read_sectors

load_root_dir:
    movw    root_dir_entries, %dx
    bsfw    bytes_per_sector, %cx   # assumes power of 2
    shlw    $5, %dx                 # size_bytes = root_dir_entries * sizeof(root_dir_entry)
    shrw    %cl, %dx                # num_sects  = size_bytes / bytes_per_sector
    xchgw   %dx, %cx
    movzbw  table_count, %dx
    movw    reserved_sectors, %ax
_load_root_dir_find_sector:
    addw    sectors_per_table, %ax
    decw    %dx
    jz      _load_root_dir_load
    jmp     _load_root_dir_find_sector
_load_root_dir_load:
    movw    %cx, %dx
    addw    %ax, %dx
    movb    %dl, first_data_sector  # store first data sector for later use
    movw    $ROOT_DIR_ADDR, %bx
    call    read_sectors

load_init_sys:
    movw    init_seg, %fs
    movw    $INIT_ENTRY, %bx
    leaw    init_file, %di
    call    read_file

load_kernel_sys:
    movw    kernel_seg, %fs
    movw    $KERNEL_ENTRY, %bx
    leaw    kernel_file, %di
    call    read_file

go2init:
    # Off to stage 2!
    ljmp    $INIT_SEGMENT, $INIT_ENTRY

error:
    leaw    disk_error_msg, %si
    call    print
_retry_boot:
    xorb    %ah, %ah
    int     $0x16                   # Int 16h 00h: Wait for Next Keystroke
    int     $0x19                   # Int 19h 00h: Invoke Boot Loader


# ----------------------------------------------------------------------------
# ------------------------------- Subroutines --------------------------------
# -----------------------------------------------------------------------------
####
# Reads a file from disk.
#
#   Inputs: es:di - file name
#           fs:bx - dest address
#  Outputs: File contents at fs:bx.
# Clobbers: bx, cx, dx, si
####
read_file:
    movw    root_dir_entries, %ax
    movw    $ROOT_DIR_ADDR, %dx
    pushw   %di
    cld
_find_file_loop:
    movw    (%esp), %di
    movw    %dx, %si
    movw    $FILE_NAME_LENGTH, %cx
    repz    cmpsb
    jz      _load_cluster_init
    addw    $SIZEOF_DIR_ENTRY, %dx
    decw    %ax
    jnz     _find_file_loop
    jmp     error
_load_cluster_init:
    movw    %dx, %di                # dir entry ptr
    movl    $FAT_ADDR, %esi         # FAT ptr
    movw    0x1A(%di), %ax
    pushw   %ax                     # starting cluster
    pushw   %bx                     # starting address
    xorl    %ebx, %ebx
_load_cluster_loop:
    movzbw  first_data_sector, %bx
    movzbw  sectors_per_cluster, %cx
    movw    2(%esp), %ax            # current cluster
    subw    $2, %ax
    imulw   %cx, %ax                # sector num = first data sector + (cluster num - 2) * cluster size
    addw    %bx, %ax
    popw    %bx                     # addr
    call    read_sectors
    pushw   %bx                     # addr + cluster size
_find_next_cluster:
    movw    2(%esp), %ax            # current cluster
    movw    %ax, %bx
    shrw    $1, %bx
    addw    %ax, %bx                # FAT index = (cluster / 2) + cluster
    movw    (%esi, %ebx), %ax       # FAT[index]
    movw    %ax, %bx
    andw    $0x01, %bx
    jz      _even_cluster
_odd_cluster:
    andw    $0xFFF, %ax
    jmp     _check_entry_value
_even_cluster:
    shrw    $4, %ax
_check_entry_value:
    movw    %ax, 2(%esp)
    cmpw    $0xFF0, %ax
    jb      _load_cluster_loop
    je      _load_file_done
    cmpw    $0xFF8, %ax
    jae     _load_file_done
    jmp     error
_load_file_done:
    popw    %bx
    popw    %ax
    popw    %di
    ret

####
# Reads a contiguous line of sectors from disk.
#
# NOTE: For floppies, reading multiple sectors may stop after
# the last sector is read on a given track.
#
#   Inputs: ax - linear sector number
#           cx - number of sectors
#           fs:bx - dest address
#  Outputs: Sector contents in memory at original fs:bx address
#           fs:bx - incremented to end of buffer
# Clobbers: ax, bx, dx
####
read_sectors:
    #
    # !!! TODO: Ensure disk read can cross segment boundaries
    #
    pushw   %cx
_read_sectors_loop:
    call    read_sector
    addw    bytes_per_sector, %bx
    incw    %ax
    decw    (%esp)
    jnz     _read_sectors_loop
_read_sectors_done:
    popw    %cx
    ret

####
# Reads a single sector from disk.
#
#   Inputs: ax - linear sector number
#           fs:bx - dest address
#  Outputs: Sector contents in memory at fs:bx.
# Clobbers: cx, dx
####
read_sector:
    pushw   %es
    pushw   %ax
    pushw   %bx
    pushw   %fs                     # using fs so we don't clobber es (used with cmpsb)
    popw    %es
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
    popw    %ax
    popw    %es
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
    movw    %ax, tmp_cylinder

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
    movw    tmp_cylinder, %cx       # cylinder
    ret

####
# Prints a zero-terminated string.
#
#   Inputs: si - address of string
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
tmp_cylinder:           .short      -1
first_data_sector:      .byte       -1
init_file:              .ascii      "INIT    SYS"
init_seg:               .short      INIT_SEGMENT
kernel_file:            .ascii      "KERNEL  SYS"
kernel_seg:             .short      KERNEL_SEGMENT
disk_error_msg:         .asciz      "Invalid system disk. Press any key to retry.\r\n"
pad:                    .space      0x1FE - (. - entry)
boot_sig:               .short      0xAA55
