# =============================================================================
# Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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

#include <boot.h>
#include "x86_boot.h"

.text
.code16

// TODO: Test some of this stuff, like A20 methods, or flick thru video modes

# -----------------------------------------------------------------------------
# --------------------------------- Stage 2 -----------------------------------
# -----------------------------------------------------------------------------

##
# Stage 2 entry point.
##
.globl Stage2
Stage2:
        popl    %ecx                    # ...we made it!
        movl    %ecx, m_Stage2Size      # store stage2 size

        call    CollectBootParams
        call    InitConsole
        call    EnableA20
        call    DetectMemory

LoadKernel:
        leaw    sz_KernelFileName, %si  # locate the kernel image on disk
        call    FindFile                # returns: di = dir entry ptr
        movl    DIRENTRY_FILESIZE(%di), %ecx
        movl    %ecx, m_KernelSize      # store kernel size
        call    CheckKernelSize

        movw    $KERNEL_SEGMENT, %ax
        movw    %ax, %es
        movw    $KERNEL_OFFSET, %bx
        call    LoadFile                # load the kernel into low memory

SwitchToProtectedMode:
        cli                             # make sure interrupts are off
        lgdtl   GdtReg                  # load the temp GDT
        movl    %cr0, %eax
        orl     $CR0_PE, %eax           # set ProtectionEnable bit
        movl    %eax, %cr0
        ljmpl   $BOOT_CS, $Entry32      # reload CS and jump to 32-bit code

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~ Begin 32-bit Code Section ~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.code32

Entry32:
        # set up Protected Mode segment selectors and stack
        movw    $BOOT_DS, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %ss
        xorw    %ax, %ax
        movw    %ax, %fs
        movw    %ax, %gs

EnterKernel:
        movl    $KERNEL_BASE, m_KernelBase      # store the kernel base
        movl    m_KernelBase, %ebp              # set up a larger stack
        movl    %ebp, m_StackBase
        movl    %ebp, %esp
        movl    $g_BootParams, %ecx             # pass bootparams ptr to kernel
        ljmpl   $BOOT_CS, $KERNEL_ENTRY         # bye bye bootloader!

Halt32:
        hlt
        jmp     Halt32

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~ End 32-bit Code Section ~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.code16


# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------


##
# -----------------------------------------------------------------------------
# CollectBootParams
# -----------------------------------------------------------------------------
# Gets installed hardware flags using a BIOS routine.
##
CollectBootParams:
        xorl    %eax, %eax
        int     $0x11
        movw    %ax, m_HwFlags
        movl    $STAGE2_BASE, m_Stage2Base
        movzwl  %gs:BDA_EBDA_ADDR, %ebx
        shll    $4, %ebx
        movl    %ebx, m_pEbda
        ret


##
# -----------------------------------------------------------------------------
# CheckKernelSize
# -----------------------------------------------------------------------------
# Ensure the kernel will fit in low memory. To do this, we find the address of
# the Extended BIOS Data Area, which is effectively the top of low memory. We
# then calculate the available space by subtracting the early kernel base
# address from the EBDA address. Finally, we compare the kernel file size with
# the available space.
#
# We should have roughly 512k bytes available for the kernel. If we ever exceed
# this size (hopefully not), we'll have to load the kernel in stages, which
# would require a working floppy disk driver in Protected Mode (or make use of
# the BIOS again in Unreal Mode).
##
CheckKernelSize:
        movl    m_KernelSize, %ecx
        movl    $KERNEL_BASE, %eax
        cmpl    $0, m_pEbda
        jz      _NoEbda
        movl    m_pEbda, %ebx           # EBDA -> effective top of low memory
        jmp     _KernelSizeCheck
_NoEbda:
        movl    m_RamLo_Legacy, %ebx
        shll    $10, %ebx
_KernelSizeCheck:
        subl    %eax, %ebx              # Capacity = pTopOfMem - pKernelBase
        cmpl    %ecx, %ebx              # Capacity - KernelSize < 0?
        jl      _KernelTooBig           #   yep, fail
        ret

_KernelTooBig:
        PRINT   sz_KernelTooBig
        jmp     Halt


##
# -----------------------------------------------------------------------------
# InitConsole
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine and collects information about the
# console.
##
InitConsole:
        movb    $0x0F, %ah      # store video page
        int     $0x10
        jc      _VgaError
        movb    %bh, m_VideoPage

        movb    $0x03, %ah      # store cursor position (bh=page)
        int     $0x10
        jc      _VgaError
        movb    %dh, _m_CursorRow
        movb    %dl, _m_CursorColumn

        # VGA Stuff
        # See http://www.ctyme.com/intr/rb-0069.htm

        // VGA_MODE = 0x03 // 0x03 = text,CGA/EGA/VGA,16fg/8bg,0xB8000
        // VGA_CLEAR = 0   // clear screen toggle

        // movb    $0x00, %ah      # set video mode
        // movb    $VGA_MODE, %al
        // movb    $VGA_CLEAR, %dl
        // notb    %dl
        // shlb    $7, %dl
        // andb    $0x80, %dl
        // orb     %dl, %al        # bit7 is clear screen (0=clear)
        // int     $0x10
        // jc      _VgaError

        movb    $0x03, %ah      # gather and store new cursor size
        int     $0x10
        jc      _VgaError
        movb    %ch, m_CursorStartLine
        movb    %cl, m_CursorEndLine

        // movb    $VGA_CLEAR, %al
        // cmpb    $1, %al
        // je      _VgaDone
        xorw    %ax, %ax
        movb    $0x02, %ah      # restore cursor position if screen not cleared
        movb    m_VideoPage, %bh
        movb    _m_CursorRow, %dh
        movb    _m_CursorColumn, %dl
        int     $0x10
        jc      _VgaError

