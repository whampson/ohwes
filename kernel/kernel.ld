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
    __kernel_base = PAGE_OFFSET;
    __setup_lma = KERNEL_LMA;

    .setup __setup_lma :
    {
        *(.setup)
    }
    . = ALIGN(PAGE_SIZE);

    __kernel_text_lma = .;
    __kernel_text_vma = PAGE_OFFSET + __kernel_text_lma;
    .text __kernel_text_vma : AT (__kernel_text_lma)
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
    }

    .eh_frame :
    {
        *(.eh_frame*)
    }

    . = ALIGN(PAGE_SIZE);
    __kernel_pages = (. - __kernel_base) >> PAGE_SHIFT;
}
