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
# the structures necessary to run the CPU in Protected Mode, and finally       #
# switch into Protected Mode.                                                  #
#==============================================================================#

# Physical Memory Map:
# 0x00000-0x0FFFF:  64 KiB
#   Kernel Data Area
#     0x0000-0x0FFF: Reserved (Real Mode IVT, BIOS)
#     0x1000-0x1FFF: CPU Structures & Boot Data
#     0x2000-0x2FFF: Memory Info
#     0x3000-0x3FFF: Page Directory
#     0x4000-0x4FFF: Page Table 0, 0-4 MiB (1024 4K pages)
#     0x5000-0xEFFF: Additional space for page tables
#     0xFFF0-0xFFFF: PID0 Stack
# 0x10000-0x8FFFF:  512 KiB
#   Kernel code area
# 0x90000-0x9FFFF:  64 KiB
#   (free)
# 0xA0000-0xFFFFF:  384 KiB
#   Reserved for Hardware/BIOS (do not use)
# 0x100000-(EOM):   ??? KiB
#   Available to user/kernel
#   Be mindful of 15M-16M hole

# -----------------------------------------------------------------------------
# -------------------------- Useful Memory Addresses --------------------------
# -----------------------------------------------------------------------------

# 0x1000-0x1FFF: CPU Structures and Boot Data
IDT_BASE                    = 0x1000        # Interrupt Descriptor Table
GDT_BASE                    = 0x1800        # Global Descriptor Table
IDT_PTR                     = 0x1900        # IDT base/limit descriptor
GDT_PTR                     = 0x1908        # GDT base/limit descriptor
CUR_ROW                     = 0x1A00        # Cursor Position (row)
CUR_COL                     = 0x1A01        # Cursor Position (column)
CUR_BEG                     = 0x1A02        # Cursor Shape (scan line begin)
CUR_END                     = 0x1A03        # Cursor Shape (scan line end)

# 0x2000-0x2FFF: Memory Info
MEMINFO                     = 0x2000        # Memory Info Area (4K bytes)
MEMINFO_1                   = 0x2010        # INT 0x15 AX=0xE820 result
MEMINFO_2A                  = 0x2008        # INT 0x15 AX=0xE801 result 1
MEMINFO_2B                  = 0x200A        # INT 0x15 AX=0xE801 result 2
MEMINFO_3                   = 0x2000        # INT 0x15 AH=0x88 result

# -----------------------------------------------------------------------------
# ----------------------------- Stage 2 Boot Code -----------------------------
# -----------------------------------------------------------------------------

.code16

.global init
init:
    xorw    %ax, %ax
    pushw   %cs                 # CS = 0x9000
    popw    %ds                 # DS = 0x9000
    movw    %ax, %es            # ES = 0x0000

    call    store_cursor
    call    detect_memory
    call    enable_a20
    call    setup_gdt

    cli
    movl    %cr0, %eax
    orl     $0x01, %eax         # enable Protected Mode
    movl    %eax, %cr0

    ljmpl   $0x10, $0x10000     # jump to 32-bit code!



####
# Stores the current cursor position and shape.
#
#   Inputs: (none)
#  Outputs: 0xA000 - current row
#           0xA001 - current column
#           0xA002 - cursor starting scan line
#           0xA003 - cursor ending scan line
# Clobbers: ax, bx, cx, dx
####
store_cursor:
   xorb     %bh, %bh
   movb     $0x03, %ah
   int      $0x10
   movb     %dh, %es:CUR_ROW
   movb     %dl, %es:CUR_COL
   movb     %ch, %es:CUR_BEG
   movb     %cl, %es:CUR_END
   ret

####
# Detects available memory on the system.
#
#   Inputs: (none)
#  Outputs: (various values in the Memory Info Area)
# Clobbers: (all, probably)
####
detect_memory:
    movw    $0x400, %cx
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
# This function utilizes the ACPI function "Query System Address Map" available
# on PCs since around 2002 to generate a table of usable and unusable physical
# address ranges. The resulting table is stored in the Memory Info Area at
# absolute address 0x1010. The table consists of a maximum of 170 entries, each
# 6 DWORDs in length (24 bytes). The end of the table is marked by an entry
# containing all zeros.
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
#  Outputs: 0x1010 - physical address mapping table (24 bytes per entry)
# Clobbers: eax, ebx, ecx, edx, %di
####
detect_memory1:
    clc
    movw    $MEMINFO_1, %di     # dest buffer
    xorl    %ebx, %ebx          # continuation value (0 to begin)
