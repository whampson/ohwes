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
#  Environment: 16-bit Real Mode
#
# Stage 2 boot loader. This code is loaded by stage 1 and is responsible for
# gathering as much information about the system as possible from the BIOS,
# before loading the kernel image before switching into the CPU into 32-bit
# Protected Mode. It then performs minimal Protected Mode CPU initialization
# before jumping to the kernel entry point, passing along the collected system
# information in a boot_info struct.
# =============================================================================

#include <boot.h>
#include "x86_boot.h"

.code16

# -----------------------------------------------------------------------------
# --------------------------------- Stage 2 -----------------------------------
# -----------------------------------------------------------------------------
.section .stage2, "ax", @progbits

##
# Stage 2 entry point. We land here after jumping from stage 1.
##
.globl stage2
stage2:
        # collect some hardware info
        xorl            %eax, %eax
        int             $0x11
        movw            %ax, hwflags
        movzwl          %gs:BDA_EBDA_ADDR, %ebx
        shll            $4, %ebx
        movl            %ebx, ebda_base

        # detect memory
        call            enable_a20
        call            detect_memory

        # load the kernel image
        leaw            sz_kernel_filename, %si
        call            find_file               # returns: di = dir entry ptr
        movl            FILESIZE(%di), %ecx
        call            check_kernel_size       # make sure it fits in low mem
        LOAD_SEGMENT    $KERNEL_SEGMENT, %es
        xorl            %ebx, %ebx
        call            load_file               # load it!

        # setup console
        call            init_early_console

        # load GDT and switch to Protected Mode
        cli                                     # no more interrupts!
        lgdt            early_gdt_desc           # load temp GDT
        movl            %cr0, %eax
        orl             $CR0_PE, %eax           # enable Protected Mode
        movl            %eax, %cr0

        # set up boot segment selectors
        movw            $BOOT_DS, %ax
        movw            %ax, %ds
        movw            %ax, %es

        # long jump to the kernel setup
        leal            boot_info, %ecx         # pass bootparams ptr to kernel
        ljmpl           $BOOT_CS, $KERNEL_LMA   # reload CS, jump to 32-bit code

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# init_early_console
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine and collects information about the
# console.
##
init_early_console:
        # store cursor position (bh=page)
        xorb            %bh, %bh
        movb            $0x03, %ah
        int             $0x10
        jc              _vga_error
        movb            %dh, cursor_row
        movb            %dl, cursor_col

        # set video mode
        movb            $VGA_MODE, %al
        orb             $0x80, %al              # don't clear screen
        movb            $0x00, %ah
        int             $0x10
        jc              _vga_error

        # set video page
        xorb            %al, %al                # page 0
        movb            $0x05, %ah
        int             $0x10
        jc              _vga_error

        # restore cursor position
        xorw            %ax, %ax
        movb            cursor_row, %dh
        movb            cursor_col, %dl
        movb            $0x02, %ah
        int             $0x10
        jc              _vga_error

        # gather and store video params
        movb            $0x0F, %ah
        int             $0x10
        jc              _vga_error
        movb            %al, vga_mode
        movb            %ah, vga_cols

_vga_done:
        ret

_vga_error:
        PRINT           sz_VideoError
        jmp             readkey_then_reset

##
# -----------------------------------------------------------------------------
# detect_memory
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines. If all methods fail, the system will display an error and halt.
##
detect_memory:
        call            _detectmem_e820h        # newest method (2002+)
        call            _detectmem_e801h        # newer method  (1994+)
        call            _detectmem_legacy       # oldest method (all 386s)
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
_detectmem_e820h:
        clc
        xorl            %eax, %eax
        movw            %ax, %es
        movw            $MEMMAP_BASE, %di       # dest buffer
        xorl            %ebx, %ebx              # continuation value (0 to begin)

_detectmem_e820h_loop:
        movl            $0x18, %ecx             # table entry size
        movl            $0x534D4150, %edx       # 'SMAP'
        movl            $0xE820, %eax           # ACPI Query System Address Map function
        int             $0x15
        jc              _detectmem_e820h_done   # error or end of table
        cmpb            $0x80, %ah
        je              _detectmem_e820h_done   # invalid command
        cmpb            $0x86, %ah
        je              _detectmem_e820h_done   # unsupported function
        testl           %ebx, %ebx
        jz              _detectmem_e820h_end    # end of table
        addw            $0x18, %di              # move table ptr
        cmpb            $0x14, %cl              # if extended attr not present...
        je              _detectmem_e820h_loop
        movw            $0, -4(%edi)            # ...ensure it is stored as zero
        jmp             _detectmem_e820h_loop

