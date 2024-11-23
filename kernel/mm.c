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
 *         File: kernel/mm.c
 *      Created: July 3, 2024
 *       Author: Wes Hampson
 *
 * Physical Page Allocator, Buddy System:
 * https://www.kernel.org/doc/gorman/html/understand/understand009.html
 * =============================================================================
 */

#include <assert.h>
#include <boot.h>
#include <cpu.h>
#include <config.h>
#include <list.h>
#include <mm.h>
#include <errno.h>
#include <ohwes.h>
#include <paging.h>
#include <pool.h>

// linker script symbols -- use operator& to get assigned value
extern uint32_t __kernel_start, __kernel_end, __kernel_size;
extern uint32_t __setup_start, __setup_end, __setup_size;
extern uint32_t __text_start, __text_end, __text_size;
extern uint32_t __data_start, __data_end, __data_size;
extern uint32_t __rodata_start, __rodata_end, __rodata_size;
extern uint32_t __bss_start, __bss_end, __bss_size;

// see doc/mm.txt for memory map

static void print_memory_map(const struct boot_info *info);
static void print_page_info(uint32_t vaddr, const struct pginfo *page);
static void print_page_mappings(struct mm_info *mm);

static void init_bss(struct boot_info *boot_info);
extern void init_pools(void);   // pool.c

struct mm_info _mm = { };
struct mm_info *g_mm = &_mm;

struct free_area {
    struct list_node free_list;
    void *bitmap;   // buddy list pair state
};

#define MAX_ORDER   11
struct zone {
    uintptr_t zone_base;
    struct free_area free_area[MAX_ORDER];
    pool_t free_list_pool;
};

void init_mm(const struct boot_info *boot_info)
{
    kprint("mem: PA:%08x-%08x VA:%08x-%08x kernel image\n",
        __virt_to_phys(&__kernel_start), __virt_to_phys(&__kernel_end),
        &__kernel_start, &__kernel_end);
    kprint("mem: PA:%08x-%08x VA:%08x-%08x .setup\n",
        __virt_to_phys(&__setup_start), __virt_to_phys(&__setup_end),
        &__setup_start, &__setup_end);
    kprint("mem: PA:%08x-%08x VA:%08x-%08x .text\n",
        __virt_to_phys(&__text_start), __virt_to_phys(&__text_end),
        &__text_start, &__text_end);
    kprint("mem: PA:%08x-%08x VA:%08x-%08x .data\n",
        __virt_to_phys(&__data_start), __virt_to_phys(&__data_end),
        &__data_start, &__data_end);
    kprint("mem: PA:%08x-%08x VA:%08x-%08x .rodata\n",
        __virt_to_phys(&__rodata_start), __virt_to_phys(&__rodata_end),
        &__rodata_start, &__rodata_end);
    kprint("mem: PA:%08x-%08x VA:%08x-%08x .bss\n",
        __virt_to_phys(&__bss_start), __virt_to_phys(&__bss_end),
        &__bss_start, &__bss_end);
    kprint("mem: kernel image takes up %d pages\n",
        PAGE_ALIGN((uint32_t) &__kernel_size) >> PAGE_SHIFT);

    init_bss((struct boot_info *) boot_info);

    g_mm->pgdir = (void *) __phys_to_virt(KERNEL_PGDIR);

    // print_memory_map(boot_info);
    // print_page_mappings(g_mm);
}

static void init_bss(struct boot_info *boot_info)
{
    // zero the BSS region
    // make sure we back up the boot info so we don't lose it!!
    struct boot_info boot_info_copy;
    memcpy(&boot_info_copy, boot_info, sizeof(struct boot_info));
    memset(&__bss_start, 0, (size_t) &__bss_size);
    memcpy(boot_info, &boot_info_copy, sizeof(struct boot_info));
}

static void print_memory_map(const struct boot_info *info)
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
        const acpi_mmap_t *e = info->mem_map;
        while (e->type != 0)
        {
#if PRINT_MEMORY_MAP
            uint32_t base = (uint32_t) e->base;
            uint32_t limit = (uint32_t) e->length - 1;

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

static void print_page_mappings(struct mm_info *mm)
{
#if PRINT_PAGE_MAP
    struct pginfo *pgdir = mm->pgdir;
    struct pginfo *pgtbl;
    struct pginfo *page;
    uint32_t vaddr;

    for (int i = 0; i < PDE_COUNT; i++) {
        page = &pgdir[i];
        if (!page->p) {
            continue;
        }

        vaddr = i << PGDIR_SHIFT;
        print_page_info(vaddr, page);

        if (page->pde && page->ps) {
            continue;   // large
        }

        pgtbl = (struct pginfo *) (page->pfn << PAGE_SHIFT);
        for (int j = 0; j < PTE_COUNT; j++) {
            page = &pgtbl[j];
            if (!page->p) {
                continue;
            }
            vaddr = (i << PGDIR_SHIFT) | (j << PAGE_SHIFT);
            print_page_info(vaddr, page);
        }
    }
#endif
}

static void print_page_info(uint32_t vaddr, const struct pginfo *page)
{
    uint32_t paddr = page->pfn << PAGE_SHIFT;
    uint32_t plimit = paddr + PAGE_SIZE - 1;
    uint32_t vlimit = vaddr + PAGE_SIZE - 1;
    if (page->pde) {
        if (page->ps) {
            plimit = paddr + PGDIR_SIZE - 1;
        }
        vlimit = vaddr + PGDIR_SIZE - 1;
    }

    //            vaddr-vlimit -> paddr-plimit k/M/T rw u/s a/d g wt nc
    kprint("page: v(%08X-%08X) -> p(%08X-%08X) %c %-2s %c %c %c %s%s\n",
        vaddr, vlimit, paddr, plimit,
        page->pde ? (page->ps ? 'M' : 'T') : 'k',       // (k) small page, (M) large page, (T) page table
        page->rw ? "rw" : "r",                          // read/write
        page->us ? 'u' : 's',                           // user/supervisor
        page->a ? (page->d ? 'd' : 'a') : ' ',          // accessed/dirty
        page->g ? 'g' : ' ',                            // global
        page->pwt ? "wt " : "  ",                       // write-through
        page->pcd ? "nc " : "  ");                      // no-cache
}
