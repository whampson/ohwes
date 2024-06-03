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
 *         File: kernel/memory.c
 *      Created: January 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <boot.h>
#include <ohwes.h>
#include <errno.h>
#include <paging.h>

/*
 Physical Address Map:

LOW MEMORY:

  100000 +----------------------------+ 1M
         |////////////////////////////|
         | (reserved for hardware) ///|
         |////////////////////////////|
   C0000 +----------------------------+ 768K
         |                            |
         | VGA Frame Buffer           |
         |                            |
   B8000 +----------------------------+ 736K
         |////////////////////////////|
         | (reserved for VGA) ////////|
         |////////////////////////////|
   A0000 +----------------------------+ 640K
         |////////////////////////////|
         | (reservred for EBDA) //////|
         |////////////////////////////|
   9FC00 +----------------------------+ 639K    <-- address can vary by hardware
         |                            |
         | (free)                     |
         |                            |
   91000 +----------------------------+ 128K
         | User Stack                 |         <-- TODO: dynamically allocate
   90000 +----------------------------+ 128K
         |                            |
         |                            |
         |                            |
         | (free)                     |
         |                            |
         |                            |
         |                            |
         |                            |
         +----------------------------+ (varies)
         |                            |
         |                            |
         | Kernel Code                |
         |                            |
         |                            |
   20000 +----------------------------+ 128K
         |                            |
         |                            |
         | DMA Buffer                 |
         |                            |
         |                            |
   10000 +----------------------------+ 64K
         | Kernel Stack               |         <-- TODO: dynamically allocate
    F000 +----------------------------+ 63K
         |                            |
         | (free)                     |
         |                            |
    5000 +----------------------------+ 20K
         | Kernel Page Table (0-4M)   |         <-- TODO: dynamically allocate
    4000 +----------------------------+ 16K
         | Memory Info                |
    3000 +----------------------------+ 12K
         | IDT/GDT/LDT/TSS            |
    2000 +----------------------------+ 8K
         | System Page Directory      |
    1000 +----------------------------+ 4K
         | Zero Page                  |
       0 +============================+ 0K


HIGH MEMORY:

FFFFFFFF +----------------------------+ 4G
         |                            |
         |                            |
         |                            |
         |                            |
         | (free, if available)       |
         |                            |
         |                            |
         |                            |
         |                            |
 1000000 +----------------------------+ 16M
         |////////////////////////////|
         | (reserved for hardware) ///|
         |////////////////////////////|
  F00000 +----------------------------+ 15M
         |                            |
         |                            |
         | (free, if available)       |
         |                            |
         |                            |
  100000 +----------------------------+ 1M


NOTES:
    * VGA frame buffer hardware is mapped at 0xA0000-0xBFFFF. Which chunk of
      this range is used depends on the VGA hardware configuration (see vga.c).
      For now, we are using the region from 0xB8000-0xBFFFF (CGA config).
*/

struct mem_info
{
    bool large_page_support;        // large page = 4M bytes = 1024 4K pages
    uint32_t total_physical_pages;
};

struct mem_info *g_mem_info;

static void print_meminfo(const struct boot_info *info);
extern int init_paging(const struct boot_info *boot_info, uint32_t pgtbl);

void init_memory(const struct boot_info *boot_info)
{
    zeromem((void *) SYSTEM_MEMORY_PAGE, PAGE_SIZE);
    g_mem_info = (struct mem_info *) SYSTEM_MEMORY_PAGE;

    print_meminfo(boot_info);   // TODO: copy ACPI memory map to mem info page
    init_paging(boot_info, KERNEL_PAGE_TABLE);

#if PRINT_PAGE_MAP
    print_page_mappings();
#endif
}

static void print_meminfo(const struct boot_info *info)
{
    int kb_total = 0;
    int kb_free = 0;
    int kb_reserved = 0;
    int kb_acpi = 0;
    int kb_bad = 0;

    int kb_free_low = 0;    // between 0 and 640k
    int kb_free_1M = 0;     // between 1M and 16M
    int kb_free_16M = 0;    // between 1M and 4G

    if (!info->mem_map) {
        kprint("mem: bios-e820: memory map not available\n");
        if (info->kb_high_e801h != 0) {
            kb_free_1M = info->kb_high_e801h;
            kb_free_16M = (info->kb_extended << 6);
        }
        else {
            kprint("mem: bios-e801: memory map not available\n");
            kb_free_1M = info->kb_high;
        }
        kb_free_low = info->kb_low;
        kb_free = kb_free_low + kb_free_1M + kb_free_16M;
    }
    else {
        kprint("mem: bios-e820: ACPI memory map at %08X\n", info->mem_map);
        const acpi_mmap_t *e = info->mem_map;
        while (e->type != 0) {
            uint32_t base = (uint32_t) e->base;
            uint32_t limit = (uint32_t) e->length - 1;

#if PRINT_MEMORY_MAP
            kprint("mem: bios-e820: %08lX-%08lX ", base, base+limit, e->attributes, e->type);
            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE: kprint("free"); break;
                case ACPI_MMAP_TYPE_RESERVED: kprint("reserved"); break;
                case ACPI_MMAP_TYPE_ACPI: kprint("reserved ACPI"); break;
                case ACPI_MMAP_TYPE_ACPI_NVS: kprint("reserved ACPI non-volatile"); break;
                case ACPI_MMAP_TYPE_BAD: kprint("bad"); break;
                default: kprint("unknown (%d)", e->type); break;
            }
            if (e->attributes) {
                kprint(" (attributes = %X)", e->attributes);
            }
            kprint("\n");
#endif

            // TODO: kb count does not account for overlapping regions

            int kb = (e->length >> 10);
            kb_total += kb;

            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE:
                    kb_free += kb;
                    break;
                case ACPI_MMAP_TYPE_ACPI:
                case ACPI_MMAP_TYPE_ACPI_NVS:
                    kb_acpi += kb;
                    break;
                case ACPI_MMAP_TYPE_BAD:
                    kb_bad += kb;
                    break;
                default:
                    kb_reserved += kb;
                    break;
            }

            e++;
        }
    }

    kprint("mem: %dk free", kb_free);
    if (kb_total) kprint(", %dk total", kb_total);
    if (kb_bad) kprint(", %dk bad", kb_bad);
    kprint("\n");
    if (kb_free < MIN_KB_REQUIRED) {
        panic("we need at least %dk of RAM to operate!", MIN_KB_REQUIRED);
    }
}