_detectmem_e820h_end:
        movl            $MEMMAP_BASE, mem_map

_detectmem_e820h_done:
        ret

####
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G.
#
# This function should work on BIOSes made after 1994.
####
_detectmem_e801h:
        # data can be returned in ax/bx or cx/dx; initialize all to zero
        xorw            %ax, %ax
        xorw            %bx, %bx
        xorw            %cx, %cx
        xorw            %dx, %dx
        movw            $0xE801, %ax
        int             $0x15
        jc              _detectmem_e801h_done   # unsupported function
        jcxz            _detectmem_e801h_store  # cx = 0, use ax/bx
        movw            %cx, %ax
        movw            %dx, %bx
_detectmem_e801h_store:
        movw            %ax, kb_high_e801h      # contig RAM >1M in 1K blocks, up to 16M
        movw            %bx, kb_extended        # contig RAM >16M in 64K blocks
_detectmem_e801h_done:
        ret

####
# Gets the number of contiguous 1K blocks from 0 to 640K, and from 1M up to 15M
# or 64M, depending on the BIOS. If detection fails, the system will display
# an error and halt. This detection method should be supported on every 386
# BIOS, so if this fails, either something is seriouly wrong or the system
# probably does not have enough memory to run this OS!
####
_detectmem_legacy:
        clc
        movb            $0x88, %ah
        int             $0x15
        jc              _detectmem_legacy_lo
        cmpb            $0x80, %ah              # (invalid command)
        je              _detectmem_legacy_lo
        cmpb            $0x86, %ah              # (unsupported func)
        je              _detectmem_legacy_lo
        movw            %ax, kb_high            # contiguous RAM >1M in 1K blocks

_detectmem_legacy_lo:
        clc
        int             $0x12
        jc              _detectmem_legacy_error
        movw            %ax, kb_low             # contiguous RAM <1M in 1K blocks
        jz              _detectmem_legacy_error # fail if it's zero!
        ret

_detectmem_legacy_error:
        PRINT           sz_MemError
        jmp             readkey_then_reset

##
# -----------------------------------------------------------------------------
# enable_a20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
###
enable_a20:
        call            _test_a20
        jnc             _enable_a20_done
        call            _enable_a20_bios
        call            _test_a20
        jnc             _enable_a20_done
        call            _enable_a20_port_92h
        call            _test_a20
        jnc             _enable_a20_done
        call            _enable_a20_keyboard
        call            _test_a20
        jc              _enable_a20_error
_enable_a20_done:
        ret

_enable_a20_error:
        PRINT           sz_a20_error
        jmp             readkey_then_reset

##
# Test whether A20 gate is enabled.
##
_test_a20:
        xorw            %ax, %ax
        movw            %ax, %es                # ds = 0x0000
        decw            %ax                     # fs = 0xFFFF
        movw            %ax, %fs
        rorw            $4, %ds:0x7DFE          # tweak the boot sector ID
        movw            %es:0x7DFE, %dx         # [0x00FDFE] -> dx
        movw            %fs:0x7E0E, %ax         # [0x10FDFE] -> ax
        cmpw            %ax, %dx                # are they the same?
        je              _test_a20_off
        clc
        jmp             _test_a20_done
_test_a20_off:
        stc                                     # CF=1 if A20 disabled
_test_a20_done:
        ret

##
# Enable A20 using BIOS INT=15h,AX=2401h.
# Supported on newer systems.
##
_enable_a20_bios:
        movw            $0x2401, %ax
        int             $0x15
        movw            $A20_BIOS, a20_method
        ret

##
# Enable A20 by writing port 92h.
# Not supported on all systems.
##
_enable_a20_port_92h:
        inb             $0x92, %al
        orb             $0x02, %al              # A20 enable bit
        andb            $0xFE, %al              # ensure possible reset flag cleared
        outb            %al, $0x92
        movw            $A20_FAST, a20_method
        ret

