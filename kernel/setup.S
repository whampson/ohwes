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
#         File: boot/setup.S
#      Created: July 18, 2024
#       Author: Wes Hampson
#  Environment: 32-bit Protected Mode
# =============================================================================

#include <config.h>
#include <kernel.h>
#include <paging.h>
#include <x86.h>

.code32
.section .setup, "ax", @progbits

.globl setup_kernel
setup_kernel:
        movl            %ecx, boot_params_ptr   # store boot params
        lgdt            gdt_desc                # load the real GDT
        ljmp            $KERNEL_CS, $_setup     # reload CS
_setup:
        # set up the Protected Mode stack stack
        movl            $STACK_BASE, %ebp
        movl            %ebp, %esp

        # reload the segment registers
        movw            $KERNEL_DS, %ax
        movw            %ax, %ds                # ds = KERNEL_DS
        movw            %ax, %es                # es = KERNEL_DS
        movw            %ax, %ss                # ss = KERNEL_DS
        xorw            %ax, %ax
        movw            %ax, %fs                # fs = (not used)
        movw            %ax, %gs                # gs = (not used)
        movw            $_LDT_SEGMENT, %ax
        lldt            %ax                     # LDT not used
        movw            $_TSS_SEGMENT, %ax
        ltr             %ax                     # TSS not used

# Right now, we only care about 4M of memory; we know our kernel is under 1M
# because we loaded it in Real Mode from the boot loader, which can only address
# 1M, thus we need just one page table. We are going to map the kernel to
# virtual address C0000000-C00FFFFF. We are also going to temporarily identity
# map 00000000-000FFFFF so this setup code can continue execution. Setup code is
# part of the kernel image, but linked with a base address of 0, while the rest
# of the kernel is linked with a base of C0000000.

TABLE_COUNT = PAGE_SIZE >> 2
PAGE_FLAGS = _PAGE_PRESENT | _PAGE_RW
KERNEL_PFN = PAGE_OFFSET >> (PGDIR_SHIFT - 2)

        # zero the page directory
        xorl            %eax, %eax
        movl            $TABLE_COUNT, %ecx
        movl            $KERNEL_PGDIR, %edi
    rep stosl

        # zero the page table
        movl            $TABLE_COUNT, %ecx
        movl            $KERNEL_PGTBL, %edi
    rep stosl

        # open up low 1M in page table
        movl            $KERNEL_PGTBL, %edi     # pgtbl
        xorl            %ebx, %ebx              # pfn (page frame number)
        xorl            %ecx, %ecx              # count
_map_low_1M_loop:
        movl            %ebx, %eax              # pte = pfn << PAGE_SHIFT
        shll            $PAGE_SHIFT, %eax
        orl             $PAGE_FLAGS, %eax       # pte |= PAGE_FLAGS
        movl            %eax, 0(%edi, %ebx, 4)  # pgtbl[pfn] = pte
        incl            %ebx                    # pfn++
        incl            %ecx                    # count++
        cmpl            $0x100, %ecx
        jb              _map_low_1M_loop

        # setup page directory
        #  map virt:00000000-000FFFFF to phys:00000000-000FFFFF (temp for setup)
        #  map virt:C0000000-C00FFFFF to phys:00000000-000FFFFF (kernel)
        movl            $KERNEL_PGDIR, %edi
        movl            $KERNEL_PGTBL, %eax
        orl             $PAGE_FLAGS, %eax
        movl            %eax, KERNEL_PFN(%edi)
        movl            %eax, 0(%edi)           # identity map

        # enable paging
        movl            $KERNEL_PGDIR, %eax
        movl            %eax, %cr3
        movl            %cr0, %eax
        orl             $CR0_PG, %eax
        movl            %eax, %cr0

        # now that paging is enabled, reload the stack registers, IDT and GDT
        # registers, EIP, and the boot params ptr with their kernel-space
        # addresses
        addl            $PAGE_OFFSET, boot_params_ptr
        addl            $PAGE_OFFSET, %esp
        addl            $PAGE_OFFSET, %ebp
        addl            $PAGE_OFFSET, _gdt_ptr
        addl            $PAGE_OFFSET, _idt_ptr
        lgdt            gdt_desc
        lidt            idt_desc
        leal            _reload_eip, %eax
        addl            $PAGE_OFFSET, %eax
        jmp             *%eax
_reload_eip:
        # now we are executing in higher-half address space :-) this screws with
        # debugger symbols, luckily we aren't in this code for much longer...

        # load boot params ptr, clear identity mapping, and pass control off
        # to the higher-half kernel; we use an indirect jump instead of a call
        # because calls are EIP-relative (I think), and the previous indirect
        # call used to reload EIP seems to screw with the call address
        movl            boot_params_ptr, %ecx   # fastcall param goes in ecx
        movl            $0, KERNEL_PGDIR        # clear identity mapping
        movl            %cr3, %eax
        movl            %eax, %cr3              # flush TLB
        leal            _die, %ebx
        addl            $PAGE_OFFSET, %ebx
        pushl           %ebx                    # push return address
        leal            start_kernel, %eax
        jmp             *%eax                   # jump to higher-half kernel

_die:
        # we should never end up here... but in case we do, make sure we do
        # something predictable! print an X in the corner of the screen and spin
        # forever
        movw            $0x4758, PAGE_OFFSET+0xB8000
_idle:
        jmp             _idle

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

boot_params_ptr:
        .long           0

# -----------------------------------------------------------------------------

# Interrupt Descriptor Table
# The IDT is aligned to PAGE_SIZE in case we ever decide to implement the
# Pentium F00F bug workaround :-D
.align PAGE_SIZE, 0
idt:
        .fill           256, 4, 0
_idt_end:

        .word           0                       # align
idt_desc:
        .word           _idt_end - idt - 1
_idt_ptr:
        .long           idt

# -----------------------------------------------------------------------------

# Global Descriptor Table
# We load the real GDT here and not in stage2 so we can keep it in kernel space.
.align 8
gdt:
        .quad           0                       # (must be 0)
        .quad           0                       # (not used)
        .quad           0x00CF9A000000FFFF      # [0x10] KERNEL_CS  rx  ring0
        .quad           0x00CF92000000FFFF      # [0x18] KERNEL_DS  rw  ring0
        .quad           0x00CFFA000000FFFF      # [0x23] USER_CS    rx  ring3
        .quad           0x00CFF2000000FFFF      # [0x2B] USER_DS    rw  ring3
        .quad           0x00C0820000000000      # [0x30] _LDT_SEGMENT
        .quad           0x00C0890000000000      # [0x38] _TSS_SEGMENT
_gdt_end:

        .word           0                       # align
gdt_desc:
        .word           _gdt_end - gdt - 1
_gdt_ptr:
        .long           gdt
