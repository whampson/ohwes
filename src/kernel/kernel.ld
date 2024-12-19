/* =============================================================================
* Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
*         File: src/kernel/kernel.ld
*      Created: June 3, 2024
*       Author: Wes Hampson
* =============================================================================
*/

#include <i386/paging.h>
#include <kernel/config.h>

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(setup_kernel)

SECTIONS
{
    _kernel_lma = KERNEL_BASE;
    _kernel_vma = PAGE_OFFSET + _kernel_lma;
    _kernel_start = _kernel_vma;

    _setup_lma = _kernel_lma;
    .setup _setup_lma :
    {
        _setup_start = .;
        *(.setup)
        _setup_end = .;
    }
    _setup_size = SIZEOF(.setup);
    . = ALIGN(PAGE_SIZE);

    _text_lma = .;
    _text_vma = PAGE_OFFSET + _text_lma;
    .text _text_vma : AT (_text_lma)
    {
        _text_start = .;
        *(.text*)
    }
    _text_end = .;
    _text_size = SIZEOF(.text);

    .data :
    {
        _data_start = .;
        *(.data*)
    }
    _data_end = .;
    _data_size = SIZEOF(.data);

    .rodata :
    {
        _rodata_start = .;
        *(.rodata*)
    }
    _rodata_end = .;
    _rodata_size = SIZEOF(.rodata);

    .bss :
    {
        _bss_start = .;
        *(.bss*)
    }
    _bss_end = .;
    _bss_size = SIZEOF(.bss);

    _kernel_end = .;
    _kernel_size = _kernel_end - _kernel_start;
}
