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
#         File: kernel/setup.S
#      Created: July 18, 2024
#       Author: Wes Hampson
#  Environment: 32-bit Protected Mode
# =============================================================================

#include <kernel.h>
#include <paging.h>
#include <x86.h>

.code32
.section .setup, "ax", @progbits

##
# Kernel setup.
# Bridge the gap between the boot loader and kernel by initializing the
# Protected Mode CPU descriptors and setting up virtual memory mappings for
# the kernel code.
##
.globl setup_kernel
setup_kernel:
        movl            %ecx, boot_params_ptr   # store boot params
        lgdt            gdt_desc                # load the real GDT
        ljmp            $KERNEL_CS, $_setup     # cs = KERNEL_CS
_setup:
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

        # setup the init stack
        movl            $KERNEL_INIT_STACK, %ebp
        movl            %ebp, %esp

        # setup the IDT and paging; first 1M of physical memory will be mapped
        # at PAGE_OFFSET and temporarily identity mapped
        call            setup_idt
        call            setup_paging

        # now that paging is enabled and kernel-space is mapped in high memory,
        # reload the stack registers, IDT and GDT registers, the boot params
        # ptr, and EIP with their kernel-space equivalents
        movl            $PAGE_OFFSET+KERNEL_INIT_STACK, %ebp
        movl            %ebp, %esp
        addl            $PAGE_OFFSET, boot_params_ptr
        addl            $PAGE_OFFSET, _gdt_ptr
        addl            $PAGE_OFFSET, _idt_ptr
        lgdt            gdt_desc
        lidt            idt_desc
        ljmp            $KERNEL_CS, $PAGE_OFFSET+_kernel_trampoline

_kernel_trampoline:
        # now we are executing in high address space :-) this screws with
        # debugger symbols, luckily we aren't in this code for much longer...

        # load boot params ptr, clear identity mapping, and pass control off
        # to the high-mapped kernel
        movl            boot_params_ptr, %ecx       # fastcall param goes in ecx
        movl            $0, KERNEL_PGDIR            # clear identity mapping
        movl            %cr3, %eax
        movl            %eax, %cr3                  # flush TLB
        lcall           $KERNEL_CS, $start_kernel   # far call into the kernel

        # we should never end up here... but in case we do, make sure we do
        # something predictable! print an X in the corner of the screen and spin
        # forever
        movw            $0x4758, PAGE_OFFSET+0xB8000
_idle:
        jmp             _idle

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# setup_idt
# -----------------------------------------------------------------------------
# Fill the IDT with interrupt vectors (see kernel/entry.S).
##
setup_idt:
        # TODO
        lidt            idt_desc
        ret

# -----------------------------------------------------------------------------

##
# -----------------------------------------------------------------------------
# setup_paging
# -----------------------------------------------------------------------------
# Create the initial page tables for kernel setup and initialization and enable
# paging on the CPU. Right now, we only care about the low 1M of physical
# memory. We know our kernel is under 1M in size because we loaded it in Real
# Mode from the boot loader, which can only address 1M. Thus, we need just one
# page table in addition to the page directory (we are using 4K pages only at
# this point). If the kernel ever grows larger than about 512k, we will need to
# adjust this scheme :-) The kernel is linked with a base virtual address of
# C0000000 (PAGE_OFFSET), while this setup code is linked with a base address of
# 00000000 so it can bridge the gap between the boot loader and the kernel.
# Therefore, we must create two mappings for the low 1M of physical memory: one
# for the kernel at C0000000-C00FFFFF and another temporary identity mapping so
# this setup code can continue execution.
##
setup_paging:

TABLE_COUNT = PAGE_SIZE >> 2
PAGE_FLAGS = _PAGE_PRESENT | _PAGE_RW
KERNEL_PDN = PAGE_OFFSET >> (PGDIR_SHIFT - 2)

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
        movl            $KERNEL_PGTBL, %edi         # pgtbl
        xorl            %ebx, %ebx                  # pfn (page frame number)
        xorl            %ecx, %ecx                  # count
_map_low_1M_loop:
        movl            %ebx, %eax                  # pte = pfn << PAGE_SHIFT
        shll            $PAGE_SHIFT, %eax
        orl             $PAGE_FLAGS, %eax           # pte |= PAGE_FLAGS
        movl            %eax, 0(%edi, %ebx, 4)      # pgtbl[pfn] = pte
        incl            %ebx                        # pfn++
        incl            %ecx                        # count++
        cmpl            $0x100, %ecx
        jb              _map_low_1M_loop

        # setup page directory
        #  map virt:00000000-000FFFFF to phys:00000000-000FFFFF (temp for setup)
        #  map virt:C0000000-C00FFFFF to phys:00000000-000FFFFF (kernel)
        movl            $KERNEL_PGDIR, %edi
        movl            $KERNEL_PGTBL, %eax
        orl             $PAGE_FLAGS, %eax
        movl            %eax, KERNEL_PDN(%edi)
        movl            %eax, 0(%edi)               # identity map

        # enable paging
        movl            $KERNEL_PGDIR, %eax
        movl            %eax, %cr3
        movl            %cr0, %eax
        orl             $CR0_PG, %eax
        movl            %eax, %cr0
        # TODO: set WP and PSE bit in CR4

_setup_paging_done:
        ret

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

boot_params_ptr:
        .long           0

# -----------------------------------------------------------------------------

# Interrupt Descriptor Table
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
