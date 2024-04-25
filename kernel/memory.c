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

#include <boot.h>
#include <ohwes.h>

/*
 Physical Address Map:

         |                            |
         | Kernel Code                |
         |                            |
   10000 +----------------------------+ 64K
         |                            |
         | Kernel Stack / Free        |
         |                            |
    4000 +----------------------------+ 16K
         | Kernel Page Table          |
    3000 +----------------------------+ 12K
         | System Page Directory      |
    2000 +----------------------------+ 8K
         | CPU Data Area              |
    1000 +----------------------------+ 4K
         | NULL Page                  |
       0 +============================+ 0K
*/


#define PAGE_DIR    0x2000
#define PAGE_TABLE  0x3000

static void print_meminfo(const struct boot_info *info);

void init_memory(const struct boot_info *info)
{
    print_meminfo(info);

    struct pde *pgdir = (struct pde *) PAGE_DIR;
    zeromem(pgdir, PAGE_SIZE);

    pgdir[0].p = 1;     // present
    pgdir[0].rw = 1;    // writable
    pgdir[0].us = 1;    // user-accessible (for now)
    pgdir[0].ps = 0;    // point to a 4k page table
    pgdir[0].address = PAGE_TABLE >> PAGE_SHIFT;


    struct pte *pgtbl = (struct pte *) PAGE_TABLE;
    zeromem(pgtbl, PAGE_SIZE);

    // open up 0x1000-0x100000, make everything else inaccessible for now
    for (int i = 0; i < 1024; i++) {
        if ((i > 0 && i < 0xA0) || (i >= 0xB8 && i <= 0xBF)) {
            pgtbl[i].p = 1;     // present
            pgtbl[i].rw = 1;    // writable
            pgtbl[i].us = 1;    // user-accessible
            pgtbl[i].address = i;
        }
    }

    uint32_t cr3 = 0;
    cr3 |= PAGE_DIR;
    // cr3 |= CR3_PCD;
    // cr3 |= CR3_PWT;
    load_cr3(cr3);

    uint32_t cr0 = 0;
    store_cr0(cr0);
    cr0 |= CR0_PG;
    load_cr0(cr0);
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
        kprint("bios-e820: memory map not available\n");
        if (info->kb_high_e801h != 0) {
            kb_free_1M = info->kb_high_e801h;
            kb_free_16M = (info->kb_extended << 6);
        }
        else {
            kprint("bios-e801: memory map not available\n");
            kb_free_1M = info->kb_high;
        }
        kb_free_low = info->kb_low;
        kb_free = kb_free_low + kb_free_1M + kb_free_16M;
    }
    else {
        const acpi_mmap_t *e = info->mem_map;
        while (e->type != 0) {
            uint32_t base = (uint32_t) e->base;
            uint32_t limit = (uint32_t) e->length - 1;

#if SHOW_MEMMAP
            kprint("bios-e820: %08lX-%08lX ", base, base+limit, e->attributes, e->type);
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

    kprint("boot: %dk free", kb_free);
    if (kb_total) kprint(", %dk total", kb_total);
    if (kb_bad) kprint(", %dk bad", kb_bad);
    kprint("\n");
    if (kb_free < MIN_KB_REQUIRED) {
        panic("we need at least %dk of RAM to operate!", MIN_KB_REQUIRED);
    }
}
