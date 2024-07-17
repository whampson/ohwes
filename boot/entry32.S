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
#         File: boot/entryetry32.S
#      Created: July 14, 2024
#       Author: Wes Hampson
#  Environment: 32-bit Protected Mode
#
# =============================================================================

#include <boot.h>
#include "x86_boot.h"

# -----------------------------------------------------------------------------
# --------------------- Stage 2 (32-bit Protected Mode) -----------------------
# -----------------------------------------------------------------------------
.text
.code32

.globl entry32
entry32:
        # set up Protected Mode segment selectors
        movw            $BOOT_DS, %ax
        movw            %ax, %ds
        movw            %ax, %es
        movw            %ax, %ss
        xorw            %ax, %ax
        movw            %ax, %fs
        movw            %ax, %gs

        # reset the stack
        movl            $STACK_BASE, %ebp
        movl            %ebp, %esp

        movw            $0x0C53, 0xB8000
        movw            $0x0B54, 0xB0000

        # TODO: init paging

// _loop:
//         jmp             _loop

        movl            $boot_info, %ecx        # pass bootparams ptr to kernel
        ljmpl           $BOOT_CS, $KERNEL_ENTRY # bye bye bootloader!

