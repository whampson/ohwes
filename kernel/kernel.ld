/* =============================================================================
* Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
*
* This file is part of the OH-WES Operating System.
* OH-WES is free software; you may redistribute it and/or modify it under the
* terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* -----------------------------------------------------------------------------
*         File: kernel/kernel.ld
*      Created: June 3, 2024
*       Author: Wes Hampson
* =============================================================================
*/

#include <config.h>
#include <paging.h>

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

SECTIONS
{
    __kernel_lma = KERNEL_BASE;
    __kernel_vma = PAGE_OFFSET + __kernel_lma;
    __kernel_start = __kernel_vma;

    __setup_lma = __kernel_lma;
    .setup __setup_lma :
    {
        __setup_start = .;
        *(.setup)
        __setup_end = .;
    }
    __setup_size = SIZEOF(.setup);
    . = ALIGN(PAGE_SIZE);

    __text_lma = .;
    __text_vma = PAGE_OFFSET + __text_lma;
    .text __text_vma : AT (__text_lma)
    {
        __text_start = .;
        *(.text*)
        __text_end = .;
    }
    __text_size = SIZEOF(.text);

    .data :
    {
        __data_start = .;
        *(.data*)
        __data_end = .;
    }
    __data_size = SIZEOF(.data);

    .rodata :
    {
        __rodata_start = .;
        *(.rodata*)
        __rodata_end = .;
    }
    __rodata_size = SIZEOF(.rodata);

    .bss :
    {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
    __bss_size = SIZEOF(.bss);

    /* TODO: do we need this?
             we aren't doing kernel-level exception handling... */
    .eh_frame :
    {
        __eh_frame_start = .;
        *(.eh_frame*)
        __eh_frame_end = .;
    }
    __eh_frame_size = SIZEOF(.eh_frame);

    __kernel_end = .;
    __kernel_size = __kernel_end - __kernel_start;
}