_VgaDone:
        movb    $0x0F, %ah      # gather and store video params
        int     $0x10
        jc      _VgaError
        movb    %al, m_VideoMode
        movb    %bh, m_VideoPage
        movb    %ah, m_VideoCols
        ret

_VgaError:
        PRINT   sz_VideoError
        jmp     Halt

_m_CursorRow:    .long   0
_m_CursorColumn: .long   0


##
# -----------------------------------------------------------------------------
# DetectMemory
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines. If all methods fail, the system will display an error and halt.
##
DetectMemory:
        call    DetectMemory_e820
        call    DetectMemory_e801
        call    DetectMemory_Legacy
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
        xorl    %eax, %eax
        movw    %ax, %es
        movw    $MEMMAP_BASE, %di       # dest buffer
        xorl    %ebx, %ebx              # continuation value (0 to begin)
_DetectMemory_e820_Loop:
        movl    $0x18, %ecx             # table entry size
        movl    $0x534D4150, %edx       # 'SMAP'
        movl    $0xE820, %eax           # ACPI Query System Address Map function
        int     $0x15
        jc      _DetectMemory_e820_Done # error or end of table
        cmpb    $0x80, %ah
        je      _DetectMemory_e820_Done # invalid command
        cmpb    $0x86, %ah
        je      _DetectMemory_e820_Done # unsupported function
        testl   %ebx, %ebx
        jz      _DetectMemory_e820_End  # end of table
        addw    $0x18, %di              # move table ptr
        cmpb    $0x14, %cl              # if extended attr not present...
        je      _DetectMemory_e820_Loop
        movw    $0, -4(%edi)            # ...ensure it is stored as zero
        jmp     _DetectMemory_e820_Loop
_DetectMemory_e820_End:
        movl    $MEMMAP_BASE, m_pMemoryMap

_DetectMemory_e820_Done:
        ret

####
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G.
#
# This function should work on BIOSes made after 1994.
####
DetectMemory_e801:
        # data can be returned in ax/bx or cx/dx; initialize all to zero
        xorw    %ax, %ax
        xorw    %bx, %bx
        xorw    %cx, %cx
        xorw    %dx, %dx
        movw    $0xE801, %ax
        int     $0x15
        jc      _DetectMemory_e801_Done  # unsupported function
        jcxz    _DetectMemory_e801_Store # cx = 0, use ax/bx
        movw    %cx, %ax
        movw    %dx, %bx
_DetectMemory_e801_Store:
        movw    %ax, m_RamLo_E801h  # contiguous RAM >1M in 1K blocks, up to 16M
        movw    %bx, m_RamHi_E801h  # contiguous RAM >16M in 64K blocks
_DetectMemory_e801_Done:
        ret

####
# Gets the number of contiguous 1K blocks from 0 to 640K, and from 1M up to 15M
# or 64M, depending on the BIOS. If detection fails, the system will display
# an error and halt. This detection method should be supported on every 386
# BIOS, so if this fails, either something is seriouly wrong or the system
# probably does not have enough memory to run this OS!
####
DetectMemory_Legacy:
        clc
        movb    $0x88, %ah
        int     $0x15
        jc      _DetectMemory_Legacy_Lo
        cmpb    $0x80, %ah                     # (invalid command)
        je      _DetectMemory_Legacy_Lo
        cmpb    $0x86, %ah                     # (unsupported func)
        je      _DetectMemory_Legacy_Lo
        movw    %ax, m_RamHi_Legacy            # contiguous RAM >1M in 1K blocks

_DetectMemory_Legacy_Lo:
        clc
        int     $0x12
        jc      _DetectMemory_Legacy_Error
        movw    %ax, m_RamLo_Legacy            # contiguous RAM <1M in 1K blocks
        jz      _DetectMemory_Legacy_Error
        ret

_DetectMemory_Legacy_Error:
        PRINT   sz_MemError
        jmp     Halt

##
# -----------------------------------------------------------------------------
# EnableA20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
###
EnableA20:
        call    TestA20
        jnc     _EnableA20_Done
        call    _EnableA20_Bios
        call    TestA20
        jnc     _EnableA20_Done
        call    _EnableA20_Port92h
        call    TestA20
        jnc     _EnableA20_Done
        call    _EnableA20_Keyboard
        call    TestA20
        jc      _EnableA20_Error
_EnableA20_Done:
        ret

