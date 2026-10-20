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
#         File: i386/boot/stage2.S
#      Created: March 21, 2023
#       Author: Wes Hampson
#  Environment: 16-bit Real Mode
#               32-bit Protected Mode (jump to kernel only)
#
# Stage 2 of the boot loader. This code is loaded by stage 1 and is primarily
# responsible for loading the kernel image into memory, switching the CPU into
# 32-bit Protected Mode, then transferring control to the kernel.
#
# Prior to loading the kernel, stage 2 gathers as much information as possible
# about the current hardware from the BIOS. The BIOS routines are unavailable in
# Protected Mode, so it's important to grab everything now to avoid having to
# switch back-and-forth between Real and Protected Mode, or worse: write our own
# hardware-scraping routines. The information gathered is stored in a boot_info
# struct, a pointer to which is passed as a parameter to the kernel entry point.
#
# The kernel image is initially loaded in low memory (below 640K) because we
# use BIOS routines to access the disk, which are incapable of addressing memory
# above 1M. As a result, this boot loader can load a kernel image of up to
# approx. 576K, give or take a few kilobytes for reserved memory and this code.
# If we ever hit this limit, we have a couple of options: 1) give the kernel
# its own "stage 3" boot strapper that's capable of reading the boot drive in
# Protected Mode, or 2) load the kernel in chunks, switching to Real Mode to
# access the disk for each chunk and back to Protected mode to copy each chunk
# into high memory.
# =============================================================================

#include "bootldr.h"

# =============================================================================
# --------------------------------- Stage 2 -----------------------------------
# =============================================================================
.section .stage2, "ax", @progbits
.code16

# Boot Loader GDT Segment Selectors
BOOT_CS = 0x08
BOOT_DS = 0x10

# -----------------------------------------------------------------------------

##
# Boot loader stage 2 entry point. We land here after jumping from stage 1.
# The size of the stage 2 program code is passed as a 32-bit value on the stack.
#
# Inputs:
#       [sp-4] - size of stage 2
##
.globl stage2
stage2:
        popl        %ecx                    # pop stage 2 size
        movl        %ecx, lStage2Size       # store it!
        call        stage2_sanity_check     # ensure we loaded correctly
        call        collect_bios_hwinfo     # gather generic hardware info
        call        setup_video             # init VGA and collect video info
        call        detect_memory           # find out how much RAM we have

        # load the kernel image
        leaw        sKernelFileName, %si
        call        find_file               # returns: di = dir entry ptr
        movl        DIRENTRY_FILESIZE(%di), %ecx
        movl        %ecx, bBootInfo+BI_KERNEL_SIZE
        call        check_kernel_size       # make sure it fits in low mem
        movw        $KERNEL_SEGMENT, %ax
        movw        %ax, %es                # es = kernel segment
        xorw        %bx, %bx                # es:bx = kernel address
        call        load_file               # load it!

        # load GDT and switch to Protected Mode
        cli
        lgdt        GdtDesc
        movl        %cr0, %eax
        orl         $CR0_PE, %eax
        movl        %eax, %cr0
        ljmpl       $BOOT_CS, $_continue

# -----------------------------------------------------------------------------
# 32-bit code
.code32

_continue:
        # set up boot segment selectors
        movw        $BOOT_DS, %ax
        movw        %ax, %ds
        movw        %ax, %es
        movw        %ax, %ss

        # move the kernel to its new home above 1M
        movl        $KERNEL_LOAD, %esi
        movl        $KERNEL_BASE, %edi
        movl        bBootInfo+BI_KERNEL_SIZE, %ecx
        cld
    rep movsb

        # long jump to the kernel setup
        leal        bBootInfo, %ecx         # pass boot info ptr to kernel
        movl        $KERNEL_BASE, %ebx
        movl        0(%ebx), %ebx           # load kernel entry point addr
        jmpl        *%ebx                   # indirect jump to kernel

