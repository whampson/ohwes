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
#      Created: Mar 21, 2023
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
TempGdt:
        .quad           0x0                     # (not used)
        .quad           0x00CF9A000000FFFF      # cs = 0x08
        .quad           0x00CF92000000FFFF      # ds = 0x10
TempGdtEnd:

        .word           0                       # (align)
TempGdtReg:
        .word           TempGdtEnd-TempGdt-1    # GDT limit
        .long           TempGdt                 # GDT base, aligned

# -----------------------------------------------------------------------------
# ---------------------------------- Stage2 -----------------------------------
# -----------------------------------------------------------------------------

##
# Stage 2 entry point.
##
.globl Stage2
Stage2:
        call            InitVideo
        leaw            s_Hello, %si
        call            Bios_PrintLn
        call            GetHwInfo
        call            DetectMemory
        call            EnableA20

_GoToProtectedMode:
        cli
        lgdtl           TempGdtReg
        movl            %cr0, %eax
        orl             $0x01, %eax     # Protected Mode enable bit
        movl            %eax, %cr0
        jmpl            $TEMP_CS, $_ProtectedModeEntry

.code32
_ProtectedModeEntry:
        movw            $TEMP_DS, %ax
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %ss
        xorw            %ax, %ax
        movw            %ax, %fs
        movw            %ax, %gs
        call            Init            # initialize devices and load kernel
_IdleLoop:
        jmp             _IdleLoop

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

.code16

##
# -----------------------------------------------------------------------------
# InitVideo
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine.
##
InitVideo:
        xorw            %ax, %ax
        movb            $VGA_MODE, %al
        int             $0x10
        ret
##
# -----------------------------------------------------------------------------
# GetHwInfo
# -----------------------------------------------------------------------------
# Gets installed hardware flags using a BIOS routine.
##
GetHwInfo:
        int             $0x11
        movw            %ax, g_EquipmentFlags
        ret

##
# -----------------------------------------------------------------------------
# DetectMemory
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines. If all methods fail, the system will display an error and halt.
##
DetectMemory:
        call            DetectMemory_e820
        call            DetectMemory_e801
        call            DetectMemory_Legacy
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
DetectMemory_e820:
        clc
        movw            $MEMMAP_BASE, %di               # dest buffer
        xorl            %ebx, %ebx                      # continuation value (0 to begin)
_DetectMemory_e820_Loop:
        movl            $0x18, %ecx                     # table entry size
        movl            $0x534D4150, %edx               # 'SMAP'
        movl            $0xE820, %eax                   # ACPI Query System Address Map function
        int             $0x15
        jc              _DetectMemory_e820_Done         # error or end of table
        cmpb            $0x80, %ah
        je              _DetectMemory_e820_Done         # invalid command
        cmpb            $0x86, %ah
        je              _DetectMemory_e820_Done         # unsupported function
        testl           %ebx, %ebx
        jz              _DetectMemory_e820_Done         # end of table
        addw            $0x18, %di                      # move table ptr
        cmpb            $0x14, %cl                      # if extended attr not present...
        je              _DetectMemory_e820_Loop
        movw            $0, -4(%edi)                    # ...ensure it is stored as zero
        jmp             _DetectMemory_e820_Loop
_DetectMemory_e820_Done:
        ret     # TODO: set method bit

####
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G.
#
# This function will work on all systems made after 1994. If detection fails,
# the function will return without error.
####
DetectMemory_e801:
        # data can be returned in ax/bx or cx/dx; initialize all to zero
        xorw            %ax, %ax
        xorw            %bx, %bx
        xorw            %cx, %cx
        xorw            %dx, %dx
        movw            $0xE801, %ax
        int             $0x15
        jcxz            DetectMemory_e801_Store         # cx = 0, use ax/bx
        movw            %cx, %ax
        movw            %dx, %bx
DetectMemory_e801_Store:
        movw            %ax, g_RamCapacityLo_e801       # contiguous RAM >1M in 1K blocks, up to 16M
        movw            %bx, g_RamCapacityHi_e801       # contiguous RAM >16M in 64K blocks
        ret     # TODO: set method bit

####
# Gets the number of contiguous 1K blocks of memory up to 15M or 64M, depending
# on the BIOS. If detection fails, the system will display an error and halt.
####
DetectMemory_Legacy:
        clc
        movb            $0x88, %ah
        int             $0x15
        jc              DetectMemory_LegacyLo
        cmpb            $0x80, %ah                      # (invalid command)
        je              DetectMemory_LegacyLo
        cmpb            $0x86, %ah                      # (unsupported func)
        je              DetectMemory_LegacyLo
        movw            %ax, g_RamCapacityHi            # contiguous RAM >1M in 1K blocks
DetectMemory_LegacyLo:
        clc
        int             $0x12
        jc              _DetectMemory_Legacy_Error
        movw            %ax, g_RamCapacityLo            # contiguous RAM <1M in 1K blocks
        jz              _DetectMemory_Legacy_Error
        ret     # TODO: set method bit

