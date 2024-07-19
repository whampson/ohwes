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

#include <kernel.h>

.code32
.section .setup, "ax", @progbits

.globl setup_kernel
setup_kernel:
        movw            $0x0C53, 0xB8000
        movl            %ecx, boot_params_ptr   # store boot params
        lgdt            gdt_ptr                 # load the real GDT
        ljmp            $KERNEL_CS, $_setup
_setup:
        # set up Protected Mode registers
        movw            $KERNEL_DS, %ax
        movw            %ax, %ds                # ds = KERNEL_DS
        movw            %ax, %es                # es = KERNEL_DS
        movw            %ax, %ss                # ss = KERNEL_DS
        xorw            %ax, %ax
        movw            %ax, %fs                # fs = (not used)
        movw            %ax, %gs                # gs = (not used)
        lidt            idt_ptr                 # IDT to be filled later...
        movw            $_LDT_SEGMENT, %ax
        lldt            %ax                     # LDT not used
        movw            $_TSS_SEGMENT, %ax
        ltr             %ax                     # TSS not used

        # reset the stack
        movl            $STACK_BASE, %ebp
        movl            %ebp, %esp

        # TODO: init paging


        movl            boot_params_ptr, %ecx
        // call            kmain

_idle:
        hlt
        jmp             _idle

# -----------------------------------------------------------------------------
# ----------------------------------- Data ------------------------------------
# -----------------------------------------------------------------------------

# The GDT. We load the real GDT here and not in stage2 so we can keep it in
# kernel space.
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

# -----------------------------------------------------------------------------

        .word           0                       # align
gdt_ptr:
        .word           _gdt_end - gdt - 1
        .long           gdt

# -----------------------------------------------------------------------------

        .word           0                       # align
idt_ptr:
        .word           (4 * 256) - 1
        .long           IDT_BASE

# -----------------------------------------------------------------------------

boot_params_ptr:
        .long           0