_detect_memory1_loop:
    movl    $0x18, %ecx         # table entry size
    movl    $0x534D4150, %edx   # 'SMAP'
    movl    $0xE820, %eax       # ACPI Query System Address Map function
    int     $0x15
    jc      _detect_memory1_done
    testl   %ebx, %ebx
    jz      _detect_memory1_done
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
#  Outputs: 0x1008 - number of contiguous 1K blocks between 1M and 16M (2 bytes)
#           0x100A - number of contiguous 64K blocks between 16M and 4G (2 bytes)
# Clobbers: ax, bx, cx, dx
####
detect_memory2:
    clc
    xorw    %cx, %cx
    xorw    %dx, %dx
    movw    $0xE801, %ax
    int     $0x15
    jc      _detect_memory2_done
    je      _detect_memory2_done
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
# This function is limited to detecting 64M, but is guaranteed to work on all
# systems with a BIOS. If detection somehow fails, the boot process will halt
# as we have no way of knowing how much memory the system has!
#
#   Inputs: (none)
#  Outputs: 0x1000 - number of contiguous 1K blocks between 1M and 64M (2 bytes)
# Clobbers: ax
####
detect_memory3:
    clc
    movb    $0x88, %ah
    int     $0x15
    jc      mem_error
    test    %ax, %ax
    jz      mem_error
    movw    %ax, %es:MEMINFO_3      # contiguous 1K blocks above 1M (limit 64M)
    ret

####
# Enables the A20 address line.
#
# This function attempts to enable the A20 line using three methods.
# If all methods fail, the system is halted.
#
#   Inputs: (none)
#   Output: (none)
# Clobbers: (lots)
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
# This test is done by reading a value at a known address (the boot sector
# signature in this case), then comparing that value with the value read at a
# different address, but an address which would "wrap around" to the known
# address if A20 is disabled. If the values are the same, then A20 is not
# enabled.
#       0000:7DFE = 0x007DFE
#       FFFF:7E0E = 0x007DFE if A20 disabled
#       FFFF:7E0E = 0x107DFE if A20 enabled
#
#   Inputs: (none)
#  Outputs: CF set if A20 is not enabled
# Clobbers: ax, dx, fs
###
test_a20:
    clc
    movw    $0xFFFF, %ax
    movw    %ax, %fs
    movw    %es:0x7DFE, %dx         # boot sector signature (0xAA55)
    movw    %fs:0x7E0E, %ax
    cmpw    %ax, %dx                # should not be equal if A20 enabled
    je      _a20_not_set
    jmp     _test_a20_done
_a20_not_set:
    stc
_test_a20_done:
    ret

setup_gdt:
    movl    $temp_gdt_base, %esi    # src
    movl    $GDT_BASE, %edi         # dest
    movzwl  gdt_r, %ecx             # count
    rep     movsb
    lgdtl   gdt_r
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

# The GDT. Open up all of memory and use Paging for protection.
.align 16
temp_gdt_base:
    .quad   0x0000000000000000      # (not used)
    .quad   0x0000000000000000      # TSS_SEG   (initialized in kmain)
    .quad   0x00CF9A000000FFFF      # KERNEL_CS
    .quad   0x00CF92000000FFFF      # KERNEL_DS
    .quad   0x00CFF2000000FFFF      # USER_CS
    .quad   0x00CFF2000000FFFF      # USER_DS
    .quad   0x0000000000000000      # LDT_SEG   (initialized in kmain)
temp_gdt_end:

.align 4
gdt_r:
    .word   temp_gdt_end - temp_gdt_base  - 1
    .long   GDT_BASE

s_success:          .asciz      "\r\nSuccess!\r\n"
s_error_mem:        .asciz      "\r\nError: failed to detect memory!\r\n"
s_error_a20:        .asciz      "\r\nError: failed to enable A20!\r\n"
