#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
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
#    File: boot/init.S                                                         #
# Created: November 25, 2020                                                   #
#  Author: Wes Hampson                                                         #
#------------------------------------------------------------------------------#
# This is stage 2 of the bootloader. The role of stage 2 is to gather          #
# information about the system's memory, enable the A20 address line, set up   #
# the minimum structures necessary to run 32-bit code on the CPU in Protected  #
# Mode, and finally, switch into Protected Mode and call the kernel.           #
#==============================================================================#

.code16
#include <nb/init.h>

.global init
init:
    xorw    %ax, %ax
    pushw   %cs                 # CS = 0x0900
    popw    %ds                 # DS = 0x0900
    movw    %ax, %es            # ES = 0x0000

    call    store_cursor
    call    detect_memory
    call    enable_a20
    call    setup_gdt

    cli
    movl    %cr0, %eax
    orl     $0x01, %eax         # enable Protected Mode
    movl    %eax, %cr0

    ljmpl   $0x08, $KERNEL_ENTRY


####
# Stores the current cursor position and shape.
#
#   Inputs: (none)
#  Outputs: cursor row
#           cursor column
#           cursor starting scan line
#           cursor ending scan line
# Clobbers: ax, bx, cx, dx
####
store_cursor:
   xorb     %bh, %bh
   movb     $0x03, %ah
   int      $0x10
   movb     %dh, %es:CURSOR_ROW
   movb     %dl, %es:CURSOR_COL
   movb     %ch, %es:CURSOR_BEG
   movb     %cl, %es:CURSOR_END
   ret

####
# Detects available memory on the system using three separate BIOS calls.
# Each subsequent call has wider support on existing systems. If the third
# call fails, the system will halt as we have no way of knowing how much memory
# we have!
#
#   Inputs: (none)
#  Outputs: (various values in the Memory Info Area)
# Clobbers: (all, probably)
####
detect_memory:
    movw    $PAGE_SIZE>>2, %cx
    movl    $MEMINFO, %edi
    xorl    %eax, %eax
    rep     stosl               # zero-out memory info region
    call    detect_memory1
    call    detect_memory2
    call    detect_memory3      # will halt if unable to detect
    ret

####
# Gets a mapping of physical address space and stores the result in the Memory
# Info Area.
#
# This function utilizes the ACPI function "Query System Address Map", available
# on PCs since around 2002, to generate a table of usable and unusable physical
# address ranges. Each table entry is 6 DWORDs in length (24 bytes).
#
# The table entry format is as follows:
#   0x00: low base address
#   0x04: high base address
#   0x08: low limit
#   0x0C: high limit
#   0x10: type
#   0x14: extended attributes
# 
# The entry types are as follows:
#      1: free to use
#      2: reserved, do not use
#      3: ACPI tables, do not use (unless OS has read ACPI tables)
#      4: non-volatile, do not use (unless implementing suspend-to-RAM sleep)
#      5: bad RAM, do not use
#      6: disabled, do not use
#
# The extended attributes are as follows:
#  bit 0: extended attributes present if set
#  bit 1: non-volatile
#  bit 2: slow access
#  bit 3: hardware error logging area
#
#   Inputs: (none)
#  Outputs: table describing physical address mapping (24 bytes per entry)
# Clobbers: eax, ebx, ecx, edx, di
####
detect_memory1:
    clc
    movw    $MEMINFO_1, %di         # dest buffer
    xorl    %ebx, %ebx              # continuation value (0 to begin)
_detect_memory1_loop:
    movl    $0x18, %ecx             # table entry size
    movl    $0x534D4150, %edx       # 'SMAP'
    movl    $0xE820, %eax           # ACPI Query System Address Map function
    int     $0x15
    jc      _detect_memory1_done    # error or end of table
    cmpb    $0x80, %ah
    je      _detect_memory1_done    # invalid command
    cmpb    $0x86, %ah
    je      _detect_memory1_done    # unsupported function
    testl   %ebx, %ebx
    jz      _detect_memory1_done    # end of table
    addw    $0x18, %di
    jmp     _detect_memory1_loop
_detect_memory1_done:
    ret

####
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G, then stores
# the result in the Memory Info Area.
#
# This function will work on all systems made after 1994. If detection fails,
# the function will return without error and the Memory Info Area will remain
# unchanged.
#
#   Inputs: (none)
#  Outputs: number of contiguous 1K blocks between 1M and 16M
#           number of contiguous 64K blocks between 16M and 4G
# Clobbers: ax, bx, cx, dx
####
detect_memory2:
    clc
    xorw    %cx, %cx
    xorw    %dx, %dx
    movw    $0xE801, %ax
    int     $0x15
    jc      _detect_memory2_done    # error
    cmpb    $0x80, %ah
    je      _detect_memory2_done    # invalid command
    cmpb    $0x86, %ah
    je      _detect_memory2_done    # unsupported function
    jcxz    _detect_memory2_store
    movw    %cx, %ax
    movw    %dx, %bx
_detect_memory2_store:
    movw    %ax, %es:MEMINFO_2A     # contiguous 1K blocks 1M to 16M
    movw    %bx, %es:MEMINFO_2B     # contiguous 64K blocks above 16M
_detect_memory2_done:
    ret

