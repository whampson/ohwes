# =============================================================================
# Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
#         File: src/i386/boot/stage2.S
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

        # setup text mode VGA
        call            init_vga

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

        # load GDT and switch to Protected Mode
        cli                                     # no more interrupts!
        lgdt            early_gdt_desc          # load temp GDT
        movl            %cr0, %eax
        orl             $CR0_PE, %eax           # enable Protected Mode
        movl            %eax, %cr0

        # set up boot segment selectors
        movw            $BOOT_DS, %ax
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %ss

        # long jump to the kernel setup
        leal            boot_info, %ecx         # pass bootparams ptr to kernel
        movl            $KERNEL_BASE, %ebx
        movl            0(%ebx), %ebx           # load kernel entry point addr
        pushl           $BOOT_CS
        pushl           %ebx
        ljmpl           *(%esp)                 # long jump to 32-bit kernel

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# init_vga
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine and collects information about the
# console.
##
init_vga:

        # store cursor position
        xorb            %bh, %bh                # page 0
        movb            $0x03, %ah
        int             $0x10
        jc              _vga_error
        movb            %dh, tmp_cursor_row
        movb            %dl, tmp_cursor_col

        # set video mode
        movb            $VGA_MODE, %al
        orb             $0x80, %al              # don't clear screen
        movb            $0x00, %ah
        int             $0x10
        jc              _vga_error_bad_mode

        # set font size
        movw            $VGA_FONT, %ax
_font0: cmpw            $_VGA_FONT_8x14, %ax
        jne             _font1
        jmp             _dofont
_font1: cmpw            $_VGA_FONT_8x8, %ax
        jne             _font2
        jmp             _dofont
_font2: cmpw            $_VGA_FONT_8x16, %ax
        jne             _vga_error_bad_font
_dofont:addw            $0x1110, %ax
        xorb            %bl, %bl
        int             $0x10
        jc              _vga_error_bad_font

        # set video page
        xorb            %al, %al                # page 0
        movb            $0x05, %ah
        int             $0x10
        jc              _vga_error

        # gather and store video info for kernel
        movb            $0x0F, %ah
        int             $0x10
        jc              _vga_error
        andb            $0x7F, %al
        movb            %al, vga_mode
        xorb            %bh, %bh
        movb            $0x03, %ah
        int             $0x10
        jc              _vga_error

        # restore cursor position
        xorw            %ax, %ax
        movb            tmp_cursor_row, %dh
        movb            tmp_cursor_col, %dl
        movb            $0x02, %ah
        int             $0x10
        jc              _vga_error

_vga_done:
        ret

_vga_error_bad_mode:
        PRINT           sz_video_error_bad_mode
        jmp             readkey_then_reset

_vga_error_bad_font:
        PRINT           sz_video_error_bad_font
        jmp             readkey_then_reset

_vga_error:
        PRINT           sz_video_error
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
        PRINT           sz_memory_error
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
        movw            %ax, %es                # es = 0x0000
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
        jc              _enable_a20_bios_done   # error
        cmpb            $0x86, %ah              # function not supported
        je              _enable_a20_bios_done
        movw            $A20_BIOS, a20_method
_enable_a20_bios_done:
        ret

##
# Enable A20 by writing port 92h (System Control Port A).
# Not supported on all systems.
##
_enable_a20_port_92h:
        inb             $0x92, %al
        orb             $0x02, %al              # A20 enable bit
        andb            $0xFE, %al              # ensure potential reset flag cleared
        outb            %al, $0x92              # send it
        movw            $A20_PORT92, a20_method
        ret

##
# Enable A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
_enable_a20_keyboard:
        call            kb_flush
        movb            $0xD1, %al              # cmd: write controller output port
        outb            %al, $0x64
        call            kbout_wait              # wait for controller ready
        movb            $0xFF, %al              # A20 enable bit
        outb            %al, $0x60              # write output reg
        call            kb_flush
        movw            $A20_KEYBOARD, a20_method
        ret

kbout_wait:
        inb             $0x64, %al
        testb           $0x02, %al
        jz              _kbout_wait_done
        jmp             kbout_wait
_kbout_wait_done:
        ret

kb_flush:
        call            kbout_wait
_kb_flush_loop:
        inb             $0x64, %al
        testb           $0x01, %al
        jz              _kb_flush_done
        inb             $0x60
        jmp             _kb_flush_loop
_kb_flush_done:
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
        movl            $KERNEL_BASE, %eax
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

tmp_cursor_row:         .long   0
tmp_cursor_col:         .long   0

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
sz_memory_error:
        .asciz          "\r\nMemory error!"
sz_video_error_bad_mode:
        .asciz          "\r\nInvalid VGA mode!"
sz_video_error_bad_font:
        .asciz          "\r\nInvalid VGA font!"
sz_video_error:
        .asciz          "\r\nVGA initialization error!"
sz_a20_error:
        .asciz          "\r\A20 error!"
sz_kernel_too_big:
        .asciz          "\r\nKernel image too big!"
