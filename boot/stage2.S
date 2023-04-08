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
#         File: boot/stage2.S
#      Created: March 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode (except for jump to Protected Mode)
#
# Gathers information about the system using the BIOS, then switches into
# 32-bit Protected Mode with a flat memory model.
# =============================================================================

#include "boot.h"

PORT_PS2KBD_CMD         = 0x64  # in = status, out = command
PORT_PS2KBD_DATA        = 0x60
PORT_SYSCNTLA           = 0x92

TEMP_CS                 = 0x08
TEMP_DS                 = 0x10

VGA_MODE                = 0x03  # 0x03 = text, CGA/EGA/VGA, 16 colors, B8000h

.text
.code16

##
# Temporary GDT so we can get into Protected Mode.
##
.align 16
temp_gdt:
        .quad           0x0                     # (not used)
        .quad           0x00CF9A000000FFFF      # cs = 0x08
        .quad           0x00CF92000000FFFF      # ds = 0x10
temp_gdt_end:

        .word           0                       # (align)
temp_gdt_reg:
        .word           temp_gdt_end-temp_gdt-1 # GDT limit
        .long           temp_gdt                # GDT base, aligned

# -----------------------------------------------------------------------------
# ---------------------------------- Stage2 -----------------------------------
# -----------------------------------------------------------------------------

##
# Stage 2 entry point.
##
.globl stage2
stage2:
        call            init_video
        leaw            s_hello, %si
        call            bios_println
        call            get_hwinfo
        call            detect_memory
        call            enable_a20

        cli
        lgdtl           temp_gdt_reg
        movl            %cr0, %eax
        orl             $0x01, %eax     # Protected Mode enable bit
        movl            %eax, %cr0
        jmpl            $TEMP_CS, $start32

.code32
start32:
        movw            $TEMP_DS, %ax
        movw            %ax, %ds
        movw            %ax, %ss
        movw            %ax, %es
        xorw            %ax, %ax
        movw            %ax, %fs
        movw            %ax, %gs
        call            init            # initialize devices and load kernel

idle_loop:
        jmp             idle_loop

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

.code16

##
# -----------------------------------------------------------------------------
# init_video
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine.
##
init_video:
        xorw            %ax, %ax
        movb            $VGA_MODE, %al
        int             $0x10   # sets video mode and clears the screen
        ret
##
# -----------------------------------------------------------------------------
# get_hwinfo
# -----------------------------------------------------------------------------
# Gets installed hardware flags using a BIOS routine.
##
get_hwinfo:
        int             $0x11
        movw            %ax, g_hwflags
        ret

##
# -----------------------------------------------------------------------------
# detect_memory
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines. If all methods fail, the system will display an error and halt.
##
detect_memory:
        call            detect_memory_e820
        call            detect_memory_e801
        call            detect_memory_legacy
        ret

####
# Gets the physical address space map.
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
#      3: ACPI tables
#      4: non-volatile
#      5: bad RAM, do not use
#
# The extended attributes are as follows:
#  bit 0: extended attributes present if set
#  bit 1: non-volatile
#  bit 2: slow access
#  bit 3: hardware error logging area
####
detect_memory_e820:
        clc
        xorl            %eax, %eax
        movw            %ax, %es
        movw            $MEMMAP_BASE, %di               # dest buffer
        xorl            %ebx, %ebx                      # continuation value (0 to begin)
_detect_memory_e820_loop:
        movl            $0x18, %ecx                     # table entry size
        movl            $0x534D4150, %edx               # 'SMAP'
        movl            $0xE820, %eax                   # ACPI Query System Address Map function
        int             $0x15
        jc              _detect_memory_e820_done         # error or end of table
        cmpb            $0x80, %ah
        je              _detect_memory_e820_done         # invalid command
        cmpb            $0x86, %ah
        je              _detect_memory_e820_done         # unsupported function
        testl           %ebx, %ebx
        jz              _detect_memory_e820_success         # end of table
        addw            $0x18, %di                      # move table ptr
        cmpb            $0x14, %cl                      # if extended attr not present...
        je              _detect_memory_e820_loop
        movw            $0, -4(%edi)                    # ...ensure it is stored as zero
        jmp             _detect_memory_e820_loop

_detect_memory_e820_success:
        movl            $1, g_has_memory_map
_detect_memory_e820_done:
        ret     # TODO: set method bit

####
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G.
#
# This function will work on all systems made after 1994. If detection fails,
# the function will return without error.
####
detect_memory_e801:
        # data can be returned in ax/bx or cx/dx; initialize all to zero
        xorw            %ax, %ax
        xorw            %bx, %bx
        xorw            %cx, %cx
        xorw            %dx, %dx
        movw            $0xE801, %ax
        int             $0x15
        jcxz            detect_memory_e801_store        # cx = 0, use ax/bx
        movw            %cx, %ax
        movw            %dx, %bx
detect_memory_e801_store:
        movw            %ax, g_ramsize_e801_lo          # contiguous RAM >1M in 1K blocks, up to 16M
        movw            %bx, g_ramsize_e801_hi          # contiguous RAM >16M in 64K blocks
        ret     # TODO: set method bit