_DetectMemory_Legacy_Error:
        leaw            s_DetectMemoryError, %si
        call            Bios_Print
_DetectMemory_Legacy_ErrorLoop:
        jmp             _DetectMemory_Legacy_ErrorLoop

##
# -----------------------------------------------------------------------------
# EnableA20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
###
EnableA20:
        call            TestA20
        jnc             _EnableA20_Done
        call            EnableA20_Bios
        call            TestA20
        jnc             _EnableA20_Done
        call            EnableA20_Port92h
        call            TestA20
        jnc             _EnableA20_Done
        call            EnableA20_Keyboard
        call            TestA20
        jc              _EnableA20_Error
_EnableA20_Done:
        ret

_EnableA20_Error:
        leaw            s_A20Error, %si
        call            Bios_Print
_EnableA20_ErrorLoop:
        jmp             _EnableA20_ErrorLoop

##
# Test whether A20 gate is enabled.
##
TestA20:
        // wbinvd                                  # invalidate cache (i486+)
        xorw            %ax, %ax
        movw            %ax, %es                # ds = 0x0000
        decw            %ax                     # fs = 0xFFFF
        movw            %ax, %fs
        rorw            $4, %ds:0x7DFE          # tweak the boot sector ID
        movw            %es:0x7DFE, %dx         # [0x00FDFE] -> dx
        movw            %fs:0x7E0E, %ax         # [0x10FDFE] -> ax
        cmpw            %ax, %dx                # are they the same?
        je              _TestA20_Off
        clc
        jmp             _TestA20_Done
_TestA20_Off:
        stc                                     # CF=1 if A20 disabled
_TestA20_Done:
        ret

##
# Enable A20 using BIOS INT=15h,AX=2401h.
# Supported on newer systems.
##
EnableA20_Bios:
        movw            $0x2401, %ax
        int             $0x15
        movw            $A20METHOD_BIOS, g_A20Method
        ret

##
# Enable A20 by writing port 92h.
# Not supported on all systems.
##
EnableA20_Port92h:
        inb             $PORT_SYSCNTLA, %al
        orb             $0x02, %al          # A20 enable bit
        andb            $0xFE, %al          # clear (possible) reset flag
        outb            %al, $PORT_SYSCNTLA
        movw            $A20METHOD_PORT, g_A20Method
        ret

##
# Enable A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
EnableA20_Keyboard:
        call            WaitForKbd_Out
        movb            $0xAD, %al              # cmd: disable keyboard
        outb            %al, $PORT_PS2KBD_CMD

        call            WaitForKbd_Out
        movb            $0xD0, %al              # cmd: read controller output port
        outb            %al, $PORT_PS2KBD_CMD

        call            WaitForKbd_In
        inb             $PORT_PS2KBD_DATA, %al  # data: controller output port
        pushw           %ax                     # save output port

        call            WaitForKbd_Out
        movb            $0xD1, %al              # cmd: write controller output port
        outb            %al, $PORT_PS2KBD_CMD

        call            WaitForKbd_Out
        popw            %ax
        orb             $0x02, %al              # A20 enable bit
        outb            %al, $PORT_PS2KBD_DATA  # write output reg

        call            WaitForKbd_Out
        movb            $0xAE, %al              # cmd: enable keyboard
        outb            %al, $PORT_PS2KBD_CMD

        call            WaitForKbd_Out          # (delay)
        movw            $A20METHOD_KEYBOARD, g_A20Method
        ret

WaitForKbd_Out:
        inb             $PORT_PS2KBD_CMD, %al   # (buffer from controller perspective)
        testb           $0x02, %al              # is input buffer full?
        jnz             WaitForKbd_Out          # continue checking 'til empty (consumed)
        ret

WaitForKbd_In:
        inb             $PORT_PS2KBD_CMD, %al   # (buffer from controller perspective)
        testb           $0x01, %al              # is output buffer full?
        jz              WaitForKbd_In           # continue checking 'til data arrives
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

.align 4

.globl g_A20Method
g_A20Method:
        .long           0

.globl g_bHasMemoryMap
g_bHasMemoryMap:
        .long           0

.globl g_pAcpiMemoryMap
g_pAcpiMemoryMap:
        .long           MEMMAP_BASE

.globl g_RamCapacityLo
g_RamCapacityLo:
        .long           0

.globl g_RamCapacityHi
g_RamCapacityHi:
        .long           0

.globl g_RamCapacityLo_e801
g_RamCapacityLo_e801:
        .long           0

.globl g_RamCapacityHi_e801
g_RamCapacityHi_e801:
        .long           0

.globl g_EquipmentFlags
g_EquipmentFlags:
        .long           0

s_Hello:
        .asciz          "OH-WES is loading..."
s_DetectMemoryError:
        .asciz          "Error: unable to detect system memory!"
s_A20Error:
        .asciz          "Error: failed to enable A20 gate!"
