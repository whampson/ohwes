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
    _start  == base virtual address
    _end    == end virtual address
    _size   == _end - start
*/

#define DEFINE_SECTION(sect)    \
    .sect KERNEL_VA + __##sect##_base : AT(__##sect##_base)

#define DEFINE_SECTION_PHYSICAL(sect)   \
    .sect __##sect##_base : AT(__##sect##_base)

#define SECTION_PROLOGUE(sect)  \
    __##sect##_base = .

#define SECTION_EPILOGUE(sect)                              \
    __##sect##_base = LOADADDR(.sect);                      \
    __##sect##_size = SIZEOF(.sect);                        \
    __##sect##_start = ADDR(.sect);                         \
    __##sect##_end = __##sect##_start + __##sect##_size;    \
    . = __##sect##_base + __##sect##_size

#define ENTRY_POINT ksetup

OUTPUT_ARCH("i386")
OUTPUT_FORMAT("elf32-i386")
ENTRY(ENTRY_POINT)

MEMORY
{
    RAM_PHYSICAL("WX")  : ORIGIN = 0, LENGTH = 4M
    RAM_VIRTUAL("RW")   : ORIGIN = KERNEL_VA, LENGTH = 4M
    ROM_VIRTUAL("RX")   : ORIGIN = KERNEL_VA, LENGTH = 4M
    /* Yeah yeah yeah, virtual RAM and ROM regions overlap, but that's OK.
       This distinction is mostly there to keep the linker happy because
       sometimes it complains about LOAD sections being both writeable and
       executable. A reasonable complaint, but I plan to manage execute
       permissions via the paging structures. */
}

SECTIONS
{
    __kernel_base = KERNEL_BASE;
    __kernel_start = KERNEL_VA + __kernel_base;

    . = __kernel_base;

    /* ----------------------------------------------------------------------*/
    /* ---------------------------- Kernel Image --------------------------- */
    /* ----------------------------------------------------------------------*/

    SECTION_PROLOGUE(setup);
    DEFINE_SECTION_PHYSICAL(setup)
    {
        /* initial bootstrap code */
        /* NOTE: this section does NOT execute in high memory */
        LONG(ENTRY_POINT);  /* TODO: ELF header, preferably condensed */
        *(.setup*)
    } > RAM_PHYSICAL
    SECTION_EPILOGUE(setup);

    SECTION_PROLOGUE(text);
    DEFINE_SECTION(text)
    {
        /* kernel code */
        *(.text*)
    } > ROM_VIRTUAL
    SECTION_EPILOGUE(text);

    SECTION_PROLOGUE(rodata);
    DEFINE_SECTION(rodata)
    {
        /* read-only data */
        *(.rodata*)
    } > ROM_VIRTUAL
    SECTION_EPILOGUE(rodata);

    /* TODO: page boundary to ensure writable data get their own pages! */

    SECTION_PROLOGUE(data);
    DEFINE_SECTION(data)
    {
        /* writable data */
        *(.data*)

        /* sneak the GDT in here */
        . = ALIGN(8);
        __gdt = .;
        QUAD(0);                            /* (must be 0)                   */
        QUAD(0);                            /* [0x08] KERNEL_LDT             */
        LONG(0x0000FFFF); LONG(0x00CF9A00); /* [0x10] KERNEL_CS  rx  ring0   */
        LONG(0x0000FFFF); LONG(0x00CF9200); /* [0x18] KERNEL_DS  rw  ring0   */
        LONG(0x0000FFFF); LONG(0x00CFFA00); /* [0x23] USER_CS    rx  ring3   */
        LONG(0x0000FFFF); LONG(0x00CFF200); /* [0x2B] USER_DS    rw  ring3   */
        QUAD(0);                            /* [0x30] KERNEL_TSS             */
        QUAD(0);                            /* [0x38] EMERG_TSS              */
        __gdt_end = .;

        /* For some reason, if both the host and target are 32-bit systems, the
           QUAD() expression accepts only 32-bit values, and will clamp anything
           larger... so QUAD(0xCAFEBABEDEADBEEF) will write 0xFFFFFFFF... weird!
        */

    } > RAM_VIRTUAL
    SECTION_EPILOGUE(data);

    SECTION_PROLOGUE(bss);
    DEFINE_SECTION(bss)
    {
        /* uninitialized data */
        *(.bss*)
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(bss);

    /* ----------------------------------------------------------------------*/
    /* ---------------------------- Static Data ---------------------------- */
    /* ----------------------------------------------------------------------*/

    . = ALIGN(16);

    SECTION_PROLOGUE(idt);
    DEFINE_SECTION(idt)
    {
        /* interrupt descriptor table */
        __idt = .;
        . += (NR_IDT_VECTORS << 3);
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(idt);

    . = ALIGN(PAGE_SIZE);

    SECTION_PROLOGUE(pgdir);
    DEFINE_SECTION_PHYSICAL(pgdir)
    {
        /* kernel page directory */
        __page_dir = .;
        . += PAGE_SIZE;
    } > RAM_PHYSICAL
    SECTION_EPILOGUE(pgdir);

    SECTION_PROLOGUE(pgtbl);
    DEFINE_SECTION_PHYSICAL(pgtbl)
    {
        /* kernel page table */
        __page_tbl = .;
        . += PAGE_SIZE;
    } > RAM_PHYSICAL
    SECTION_EPILOGUE(pgtbl);

    SECTION_PROLOGUE(pgmap);
    DEFINE_SECTION_PHYSICAL(pgmap)
    {
        /* page frame bitmap */
        __page_map = .;
        . += PAGE_MAP_SIZE;
    } > RAM_PHYSICAL
    SECTION_EPILOGUE(pgmap);

    SECTION_PROLOGUE(klog);
    DEFINE_SECTION(klog)
    {
        /* kernel console log buffer */
        __klog = .;
        . += KERNEL_LOG_SIZE;
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(klog);


    SECTION_PROLOGUE(kstack);
    DEFINE_SECTION(kstack)
    {
        /* kernel stack */
        __kstack = .;
        . += FRAME_SIZE;
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(kstack);

    SECTION_PROLOGUE(ustack);
    DEFINE_SECTION(ustack)
    {
        /* user stack */
        __ustack = .;
        . += FRAME_SIZE;
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(ustack);

    SECTION_PROLOGUE(estack);
    DEFINE_SECTION(estack)
    {
        /* emergency stack */
        __estack = .;
        . += FRAME_SIZE;
    } > RAM_VIRTUAL
    SECTION_EPILOGUE(estack);

    /* ----------------------------------------------------------------------*/
    /* ------------------------- End Kernel Space -------------------------- */
    /* ----------------------------------------------------------------------*/

    __kernel_end = KERNEL_VA + .;
    __kernel_size = __kernel_end - __kernel_start;

    /DISCARD/ : { *(.eh_frame) *(.eh_frame_hdr) }
}