_EnableA20_Error:
        PRINT   sz_A20Error
        jmp     Halt

##
# Test whether A20 gate is enabled.
##
TestA20:
        xorw    %ax, %ax
        movw    %ax, %es                # ds = 0x0000
        decw    %ax                     # fs = 0xFFFF
        movw    %ax, %fs
        rorw    $4, %ds:0x7DFE          # tweak the boot sector ID
        movw    %es:0x7DFE, %dx         # [0x00FDFE] -> dx
        movw    %fs:0x7E0E, %ax         # [0x10FDFE] -> ax
        cmpw    %ax, %dx                # are they the same?
        je      _TestA20_Off
        clc
        jmp     _TestA20_Done
_TestA20_Off:
        stc                             # CF=1 if A20 disabled
_TestA20_Done:
        ret

##
# Enable A20 using BIOS INT=15h,AX=2401h.
# Supported on newer systems.
##
_EnableA20_Bios:
        movw    $0x2401, %ax
        int     $0x15
        movw    $A20_BIOS, m_A20Method
        ret

##
# Enable A20 by writing port 92h.
# Not supported on all systems.
##
_EnableA20_Port92h:
        inb     $0x92, %al
        orb     $0x02, %al          # A20 enable bit
        andb    $0xFE, %al          # clear (possible) reset flag
        outb    %al, $0x92
        movw    $A20_FAST, m_A20Method
        ret

##
# Enable A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
_EnableA20_Keyboard:
        call    KbOut_Wait
        movb    $0xAD, %al      # cmd: disable keyboard
        outb    %al, $0x64

        call    KbOut_Wait
        movb    $0xD0, %al      # cmd: read controller output port
        outb    %al, $0x64

        call    KbIn_Wait
        inb     $0x60, %al      # data: controller output port
        pushw   %ax             # save output port

        call    KbOut_Wait
        movb    $0xD1, %al      # cmd: write controller output port
        outb    %al, $0x64

        call    KbOut_Wait
        popw    %ax
        orb     $0x02, %al      # A20 enable bit
        outb    %al, $0x60      # write output reg

        call    KbOut_Wait
        movb    $0xAE, %al      # cmd: enable keyboard
        outb    %al, $0x64

        call    KbOut_Wait      # (delay)
        movw    $A20_KEYBOARD, m_A20Method
        ret

KbOut_Wait:
        movw    $1000, %cx
_KbOut_Wait_Loop:
        inb     $0x64, %al      # (from controller perspective)
        testb   $0x02, %al      # is input buffer full?
        jz      _KbOut_Wait_Done #  yes: continue checking data arrives
        decw    %cx
        jnz     _KbOut_Wait_Loop
_KbOut_Wait_Done:
        ret

KbIn_Wait:
        movw    $1000, %cx
_KbIn_Wait_Loop:
        inb     $0x64, %al      # (from controller perspective)
        testb   $0x01, %al      # is output buffer full?
        jnz     _KbIn_Wait_Done #   yes: continue checking 'til empty (consumed)
        decw    %cx
        jnz     _KbIn_Wait_Loop
_KbIn_Wait_Done:
        ret

##
# -----------------------------------------------------------------------------
# Halt
# -----------------------------------------------------------------------------
# Puts the system into an idle loop.
##
.globl Halt
Halt:
        hlt
        jmp     Halt


# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

##
# Temporary GDT so we can get into Protected Mode.
##
.align 8
EarlyGdt:
        .quad   0x0                     # (not used)
        .quad   0x00CF9A000000FFFF      # cs = 0x08
        .quad   0x00CF92000000FFFF      # ds = 0x10
EarlyGdtEnd:

        .word   0   # (align)
GdtReg:
GdtLimit:
        .word   EarlyGdtEnd-EarlyGdt-1
GdtBase:
        .long   EarlyGdt


##
# struct BootParams
##
.align 4
.globl g_BootParams
g_BootParams:
  m_KernelBase:         .long   0
  m_KernelSize:         .long   0
  m_Stage2Base:         .long   0
  m_Stage2Size:         .long   0
  m_StackBase:          .long   0

  m_RamLo_Legacy:       .long   0
  m_RamHi_Legacy:       .long   0
  m_RamLo_E801h:        .long   0
  m_RamHi_E801h:        .long   0
  m_pMemoryMap:         .long   0

  m_HwFlags:            .long   0
  m_A20Method:          .long   A20_NONE
  m_VideoMode:          .long   0
  m_VideoPage:          .long   0
  m_VideoCols:          .long   0
  m_CursorStartLine:    .long   0
  m_CursorEndLine:      .long   0

  m_pEbda:              .long   0


##
# Strings
##
sz_KernelFileName:
        .asciz          "OHWES   SYS"
sz_MemError:
        .asciz          "\r\nMemory detection error!"
sz_VideoError:
        .asciz          "\r\nVideo initialization error!"
sz_A20Error:
        .asciz          "\r\nA20 not enabled!"
sz_Stage2TooBig:
        .asciz          "\r\nStage 2 too big!"
sz_KernelTooBig:
        .asciz          "\r\nKernel too big!"
