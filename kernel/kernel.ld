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
    /* Kernel base address; should match KERN_BASE defined in <ohwes/boot.h> */
    . = 0x10000;

    /* ELF sections */
    .text       : {
        *(.entry)       /* Ensure the setup ASM code goes first. */
        *(.text) 
    }
    .data       : { *(.data) }
    .bss        : { *(.bss) }
    .rodata     : { *(.rodata) }
    _end = .;

    /* Don't allow the kernel to grow beyond 0x80000;
       this is where we keep the TTY input/output buffers. */
    . = ASSERT(_end <= 0x80000, "Kernel is too big!");
}