# End 32-bit code
# -----------------------------------------------------------------------------
.code16

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# stage2_sanity_check
# -----------------------------------------------------------------------------
# Make sure we loaded all of stage 2 into memory.
#
# Inputs:
#       ecx - stage 2 size
##
stage2_sanity_check:
        leal        stage2, %ebx            # get stage2 base address
        movl        -4(%ebx, %ecx, 1), %eax # [magic] = base + size - 4
        cmpl        $STAGE2_MAGIC, %eax     # valid magic?
        jne         _sanity_check_fail
        rorl        $4, -4(%ebx, %ecx, 1)   # modify magic so it doesn't
        ret                                 #   stay resident on next boot

_sanity_check_fail:
        PRINT       szBadMagic
        jmp         readkey_then_reset

##
# -----------------------------------------------------------------------------
# collect_bios_hwinfo
# -----------------------------------------------------------------------------
# Collects the BIOS equipment list and store the base address of the Extended
# BIOS Data Area. See https://stanislavs.org/helppc/bios_data_area.html.
##
collect_bios_hwinfo:
        # BIOS Data Area Fields
        BDA_EBDA_SEGMENT = 0x0E
        BDA_EQUIP_LIST   = 0x10

        movw        $BDA_SEGMENT, %ax
        movw        %ax, %gs
        movw        %gs:BDA_EQUIP_LIST, %ax
        movw        %ax, bBootInfo+BI_HWFLAGS            # same as INT-11h
        movzwl      %gs:BDA_EBDA_SEGMENT, %ebx
        shll        $4, %ebx
        movl        %ebx, bBootInfo+BI_EBDA_BASE         # addr = (seg << 4)
        ret

##
# -----------------------------------------------------------------------------
# setup_video
# -----------------------------------------------------------------------------
# Sets the VGA mode using a BIOS routine then collects information about the
# VGA state.
##
setup_video:
#if CONFIG_VGA_MODE
        xorw        %ax, %ax
        movb        $0x0F, %ah              # 0Fh = get current video mode
        int         $0x10                   #   ret: bh=page, al=mode, ah=cols
        cmpb        $CONFIG_VGA_MODE, %al
        je          _store_video_mode
        movb        $0x03, %ah              # 03h = save cursor position
        int         $0x10                   #   bh=page; ret: dh=row, dl=col
        pushw       %dx
        xorw        %ax, %ax                # 00h = set video mode
        movb        $CONFIG_VGA_MODE, %al   #   al=mode
        orb         $0x80, %al              #      set "no blanking" bit
        int         $0x10
        popw        %dx
        movb        $0x02, %ah              # 02h = set cursor position
        int         $0x10                   #   dh=row, dl=col
#endif
        movb        $0x0F, %ah              # 0Fh = get current video mode
        int         $0x10
        andb        $0x7F, %al              # clear "no blanking" bit
#if CONFIG_VGA_MODE
        cmpb       $CONFIG_VGA_MODE, %al    # compare readback if mode set
        jne        _badmode
#endif

_store_video_mode:
        movb        %al, bBootInfo+BI_VGA_MODE
        movb        %ah, bBootInfo+BI_VGA_COLS
        movb        %bh, bBootInfo+BI_VGA_PAGE

#if CONFIG_VGA_FONT
        movw        $CONFIG_VGA_FONT, %ax
_font0: cmpw        $0x1, %ax           # 1: 8x14,28    INT-10h,AX=1111h
        jne         _font1
        jmp         _dofont
_font1: cmpw        $0x2, %ax           # 2:  8x8,50    INT-10h,AX=1112h
        jne         _font2
        jmp         _dofont
_font2: cmpw        $0x4, %ax           # 4: 8x16,25    INT-10h,AX=1114h
        jne         _badfont
_dofont:addw        $0x1110, %ax        # 1110h = load font ROM functions
        xorb        %bl, %bl            #   param: bl = 0
        int         $0x10
        jmp         _collect_vgabios_info
#endif

_collect_vgabios_info:
        # grab the VGA BIOS table
        movb        $0x1B, %ah          # 1Bh = get video func/state info
        xorw        %bx, %bx            #   param: bx = 0
        leaw        bVgaBiosTable, %di
        int         $0x10
        cmpb        $0x1B, %al          # if al = 1Bh, function supported
        jne         _vga_done
        movb        $1, bBootInfo+BI_HAS_VGABIOS_TABLE
        leaw        bVgaBiosTable, %di
        movzwl      %di, %edi
        movl        %edi, bBootInfo+BI_VGABIOS_TABLE_PTR
