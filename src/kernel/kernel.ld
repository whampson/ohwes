/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/kernel.ld                                                  *
 * Created: December 9, 2020                                                  *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Linker script for the kernel binary.                                       *
 *============================================================================*/

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kentry)

SECTIONS
{
    /* Kernel Base Address
       This should match KERNEL_BASE defined in <ohwes/init.h> */
    . = 0x100000;   /* 1 MiB */
    __start = .;

    /* ELF Binary Sections */
    .text       : {
        *(.entry)       /* Ensure the setup ASM code goes first. */
        *(.text)
    }
    .data       : { *(.data) }
    .bss        : { *(.bss) }
    .rodata     : { *(.rodata) }
    __end = .;

    /* Since we initially load the kernel in low memory, we need to
       ensure the kernel is small enough to fit in this region.
       Limit the kernel size to 512 KiB. */
    . = ASSERT((__end - __start) <= 0x80000, "Kernel is too big!");
}
