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
*         File: i386/kernel/kernel.ld
*      Created: June 3, 2024
*       Author: Wes Hampson
* =============================================================================
*/

#include <i386/paging.h>
#include <kernel/config.h>

/* TERMINOLOGY:
    _base   == base physical address
    _limit  == end physical address less 1 (Intel convention)
    _start  == base virtual address
    _end    == end virtual address
    _size   == (_end - start) == (_limit - _base + 1)
*/

#define SECTION_PROLOGUE(sect)  \
    __##sect##_base = .

#define SECTION_DEFINE(sect)    \
    .sect KERNEL_VA + __##sect##_base : AT(__##sect##_base)

#define SECTION_EPILOGUE(sect)                              \
    __##sect##_size = SIZEOF(.sect);                        \
    __##sect##_start = ADDR(.sect);                         \
    __##sect##_end = __##sect##_start + __##sect##_size;    \
    __##sect##_base = LOADADDR(.sect);                      \
    . = __##sect##_base + __##sect##_size;                  \
    __##sect##_limit = (. - 1)

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
    __kernel_start = KERNEL_VA;
    __kernel_base = KERNEL_BASE;

    /* initial bootstrap code */
    __setup_base = __kernel_base;
    .setup __setup_base : AT(__setup_base)
    {
        /* NOTE: this section does NOT execute in high memory */
        LONG(ENTRY_POINT);  /* TODO: ELF header */
        *(.setup*)
    } > SETUP_RAM
    SECTION_EPILOGUE(setup);

    /* code */
    SECTION_PROLOGUE(text);
    SECTION_DEFINE(text)
    {
        *(.text*)
    } > KERNEL_ROM
    SECTION_EPILOGUE(text);

    /* read-only data */
    SECTION_PROLOGUE(rodata);
    SECTION_DEFINE(rodata)
    {
        *(.rodata*)
    } > KERNEL_ROM
    SECTION_EPILOGUE(rodata);

    /* writable data */
    SECTION_PROLOGUE(data);
    SECTION_DEFINE(data)
    {
        *(.data*)
    } > KERNEL_RAM
    SECTION_EPILOGUE(data);

    /* uninitialized data */
    SECTION_PROLOGUE(bss);
    SECTION_DEFINE(bss)
    {
        *(.bss*)
    } > KERNEL_RAM
    SECTION_EPILOGUE(bss);

    /* system memory structures */
    . = ALIGN(PAGE_SIZE);
    SECTION_PROLOGUE(paging);
    SECTION_DEFINE(paging)
    {
        /* kernel page directory */
        __page_dir = .;
        . += PAGE_SIZE;

        /* kernel page table */
        __page_tbl = .;
        . += PAGE_SIZE;

        /* page frame bitmap */
        __page_map = .;
        . += 32 * PAGE_SIZE; /* 32 = (((4G / 4096) / 8) / 4096) */
    } > KERNEL_RAM
    SECTION_EPILOGUE(paging);

    /* TODO: put things like GDT/IDT/LDT, stacks, etc. here! */

    __kernel_size = .;
    __kernel_end = __kernel_start + __kernel_base + __kernel_size;
    __kernel_limit = __kernel_base + (. - 1);

    /DISCARD/ : { *(.eh_frame) *(.eh_frame_hdr) }
}

ASSERT((__setup_start == __setup_base),      ".setup start mismatch!");
ASSERT((  __setup_end == __setup_limit + 1), ".setup end mismatch!");

ASSERT((  __text_start == KERNEL_VA +   __text_base),   ".text start mismatch!");
ASSERT((__rodata_start == KERNEL_VA + __rodata_base), ".rodata start mismatch!");
ASSERT((  __data_start == KERNEL_VA +   __data_base),   ".data start mismatch!");
ASSERT((   __bss_start == KERNEL_VA +    __bss_base),    ".bss start mismatch!");
ASSERT((__paging_start == KERNEL_VA + __paging_base), ".paging start mismatch!");

ASSERT((  __text_end == KERNEL_VA +   __text_limit + 1),   ".text end mismatch!");
ASSERT((__rodata_end == KERNEL_VA + __rodata_limit + 1), ".rodata end mismatch!");
ASSERT((  __data_end == KERNEL_VA +   __data_limit + 1),   ".data end mismatch!");
ASSERT((   __bss_end == KERNEL_VA +    __bss_limit + 1),    ".bss end mismatch!");
ASSERT((__paging_end == KERNEL_VA + __paging_limit + 1), ".paging end mismatch!");

ASSERT(( __setup_end -  __setup_start ==  __setup_limit -  __setup_base + 1),  ".setup size mismatch!");
ASSERT((  __text_end -   __text_start ==   __text_limit -   __text_base + 1),   ".text size mismatch!");
ASSERT((__rodata_end - __rodata_start == __rodata_limit - __rodata_base + 1), ".rodata size mismatch!");
ASSERT((  __data_end -   __data_start ==   __data_limit -   __data_base + 1),   ".data size mismatch!");
ASSERT((   __bss_end -    __bss_start ==    __bss_limit -    __bss_base + 1),    ".bss size mismatch!");
ASSERT((__paging_end - __paging_start == __paging_limit - __paging_base + 1), ".paging size mismatch!");