####
# Gets the number of contiguous 1K blocks of memory above 1M and stores the
# result in the Memory Info Area.
#
# This function is limited to detecting 64M. It is intended as a last resort for
# detecting memory; if it fails, it probably means your system is too old! For
# that reason, a failure will halt the system with an error message.
#
#   Inputs: (none)
#  Outputs: number of contiguous 1K blocks between 1M and 64M
# Clobbers: ax
####
detect_memory3:
    clc
    movb    $0x88, %ah
    int     $0x15
    jc      mem_error               # error
    cmpb    $0x80, %ah
    je      mem_error               # invalid command
    cmpb    $0x86, %ah
    je      mem_error               # unsupported function
    test    %ax, %ax
    jz      mem_error               # no memory...
    movw    %ax, %es:MEMINFO_3      # contiguous 1K blocks above 1M (limit 64M)
    ret

####
# Enables the A20 address line.
#
# This function attempts to enable the A20 line using three methods.
# If all methods fail, the system is halted. Otherwise the system would have a
# very annoying non-contiguous memory hole.
#
#   Inputs: (none)
#   Output: increased addressability!
# Clobbers: ax
####
enable_a20:
    call    test_a20
    jnc     _a20_done
    call    enable_a20_bios
    call    test_a20
    jnc     _a20_done
    call    enable_a20_kbd
    call    test_a20
    jnc     _a20_done
    call    enable_a20_fast
    call    test_a20
    jc      a20_error
_a20_done:
    ret

####
# Enables the A20 line using a BIOS function.
# Not supported on all systems.
#
#   Inputs: (none)
#  Outputs: (none)
# Clobbers: ax
###
enable_a20_bios:
    movw    $0x2401, %ax
    int     $0x15
    ret

####
# Enables the A20 via the PS/2 controller.
# This is the most common method and usually does the trick.
#
#   Inputs: (none)
#  Outputs: (none)
# Clobbers: ax
###
enable_a20_kbd:
    call    empty_8042      # wait for 8042 input buffer to be empty
    movb    $0xD1, %al      # command: Write PS/2 Controller Output Port
    outb    %al, $0x64      # submit command
    call    empty_8042      # wait for 8042 input buffer to be empty
    movb    $0xDF, %al      # enable A20
    outb    %al, $0x60      # submit data
    call    empty_8042      # wait one last time because why not?
    ret
empty_8042:
    inb     $0x64, %al      # 8042 Status Register
    testb   $0x02, %al      # is input buffer full?
    jnz     empty_8042      # if yes, continue checking 'til empty
    ret

####
# Enables the A20 line the "fast" way.
# Not supported on all systems.
#
#   Inputs: (none)
#  Outputs: (none)
# Clobbers: al
###
enable_a20_fast:
    inb     $0x92, %al
    testb   $0x02, %al
    jnz     _a20_fast_done
    orb     $0x02, %al
    outb    %al, $0x92
_a20_fast_done:
    ret

####
# Tests whether the A20 line is enabled.
#
# This test is done by reading a segment:offset with a known value (the boot
# sector signature in this case), then comparing that value with the value read
# at a different segment:offset such that the resulting address would 
# "wrap around" to the first address if A20 is NOT enabled. In other words, if
# A20 is NOT enabled, both addresses would read 0xAA55.
#
#       0000:7DFE -> 0x007DFE = 0xAA55  always
#       FFFF:7E0E -> 0x007DFE = 0xAA55  if A20 disabled
#       FFFF:7E0E -> 0x107DFE = ???     if A20 enabled
#
#   Inputs: (none)
#  Outputs: CF set if A20 is not enabled
# Clobbers: ax, dx, fs
###
test_a20:
    clc
    movw    $0xFFFF, %ax
    movw    %ax, %fs
    movw    %es:0x7DFE, %dx
    movw    %fs:0x7E0E, %ax
    cmpw    %ax, %dx
    je      _a20_not_set
    jmp     _test_a20_done
_a20_not_set:
    stc
_test_a20_done:
    ret

setup_gdt:
    leal    temp_gdt_base, %esi
    leal    GDT_BASE, %edi
    movl    temp_gdt_size, %edx
    movl    %edx, %ecx
    rep     movsb
    decl    %edx
    movw    %dx, GDT_PTR
    movl    $GDT_BASE, GDT_PTR+2
    lgdtl   GDT_PTR
    ret

####
# Prints a zero-terminated string.
#
#   Inputs: ds:si - address of string
#  Outputs: (none)
# Clobbers: ax, bx, si
####
print:
    cld
    movb    $0x0E, %ah
    movw    $0x07, %bx
_print_loop:
    lodsb
    andb    %al, %al
    jz      _print_done
    int     $0x10
    jmp     _print_loop
_print_done:
    ret

# -----------------------------------------------------------------------------
# --------------------------- Error/Reset Functions ---------------------------
# -----------------------------------------------------------------------------

mem_error:
    leaw    s_error_mem, %si
    call    print
    jmp     retry_boot

a20_error:
    leaw    s_error_a20, %si
    call    print
    jmp     retry_boot

retry_boot:
    xorb    %ah, %ah
    int     $0x16
    int     $0x19  

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

# The Global Descriptor Table (temporary)
.align 16
temp_gdt_base:
    .quad   0x0000000000000000      # (not used, required by CPU)
    .quad   0x00CF9A000000FFFF      # code segment
    .quad   0x00CF92000000FFFF      # data segment
temp_gdt_end:

temp_gdt_size:
    .long   temp_gdt_end-temp_gdt_base

s_error_mem:        .asciz      "\r\nError: failed to detect memory!\r\n"
s_error_a20:        .asciz      "\r\nError: failed to enable A20!\r\n"