_vga_done:
        ret

#if CONFIG_VGA_MODE
_badmode:
        PRINT       szBadMode
        jmp         readkey_then_reset
#endif

#if CONFIG_VGA_FONT
_badfont:
        PRINT       szBadFont
        jmp         readkey_then_reset
#endif

##
# -----------------------------------------------------------------------------
# detect_memory
# -----------------------------------------------------------------------------
# Gets memory capacity and physical memory map (if available) using BIOS
# routines. If all methods fail, or if there is not enough memory to run the
# operating system, the system will display an error and halt.
##
detect_memory:
        call        enable_a20              # turn on infamous A20 line
        call        _detectmem_e820h        # newest method (2002+)
        call        _detectmem_e801h        # newer method  (1994+)
        call        _detectmem_legacy       # oldest method (all 386s)

        cmpw        $0, bBootInfo+BI_KB_HIGH    # fail if not at least 1M of RAM
        jne         _detectmem_ok
        cmpw        $0, bBootInfo+BI_KB_HIGH_E801h
        jne         _detectmem_ok
        cmpw        $0, bBootInfo+BI_KB_EXTENDED
        jne         _detectmem_ok
        jmp         _detectmem_not_enough
        # TODO: config.h option to allow operating within 640K of RAM?
_detectmem_ok:
        ret

_detectmem_not_enough:
        PRINT       szMemTooLow
        jmp         readkey_then_reset

##
# _detectmem_e820h:
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
##
_detectmem_e820h:
        clc
        xorl        %eax, %eax
        movw        %ax, %es
        movw        $MEMMAP_BASE, %di       # dest buffer
        xorl        %ebx, %ebx              # continuation value (0 to begin)
_detectmem_e820h_loop:
        movl        $0x18, %ecx             # table entry size
        movl        $0x534D4150, %edx       # 'SMAP'
        movl        $0xE820, %eax           # Query System Address Map function
        int         $0x15
        jc          _detectmem_e820h_done   # error or end of table
        cmpb        $0x80, %ah
        je          _detectmem_e820h_done   # invalid command
        cmpb        $0x86, %ah
        je          _detectmem_e820h_done   # unsupported function
        testl       %ebx, %ebx
        jz          _detectmem_e820h_end    # end of table
        addw        $0x18, %di              # move table ptr
        cmpb        $0x14, %cl              # if extended attr not present...
        jne         _detectmem_e820h_loop
        movl        $0, -4(%edi)            # ...ensure it is stored as zero
        jmp         _detectmem_e820h_loop
_detectmem_e820h_end:
        movl        $MEMMAP_BASE, bBootInfo+BI_MEM_MAP
_detectmem_e820h_done:
        ret

##
# _detectmem_e801h:
# Gets the number of contiguous 1K blocks of memory between 1M and 16M, and
# the number of contiguous 64K blocks of memory between 16M and 4G.
#
# This function should work on BIOSes made after 1994.
##
_detectmem_e801h:
        # data can be returned in ax/bx or cx/dx; initialize all to zero
        xorw        %ax, %ax
        xorw        %bx, %bx
        xorw        %cx, %cx
        xorw        %dx, %dx
        movw        $0xE801, %ax
        int         $0x15
        jc          _detectmem_e801h_done   # unsupported function
        jcxz        _detectmem_e801h_store  # cx = 0, use ax/bx
        movw        %cx, %ax
        movw        %dx, %bx
_detectmem_e801h_store:
        movw        %ax, bBootInfo+BI_KB_HIGH_E801h  # 1M-16M, contig 1K blocks
        movw        %bx, bBootInfo+BI_KB_EXTENDED    # 16M-4G, contig 64K blocks
_detectmem_e801h_done:
        ret

##
# _detectmem_legacy:
# Gets the number of contiguous 1K blocks from 0 to 640K, and from 1M up to 15M
# or 64M, depending on the BIOS. If detection fails, the system will display
# an error and halt. This detection method should work on every 386 BIOS, so if
# this fails, either something is seriously wrong or the system does not have
# enough memory to run this OS!
##
_detectmem_legacy:
        clc
        movb        $0x88, %ah
        int         $0x15
        jc          _detectmem_legacy_lo
        cmpb        $0x80, %ah              # (invalid command)
        je          _detectmem_legacy_lo
        cmpb        $0x86, %ah              # (unsupported func)
        je          _detectmem_legacy_lo
        movw        %ax, bBootInfo+BI_KB_HIGH   # >1M, contig 1K blocks
