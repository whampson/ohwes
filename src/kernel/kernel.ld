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

#define ENTRY_POINT ksetup

OUTPUT_ARCH("i386")
OUTPUT_FORMAT("elf32-i386")
ENTRY(ENTRY_POINT)

MEMORY
{
    SETUP_RAM("WX")  : ORIGIN = KERNEL_BASE, LENGTH = 4K
    KERNEL_ROM("RX") : ORIGIN = KERNEL_VA, LENGTH = 512K
    KERNEL_RAM("WX") : ORIGIN = KERNEL_VA, LENGTH = 512K
}

SECTIONS
{
    __kernel_start = KERNEL_BASE;

    __setup_offset = .;
    .setup :
    {
        __setup_start = .;
        LONG(ENTRY_POINT);  /* TODO: ELF header */
        *(.setup)
        *(.setup.*)
        __setup_end = .;
    } > SETUP_RAM
    __setup_size = SIZEOF(.setup);
    . = __setup_start + __setup_size;

    __text_offset = .;
    .text KERNEL_VA + __text_offset : AT(__text_offset)
    {
        __text_start = .;
        *(.text)
        *(.text.*)
        __text_end = .;
    } > KERNEL_ROM
    __text_size = SIZEOF(.text);
    . = __text_offset + __text_size;

    __rodata_offset = .;
    .rodata KERNEL_VA + __rodata_offset : AT(__rodata_offset)
    {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        __rodata_end = .;
    } > KERNEL_ROM
    __rodata_size = SIZEOF(.rodata);
    . = __rodata_offset + __rodata_size;

    __data_offset = .;
    .data KERNEL_VA + __data_offset : AT(__data_offset)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > KERNEL_RAM
    __data_size = SIZEOF(.data);
    . = __data_offset + __data_size;

    __bss_offset = .;
    .bss KERNEL_VA + __bss_offset : AT(__bss_offset)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    } > KERNEL_RAM
    __bss_size = SIZEOF(.bss);
    . = __bss_offset + __bss_size;

    __kernel_end = .;
    __kernel_size = __kernel_end - __kernel_start;

    /DISCARD/ : { *(.eh_frame) *(.eh_frame_hdr) }
}