##
# Enable A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
_enable_a20_keyboard:
        call            kbout_wait
        movb            $0xAD, %al              # cmd: disable keyboard
        outb            %al, $0x64

        call            kbout_wait
        movb            $0xD0, %al              # cmd: read controller output port
        outb            %al, $0x64

        call            kbin_wait
        inb             $0x60, %al              # data: controller output port
        pushw           %ax                     # save output port

        call            kbout_wait
        movb            $0xD1, %al              # cmd: write controller output port
        outb            %al, $0x64

        call            kbout_wait
        popw            %ax
        orb             $0x02, %al              # A20 enable bit
        outb            %al, $0x60              # write output reg

        call            kbout_wait
        movb            $0xAE, %al              # cmd: enable keyboard
        outb            %al, $0x64

        call            kbout_wait              # (delay)
        movw            $A20_KEYBOARD, a20_method
        ret

WAIT_DELAY = 1000

kbout_wait:
        movw            $WAIT_DELAY, %cx
_kbout_wait_loop:
        inb             $0x64, %al              # (from controller perspective)
        testb           $0x02, %al              # is input buffer full?
        jz              _kbout_wait_done        #  yes: continue checking data arrives
        decw            %cx
        jnz             _kbout_wait_loop
_kbout_wait_done:
        ret

kbin_wait:
        movw            $WAIT_DELAY, %cx
_kbin_wait_loop:
        inb             $0x64, %al              # (from controller perspective)
        testb           $0x01, %al              # is output buffer full?
        jnz             _kbin_wait_done         #   yes: continue checking 'til empty (consumed)
        decw            %cx
        jnz             _kbin_wait_loop
_kbin_wait_done:
        ret


##
# -----------------------------------------------------------------------------
# check_kernel_size
# -----------------------------------------------------------------------------
# Ensure the kernel will fit in low memory. To do this, we find the address of
# the Extended BIOS Data Area, which is effectively the top of low memory. We
# then calculate the available space by subtracting the early kernel base
# address from the EBDA address. Finally, we compare the kernel file size with
# the available space.
#
# We should have roughly 574k bytes available for the kernel. If we ever exceed
# this size (hopefully not), we'll have to load the kernel in stages, which
# would require a working floppy disk driver in Protected Mode (or make use of
# the BIOS again in Unreal Mode).
#
# Inputs:
#       ecx - kernel image size
##
check_kernel_size:
        movl            $KERNEL_LMA, %eax
        cmpl            $0, ebda_base
        jz              _no_ebda_info
        movl            ebda_base, %ebx # EBDA = effective top of low memory
        jmp             _size_check

_no_ebda_info:
        movl            kb_low, %ebx    # no EBDA? use the kb_low field instead
        shll            $10, %ebx       # stored as 1K blocks, multiply by 1024

_size_check:
        subl            %eax, %ebx      # Capacity = pTopOfMem - pKernelBase
        cmpl            %ecx, %ebx      # Capacity - KernelSize < 0?
        jl              _kernel_too_big #   yep, fail

_size_check_done:
        ret

_kernel_too_big:
        PRINT           sz_kernel_too_big
        jmp             readkey_then_reset


# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

# keep this in-line with struct boot_info in include/boot.h!
.globl boot_info
boot_info:
  ebda_base:            .long   0
  mem_map:              .long   0
  kb_low:               .long   0
  kb_high:              .long   0
  kb_high_e801h:        .long   0
  kb_extended:          .long   0
  hwflags:              .long   0
  a20_method:           .long   A20_NONE
  vga_mode:             .long   0
  vga_cols:             .long   0
  cursor_row:           .long   0
  cursor_col:           .long   0

# -----------------------------------------------------------------------------

# Temporary GDT
.align 8
_early_gdt:
        .quad           0                       # (must be 0)
        .quad           0x00CF9A000000FFFF      # [0x08] BOOT_CS
        .quad           0x00CF92000000FFFF      # [0x10] BOOT_DS
_early_gdt_end:

        .word           0                       # align
early_gdt_desc:
        .word           _early_gdt_end - _early_gdt - 1
        .long           _early_gdt

# -----------------------------------------------------------------------------

sz_kernel_filename:
        .asciz          "OHWES   SYS"
sz_MemError:
        .asciz          "\r\nMemory error!"
sz_VideoError:
        .asciz          "\r\nVideo error!"
sz_a20_error:
        .asciz          "\r\Could not enable A20!"
sz_kernel_too_big:
        .asciz          "\r\nKernel image too big!"