_detectmem_legacy_lo:
        clc
        int         $0x12
        movw        %ax, bBootInfo+BI_KB_LOW    # <1M, contig 1K blocks
        testw       %ax, %ax
        jz          _detectmem_not_enough   # (unsupported if 0, very old PC)
        ret

##
# -----------------------------------------------------------------------------
# enable_a20
# -----------------------------------------------------------------------------
# Attempts to enable the A20 gate. If unsuccessful, the system will halt;
# otherwise there would be a very annoying non-contiguous memory hole.
###
enable_a20:
        call        _test_a20
        jnc         _enable_a20_done
        call        _enable_a20_bios
        call        _test_a20
        jnc         _enable_a20_done
        call        _enable_a20_port_92h
        call        _test_a20
        jnc         _enable_a20_done
        call        _enable_a20_keyboard
        call        _test_a20
        jc          _enable_a20_error
_enable_a20_done:
        ret

_enable_a20_error:
        PRINT       szA20Error
        jmp         readkey_then_reset

##
# Tests whether A20 gate is enabled.
##
_test_a20:
        xorw        %ax, %ax
        movw        %ax, %fs                # fs = 0x0000
        decw        %ax                     # gs = 0xFFFF
        movw        %ax, %gs
        rorw        $4, %ds:0x7DFE          # tweak the boot sector ID
        movw        %fs:0x7DFE, %dx         # [0x00FDFE] -> dx
        movw        %gs:0x7E0E, %ax         # [0x10FDFE] -> ax
        cmpw        %ax, %dx                # are they the same?
        je          _test_a20_off
        clc
        jmp         _test_a20_done
_test_a20_off:
        stc                                 # CF=1 if A20 disabled
_test_a20_done:
        ret

##
# Enables A20 using BIOS INT-15h,AX=2401h. Supported on "newer" systems.
##
_enable_a20_bios:
        movw        $0x2401, %ax
        int         $0x15
        jc          _enable_a20_bios_done   # error
        cmpb        $0x86, %ah              # function not supported
        je          _enable_a20_bios_done
        movw        $A20_BIOS, bBootInfo+BI_A20_METHOD
_enable_a20_bios_done:
        ret

##
# Enables A20 by writing port 92h (System Control Port A).
# Not supported on all systems.
##
_enable_a20_port_92h:
        inb         $0x92, %al
        orb         $0x02, %al              # A20 enable bit
        andb        $0xFE, %al              # ensure system reset flag cleared
        outb        %al, $0x92              # send it
        movw        $A20_PORT92, bBootInfo+BI_A20_METHOD
        ret

##
# Enables A20 via the PS/2 keyboard controller.
# This is the most common method and usually does the trick.
##
_enable_a20_keyboard:
        call        kb_flush
        movb        $0xD1, %al              # cmd: write controller output port
        outb        %al, $0x64
        call        kbout_wait              # wait for controller ready
        movb        $0xFF, %al              # A20 enable bit
        outb        %al, $0x60              # write output reg
        call        kb_flush
        movw        $A20_KEYBOARD, bBootInfo+BI_A20_METHOD
        ret

##
# Waits for the keyboard's output buffer to be empty.
##
kbout_wait:
        inb         $0x64, %al
        testb       $0x02, %al
        jz          _kbout_wait_done
        jmp         kbout_wait
_kbout_wait_done:
        ret

##
# Reads the keyboard's input buffer while there are bytes to be read.
# The bytes read are discarded.
##
kb_flush:
        call        kbout_wait
_kb_flush_loop:
        inb         $0x64, %al
        testb       $0x01, %al
        jz          _kb_flush_done
        inb         $0x60
        jmp         _kb_flush_loop
_kb_flush_done:
        ret

