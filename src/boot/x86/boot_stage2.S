# =============================================================================
# Copyright (C) 2023 Wes Hampson. All Rights Reserved.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------
#        File: boot/x86/boot_stage2.S
#      Created: Mar 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode (except for jump to Protected Mode)
#
# Stage 2 boot loader. Initializes devices and loads the kernel.
# =============================================================================

#include "boot.h"

.text
.code16

#
# Stage 2 entry point.
#
.globl Stage2
Stage2:
        call            InitVideo
        leaw            s_Hello, %si
        call            Bios_PrintLn
        call            GetHwInfo
        call            GetRamInfo
        call            TryEnableA20

_LoadGdt:
        leal            GdtBase, %eax
        movl            %eax, GdtReg+2
        lgdtl           GdtReg

_GoToProtectedMode:
        cli
        movl            %cr0, %eax
        orl             $0x01, %eax         # Protected Mode enable
        movl            %eax, %cr0
        jmpl            $DUMMY_CS, $_ProtectedModeEntry

.code32
_ProtectedModeEntry:
        movw            $DUMMY_DS, %ax
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %ss
        call            Init
_IdleLoop:
        jmp             _IdleLoop

#
# Global Descriptor Table (GDT)
#
.align 16
GdtBase:
        .quad           0x0000000000000000      # (not used, required by CPU)
        .quad           0x00CF9A000000FFFF      # cs = 0x08
        .quad           0x00CF92000000FFFF      # ds = 0x10
GdtEnd:

.align 8
GdtReg:
        .word           GdtEnd - GdtBase - 1    # limit
        .long           0                       # base
        .word           0

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

.code16

# -----------------------------------------------------------------------------
# InitVideo
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine.
#
InitVideo:
        xorw            %ax, %ax
        movb            $0x03, %al  # 03h = text, CGA/EGA/VGA, 16 colors, B8000h
        int             $0x10
        ret

# -----------------------------------------------------------------------------
# GetHwInfo
# -----------------------------------------------------------------------------
# Gets installed hardware flags using a BIOS routine.
#
GetHwInfo:
        int             $0x11
        movw            %ax, g_EquipmentFlags
        ret

# -----------------------------------------------------------------------------
# GetRamInfo
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines.
#
GetRamInfo:
        int             $0x12
        movw            %ax, g_RamCapacity_Int12h
        ret
    # TODO: int 15h ah=88h, int 15h ax=e801h, int 15h ax=e820h

# -----------------------------------------------------------------------------
# TryEnableA20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
#
TryEnableA20:
        call            _TryEnableA20_Test
        jnc             _TryEnableA20_Done
        call            _TryEnableA20_Bios
        call            _TryEnableA20_Test
        jnc             _TryEnableA20_Done
        call            _TryEnableA20_Port92h
        call            _TryEnableA20_Test
        jnc             _TryEnableA20_Done
        call            _TryEnableA20_Keyboard
        call            _TryEnableA20_Test
        jc              _TryEnableA20_Error
_TryEnableA20_Done:
        ret

#
# Test whether A20 gate is enabled.
#
_TryEnableA20_Test:
        // wbinvd       # invalidate cache (i486+)
        xorw            %ax, %ax
        movw            %ax, %es            # ds = 0x0000
        decw            %ax                 # fs = 0xFFFF
        movw            %ax, %fs
        rorw            $4, %ds:0x7DFE      # tweak the boot sector ID
        movw            %es:0x7DFE, %dx     # [0x00FDFE] -> dx
        movw            %fs:0x7E0E, %ax     # [0x10FDFE] -> ax
        cmpw            %ax, %dx            # are they the same?
        je              _TryEnableA20_TestOff
        clc
        jmp             _TryEnableA20_TestDone
_TryEnableA20_TestOff:
        stc                                 # CF=1 if A20 disabled
_TryEnableA20_TestDone:
        ret

#
# Enable A20 using BIOS INT=15h,AX=0x2401.
#
_TryEnableA20_Bios:
        movw            $0x2401, %ax
        int             $0x15
        movw            $3, g_A20Method
        ret

#
# Enable A20 by writing port 92h.
#
_TryEnableA20_Port92h:
        inb             $0x92, %al
        orb             $0x02, %al          # A20 enable bit
        andb            $0xFE, %al          # clear (possible) reset flag
        outb            %al, $0x92
        movw            $2, g_A20Method
        ret

#
# Enable A20 via the PS/2 keyboard controller.
#
_TryEnableA20_Keyboard:
        call            _WaitFor8042_Cmd
        movb            $0xAD, %al          # cmd: disable keyboard
        outb            %al, $0x64

        call            _WaitFor8042_Cmd
        movb            $0xD0, %al          # cmd: read controller output port
        outb            %al, $0x64

        call            _WaitFor8042_Data
        inb             $0x60, %al          # data: controller output port
        pushw           %ax

        call            _WaitFor8042_Cmd
        movb            $0xD1, %al          # cmd: write controller output port
        outb            %al, $0x64

        call            _WaitFor8042_Cmd
        popw            %ax                 # data: prev output port
        orb             $0x02, %al          # A20 enable bit
        outb            %al, $0x60

        call            _WaitFor8042_Cmd
        movb            $0xAE, %al          # cmd: enable keyboard
        outb            %al, $0x64

        call            _WaitFor8042_Cmd    # empty 8042 input buffer (delay)
        movw            $1, g_A20Method
        ret

_WaitFor8042_Cmd:
        inb             $0x64, %al
        testb           $0x02, %al          # is input buffer full?
        jnz             _WaitFor8042_Cmd    # if yes, continue checking 'til empty
        ret

_WaitFor8042_Data:
        inb             $0x64, %al
        testb           $0x01, %al          # is output buffer full?
        jz              _WaitFor8042_Data   # if no, continue checking 'til data arrives
        ret

_TryEnableA20_Error:
        leaw            s_A20Error, %si
        call            Bios_Print
_TryEnableA20_ErrorLoop:
        jmp             _TryEnableA20_ErrorLoop

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

#       [0] - diskette drive installed
#       [1] - math coprocessor (x87) installed
#       [2] - PS/2 mouse installed
#       [3] - (unused)
#     [5:4] - initial video mode (00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono)
#     [7:6] - num diskette drives attached less 1
#       [8] - (unused) (previously DMA access installed, or DMA access NOT installed)
#    [11:9] - num serial ports installed
#      [12] - game port installed
#      [13] - (unused) (previously serial printer attached, or internal modem installed)
#   [15:14] - num parallel ports installed
.globl g_EquipmentFlags
g_EquipmentFlags:
        .word           0

.globl g_RamCapacity_Int12h
g_RamCapacity_Int12h:
        .word           0

#   0 = (already enabled)
#   1 = keyboard controller
#   2 = port 92h
#   3 = BIOS INT=15h,AX=0x2401
.globl g_A20Method
g_A20Method:
        .byte           0

s_Hello:
        .asciz          "OH-WES is loading..."
s_A20Error:
        .asciz          "Error: failed to enable A20 gate!"
