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
#         File: src/i386/kernel/setup.S
#      Created: July 18, 2024
#       Author: Wes Hampson
#  Environment: 32-bit Protected Mode
# =============================================================================

#include <i386/boot.h>
#include <i386/interrupt.h>
#include <i386/paging.h>
#include <i386/x86.h>
#include <kernel/config.h>
#include <kernel/kernel.h>
#include <kernel/irq.h>         // TODO: should be i386/irq.h
#include <kernel/serial.h>

.code32
.section .setup.text, "ax", @progbits

##
# -----------------------------------------------------------------------------
# ksetup
# -----------------------------------------------------------------------------
# void __fastcall __noreturn ksetup(struct boot_info *);
#
# The kernel entry point. Bridges the gap between the boot loader and kernel by
# initializing the CPU descriptors and setting up virtual memory. This function
# should perform other necessary architecture-specific setup to ensure the
# kernel can do basic things like print and not explode if an interrupt occurs.
# Essentially "stage 3" of the boot loader.
##
.globl ksetup
ksetup:
        # make damn sure interrupts are off!
        cli

        # copy boot params from stage2
        movl            %ecx, %esi
        movl            boot_info_ptr, %edi
        movl            $BI_SIZE, %ecx
        cld
    rep movsb

        # setup the inititial stack
        movl            $KERNEL_STACK, %ebp
        movl            %ebp, %esp

        # setup paging; first 1M of physical memory will be identity mapped
        call            setup_paging

        # now that paging is enabled and kernel space is mapped at KERNEL_VA,
        # adjust the descriptor table pointers and boot info pointers to point
        # to kernel space
        addl            $KERNEL_VA, %ebp
        movl            %ebp, %esp
        addl            $KERNEL_VA, gdt_ptr
        addl            $KERNEL_VA, idt_ptr
        addl            $KERNEL_VA, boot_info_ptr

        # load the GDT and IDT descriptor registers, perform the jump that
        # reloads CS to begin execution in kernel space, then reload the
        # segment registers with new values from the GDT
        lgdt            gdt_desc
        lidt            idt_desc
        ljmp            $KERNEL_CS, $_ksetup_continue

.text   # we're executing in kernel-space now...
_ksetup_continue:
        movw            $KERNEL_DS, %ax
        movw            %ax, %ds                # ds = KERNEL_DS
        movw            %ax, %es                # es = KERNEL_DS
        movw            %ax, %ss                # ss = KERNEL_DS
        xorw            %ax, %ax
        movw            %ax, %fs                # fs = 0
        movw            %ax, %gs                # gs = 0

        # now that virtual memory is set up, zero the BSS region;
        # do this here to ensure all uninitialized variables are zeroed
        # before we ever reach C code
        movl            $__bss_size, %ecx
        movl            $__bss_start, %edi
   rep stosb

        # now we can build the CPU descriptor tables;
        # the GDT is partially initiailzed statically below (see _gdt)
        call            setup_cpu               # see cpu.c

#if SERIAL_DEBUGGING
        call            setup_debug_port
#endif

#if HIGHER_GROUND
        movl            $0, KERNEL_PGDIR        # clear identity mapping
#endif

        # pass control off to the kernel
        movl            %cr3, %eax
        movl            %eax, %cr3              # flush TLB
        leal            boot_info_ptr, %ecx     # get ptr to boot params ptr
        addl            $KERNEL_VA, %ecx        # adjust addr for kernel space
        lcall           $KERNEL_CS, $kmain      # far call into the kernel

_idle:  # we should never end up here... but in case we do, spin forever
        jmp             _idle

# -----------------------------------------------------------------------------
# ------------------------------- Subroutines ---------------------------------
# -----------------------------------------------------------------------------
.section .setup.text, "ax", @progbits

##
# -----------------------------------------------------------------------------
# setup_paging
# -----------------------------------------------------------------------------
# Create the initial page tables for kernel setup and initialization and enable
# paging on the CPU. Right now, we only care about the low 1M of physical
# memory. We know our kernel is under 1M in size because we loaded it in Real
# Mode from the boot loader, which can only address 1M. Thus, we need just one
# page table in addition to the page directory. If the kernel ever grows larger
# than about 512k, we will need to adjust this scheme :-) The kernel is linked
# with a base virtual address of KERNEL_VA, while this setup code is linked with
# a base address of 0, so it can bridge the gap between the boot loader and the
# kernel. Therefore, we must create two mappings for the low 1M of physical
# memory: one for the kernel at C0000000-C00FFFFF and another temporary identity
# mapping so this setup code can continue execution.
##
setup_paging:

PAGE_FLAGS = _PAGE_PRESENT | _PAGE_RW
PAGE_FLAGS = PAGE_FLAGS |  _PAGE_USER   # TODO: don't map kernel memory with user privileges lol
KERNEL_PDN = KERNEL_VA >> (PGDIR_SHIFT - 2)
TABLE_COUNT = PAGE_SIZE >> 2

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

        # setup null pointer trap
        andl            $~_PAGE_PRESENT, 0(%edi)

        # setup page directory
        #  map virt:00000000-000FFFFF to phys:00000000-000FFFFF (temp for setup)
        #  map virt:C0000000-C00FFFFF to phys:00000000-000FFFFF (kernel)
        movl            $KERNEL_PGDIR, %edi
        movl            $KERNEL_PGTBL, %eax
        orl             $PAGE_FLAGS, %eax
        orl             $_PAGE_PDE, %eax
        movl            %eax, 0(%edi)               # identity map
#if HIGHER_GROUND
        movl            %eax, KERNEL_PDN(%edi)      # "high" map
#endif

        # enable paging
        movl            $KERNEL_PGDIR, %eax
        movl            %eax, %cr3
        movl            %cr0, %eax
        orl             $CR0_PG, %eax               # PG = paging enable
        orl             $CR0_WP, %eax               # WP = write-protect
        movl            %eax, %cr0

_setup_paging_done:
        ret


##
# -----------------------------------------------------------------------------
# setup_debug_port
# -----------------------------------------------------------------------------
# Initializes the serial port for remote debugging.
##
setup_debug_port:
        # determine whether the port exists
        xorb            %al, %al
        movw            $SERIAL_DEBUG_PORT+UART_SCR, %dx
        outb            %al, %dx
        movb            $0x55, %al
        outb            %al, %dx
        movb            %al, %ah
        inb             %dx, %al
        cmpb            %al, %ah
        jne             setup_debug_port_done

        # configure the UART port
        movw            $SERIAL_DEBUG_PORT+UART_LCR, %dx
        movb            $0x80, %al
        outb            %al, %dx        # enable DLAB so we can set baud
        movb            $SERIAL_DEBUG_BAUD >> 8, %al
        movw            $SERIAL_DEBUG_PORT+UART_DLM, %dx
        outb            %al, %dx        # set baud divisor high byte
        movw            $SERIAL_DEBUG_PORT+UART_DLL, %dx
        movb            $SERIAL_DEBUG_BAUD & 0xFF, %al
        outb            %al, %dx        # set baud divisor low byte
        movw            $SERIAL_DEBUG_PORT+UART_LCR, %dx
        movb            $0x03, %al
        outb            %al, %dx        # LCR = 8N1
        movw            $SERIAL_DEBUG_PORT+UART_MCR, %dx
        movb            $0x0B, %al
        outb            %al, %dx        # MCR = dtr rts out2
        xorb            %al, %al
        movw            $SERIAL_DEBUG_PORT+UART_IER, %dx
        outb            %al, %dx        # no serial interrupts
        movw            $SERIAL_DEBUG_PORT+UART_FCR, %dx
        movb            $0xC7, %al
        outb            %al, %dx        # enable FIFO

        # clear any pending reads
        movw            $SERIAL_DEBUG_PORT+UART_LSR, %dx
        inb             %dx, %al
        movw            $SERIAL_DEBUG_PORT+UART_MSR, %dx
        inb             %dx, %al
        movw            $SERIAL_DEBUG_PORT+UART_IIR, %dx
        inb             %dx, %al

        movl            $1, g_debug_port_available

setup_debug_port_done:
        ret


# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------
.section .setup.data, "aw", @progbits

# -----------------------------------------------------------------------------
# GDT

# Global Descriptor Table
# We load the real GDT here and not in stage2 so we can keep it in kernel space.
# The LDT and TSS segments are loaded later, after we've begun executing the
# higher-half kernel.
.align 8
_gdt:
        .quad           0                       # (must be 0)
        .quad           0                       # [0x08] KERNEL_LDT
        .quad           0x00CF9A000000FFFF      # [0x10] KERNEL_CS  rx  ring0
        .quad           0x00CF92000000FFFF      # [0x18] KERNEL_DS  rw  ring0
        .quad           0x00CFFA000000FFFF      # [0x23] USER_CS    rx  ring3
        .quad           0x00CFF2000000FFFF      # [0x2B] USER_DS    rw  ring3
        .quad           0                       # [0x30] USER_TSS
        .quad           0                       # [0x38] KERNEL_TSS
_gdt_end:

# -----------------------------------------------------------------------------
# GDT Descriptor

        .word           0                       # align
gdt_desc:
gdt_limit:
        .word           _gdt_end - _gdt - 1
gdt_ptr:
        .long           _gdt

# -----------------------------------------------------------------------------
# IDT Descriptor

        .word           0                       # align
idt_desc:
idt_limit:
        .word           IDT_BASE + (NR_INTERRUPT_VECTORS << 3) - 1
idt_ptr:
        .long           IDT_BASE

# -----------------------------------------------------------------------------
# Boot Info

boot_info:
        .fill           BI_SIZE
boot_info_ptr:
        .long           boot_info