##
# -----------------------------------------------------------------------------
# check_kernel_size
# -----------------------------------------------------------------------------
# Ensures the kernel will fit in low memory. To do this, we find the address of
# the Extended BIOS Data Area, which is effectively the top of low memory. We
# then calculate the available space by subtracting the early kernel base
# address from the EBDA address. Finally, we compare the kernel file size with
# the available space.
#
# We should have roughly 576K bytes available for the kernel. If we ever exceed
# this size (hopefully not), we'll have to load the kernel in stages, which
# would require a working floppy disk driver in Protected Mode (or make use of
# the BIOS again in Unreal Mode).
#
# Inputs:
#       ecx - kernel image size
##
check_kernel_size:
        movl        $KERNEL_LOAD, %eax
        cmpl        $0, bBootInfo+BI_EBDA_BASE
        jz          _no_ebda_info       # EBDA = effective top of low memory
        movl        bBootInfo+BI_EBDA_BASE, %ebx
        jmp         _size_check
_no_ebda_info:                          # no EBDA? use low-memory field instead
        movl        bBootInfo+BI_KB_LOW, %ebx
        shll        $10, %ebx           # stored as 1K blocks, multiply by 1024
_size_check:
        subl        %eax, %ebx          # Capacity = pTopOfMem - pKernelLoad
        cmpl        %ecx, %ebx          # Capacity - KernelSize < 0?
        jl          _kernel_too_big     #   yep, fail
_size_check_done:
        ret

_kernel_too_big:
        PRINT       szKernelTooBig
        jmp         readkey_then_reset

##
# -----------------------------------------------------------------------------
# num2str
# -----------------------------------------------------------------------------
# Converts a 16-bit word into an uppercase hexadecimal string, prefixed with 0x,
# then prints the string.
#
# Inputs:
#       cx  - the value to print
#
# Clobbers:
#       ax, bx, cx, dx
##
num2str:
        leaw        szNum2StrBuf, %bx
        movw        $0x0A0D, %es:(%bx)
        addw        $2, %bx
        movw        $0x7830, %es:(%bx)
        addw        $2, %bx
        movw        $0x4, %cx
_num2str_loop:
        rolw        $4, %ax
        movw        %ax, %dx
        andb        $0xF, %dl
        cmpb        $10, %dl
        jae          _num2str_letter
        addb        $'0', %dl
        jmp         _num2str_store
_num2str_letter:
        subb        $10, %dl
        addb        $'A', %dl
_num2str_store:
        movb        %dl, %es:(%bx)
        incw        %bx
        decw        %cx
        jnz         _num2str_loop
        movb        $0, %es:(%bx)
        PRINT       szNum2StrBuf
        ret

# =============================================================================
# ----------------------------------- Data ------------------------------------
# =============================================================================

# Boot Loader GDT
# Bare minimum to keep the CPU happy in Protected Mode. The kernel will
# eventually replace this with it's own.
.align 8
_TheGdt:
        .quad       0                       # (must be 0)
        .quad       0x00CF9A000000FFFF      # [0x08] BOOT_CS
        .quad       0x00CF92000000FFFF      # [0x10] BOOT_DS
_TheGdtEnd:

# Boot Loader GDT Descriptor
# This is what actually gets loaded into GDTR. The Intel manual states that the
# address portion of the 48-bit descriptor must be 32-bit aligned, so we place
# a padding word at the beginning to ensure this is the case.
        .word       0   # alignment padding
GdtDesc:
        .word       _TheGdtEnd -  _TheGdt - 1
        .long       _TheGdt

# -----------------------------------------------------------------------------

lStage2Size:        .long   0
sKernelFileName:    .ascii  "OHWES   SYS"
szBadMode:          .asciz  "\r\nInvalid VGA mode selection!"
szBadFont:          .asciz  "\r\nInvalid font selection!"
szA20Error:         .asciz  "\r\nFailed to enable A20 gate!"
szKernelTooBig:     .asciz  "\r\nCannot fit kernel in 640K of memory!"
szMemTooLow:        .asciz  "\r\nNot enough system memory!"
szBadMagic:         .asciz  "\r\nInvalid boot signature!"
szNum2StrBuf:       .space  10, FILL_CHAR
bVgaBiosTable:      .space  SIZEOF_VGABIOS_TABLE, FILL_CHAR
bBootInfo:          .zero   SIZEOF_BI
