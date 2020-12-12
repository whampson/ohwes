/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/main.c                                                     *
 * Created: December 9, 2020                                                  *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kentry)

SECTIONS
{
    . = 0x10000;        /* Should match KERNEL_BASE in <nb/init.h> */
    .text       : {
        *(.entry)       /* Ensure the setup ASM code goes first. */
        *(.text) 
    }
    .data       : { *(.data) }
    .bss        : { *(.bss) }
    .rodata     : { *(.rodata*) }
    /* Others? */
    _end = .;

    . = ASSERT(_end <= 0x9C000, "Kernel is too big!");
    /* Some systems report reserved memory near 0x9FFFF, so limit the kernel
       to 0x9C000 just to be safe. */
}