####
# Gets the number of contiguous 1K blocks of memory up to 15M or 64M, depending
# on the BIOS. If detection fails, the system will display an error and halt.
####
detect_memory_legacy:
        clc
        movb            $0x88, %ah
        int             $0x15
        jc              detect_memory_LelacyLo
        cmpb            $0x80, %ah                      # (invalid command)
        je              detect_memory_LelacyLo
        cmpb            $0x86, %ah                      # (unsupported func)
        je              detect_memory_LelacyLo
        movw            %ax, g_ramsize_hi            # contiguous RAM >1M in 1K blocks
detect_memory_LelacyLo:
        clc
        int             $0x12
        jc              _detect_memory_LegacylError
        movw            %ax, g_ramsize_lo            # contiguous RAM <1M in 1K blocks
        jz              _detect_memory_LegacylError
        ret     # TODO: set method bit

_detect_memory_LegacylError:
        leaw            s_memerr, %si
        call            bios_print
        jmp             idle_loop

##
# -----------------------------------------------------------------------------
# enable_a20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
###
enable_a20:
        call            test_a20
        jnc             _enable_a20_done
        call            enable_a20_Bios
        call            test_a20
        jnc             _enable_a20_done
        call            enable_a20_port92h
        call            test_a20
        jnc             _enable_a20_done
        call            enable_a20_keyboard
        call            test_a20
        jc              _enable_a20_error
_enable_a20_done:
        ret

_enable_a20_error:
        leaw            s_a20err, %si
        call            bios_print
        jmp             idle_loop

##
# Test whether A20 gate is enabled.
##
test_a20:
        // wbinvd                                  # invalidate cache (i486+)
        xorw            %ax, %ax
        movw            %ax, %es                # ds = 0x0000
        decw            %ax                     # fs = 0xFFFF
        movw            %ax, %fs
        rorw            $4, %ds:0x7DFE          # tweak the boot sector ID
        movw            %es:0x7DFE, %dx         # [0x00FDFE] -> dx
        movw            %fs:0x7E0E, %ax         # [0x10FDFE] -> ax
        cmpw            %ax, %dx                # are they the same?
        je              _test_a20_Off
        clc
        jmp             _test_a20_Done
_test_a20_Off:
        stc                                     # CF=1 if A20 disabled
_test_a20_Done:
        ret

##
# Enable A20 using BIOS INT=15h,AX=2401h.
# Supported on newer systems.
##
enable_a20_Bios:
        movw            $0x2401, %ax
        int             $0x15
        movw            $A20METHOD_BIOS, g_a20_method
        ret

##
# Enable A20 by writing port 92h.
# Not supported on all systems.
##
enable_a20_port92h:
        inb             $PORT_SYSCNTLA, %al
        orb             $0x02, %al          # A20 enable bit
        andb            $0xFE, %al          # clear (possible) reset flag
        outb            %al, $PORT_SYSCNTLA
        movw            $A20METHOD_PORT, g_a20_method
        ret

##
# Enable A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
enable_a20_keyboard:
        call            kbd_out_wait
        movb            $0xAD, %al              # cmd: disable keyboard
        outb            %al, $PORT_PS2KBD_CMD

        call            kbd_out_wait
        movb            $0xD0, %al              # cmd: read controller output port
        outb            %al, $PORT_PS2KBD_CMD

        call            kbd_in_wait
        inb             $PORT_PS2KBD_DATA, %al  # data: controller output port
        pushw           %ax                     # save output port

        call            kbd_out_wait
        movb            $0xD1, %al              # cmd: write controller output port
        outb            %al, $PORT_PS2KBD_CMD

        call            kbd_out_wait
        popw            %ax
        orb             $0x02, %al              # A20 enable bit
        outb            %al, $PORT_PS2KBD_DATA  # write output reg

        call            kbd_out_wait
        movb            $0xAE, %al              # cmd: enable keyboard
        outb            %al, $PORT_PS2KBD_CMD

        call            kbd_out_wait          # (delay)
        movw            $A20METHOD_KEYBOARD, g_a20_method
        ret

kbd_out_wait:
        inb             $PORT_PS2KBD_CMD, %al   # (buffer from controller perspective)
        testb           $0x02, %al              # is input buffer full?
        jnz             kbd_out_wait          # continue checking 'til empty (consumed)
        ret

kbd_in_wait:
        inb             $PORT_PS2KBD_CMD, %al   # (buffer from controller perspective)
        testb           $0x01, %al              # is output buffer full?
        jz              kbd_in_wait           # continue checking 'til data arrives
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

.align 4

.globl g_a20_method
g_a20_method:
        .long           0

.globl g_has_memory_map
g_has_memory_map:
        .long           0

.globl g_acpi_memory_map
g_acpi_memory_map:
        .long           MEMMAP_BASE

.globl g_ramsize_lo
g_ramsize_lo:
        .long           0

.globl g_ramsize_hi
g_ramsize_hi:
        .long           0

.globl g_ramsize_e801_lo
g_ramsize_e801_lo:
        .long           0

.globl g_ramsize_e801_hi
g_ramsize_e801_hi:
        .long           0

.globl g_hwflags
g_hwflags:
        .long           0

s_hello:
        .asciz          "boot: entering stage 2..."
s_memerr:
        .asciz          "Error: unable to detect system memory!"
s_a20err:
        .asciz          "Error: failed to enable A20 gate!"
