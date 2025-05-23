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
 *         File: src/kernel/mm/mm.c
 *      Created: July 3, 2024
 *       Author: Wes Hampson
 *
 * Physical Page Allocator, Buddy System:
 * https://www.kernel.org/doc/gorman/html/understand/understand009.html
 * =============================================================================
 */

#include <assert.h>
#include <errno.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <kernel/config.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

// linker script symbols -- use operator& to get assigned value
extern uint32_t _kernel_start, _kernel_end, _kernel_size;
extern uint32_t _setup_start, _setup_end, _setup_size;
extern uint32_t _text_start, _text_end, _text_size;
extern uint32_t _data_start, _data_end, _data_size;
extern uint32_t _rodata_start, _rodata_end, _rodata_size;
extern uint32_t _bss_start, _bss_end, _bss_size;

// see doc/mm.txt for memory map

static void print_kernel_sections(void);
static void print_memory_map(void);
static void print_page_info(uint32_t vaddr, const struct pginfo *page);
static void print_page_mappings(void);

static void init_bss(void);

extern void init_pools(void);

__data_segment struct mm_info _mm = { };
__data_segment struct mm_info *g_mm = &_mm;

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

#define BASELIMIT(base, size)   (base), ((base)+(size)-1)

void init_mm(void)
{
    g_mm->pgdir = (void *) KERNEL_ADDR(KERNEL_PGDIR);

    print_memory_map();
    print_kernel_sections();

#if PRINT_PAGE_MAP
    print_page_mappings();
#endif

    init_bss();
    init_pools();
}

static void init_bss(void)
{
    // zero the BSS region
    memset(&_bss_start, 0, (size_t) &_bss_size);
}

static void print_kernel_sections(void)
{
    struct section {
        const char *name;
        void *start, *end;
    };

    struct section sections[] = {
        { "setup stack",        (void *) SETUP_STACK-FRAME_SIZE,        (void *) SETUP_STACK },
        { "interrupt stacks",   (void *) INT_STACK_LIMIT,               (void *) INT_STACK_BASE },
        { "page directory",     (void *) KERNEL_PGDIR,                  (void *) KERNEL_PGDIR+PAGE_SIZE },
        { "kernel page table",  (void *) KERNEL_PGTBL,                  (void *) KERNEL_PGTBL+PAGE_SIZE },
        { "kernel image:",      &_kernel_start,                         &_kernel_end },
        { ".setup",             &_setup_start,                          &_setup_end },
        { ".text",              &_text_start,                           &_text_end },
        { ".data",              &_data_start,                           &_data_end },
        { ".rodata",            &_rodata_start,                         &_rodata_end },
        { ".bss",               &_bss_start,                            &_bss_end },
    };

    for (int i = 0; i < countof(sections); i++) {
        struct section *sec = &sections[i];
        kprint("PA:%08X-%08X VA:%08X-%08X %s\n",
            PHYSICAL_ADDR(sec->start), PHYSICAL_ADDR(sec->end)-1,
            sec->start, sec->end-1, sec->name);
    }

    kprint("kernel stack space allows for %d nested interrupts\n", NR_INT_STACKS);
    kprint("kernel image is %dk bytes (%d pages)\n",
        align((size_t) &_kernel_size, 1024) >> 10,
        PAGE_ALIGN((size_t) &_kernel_size) >> PAGE_SHIFT);
}

static void print_memory_map(void)
{
    int kb_total = 0;
    int kb_free = 0;
    int kb_reserved = 0;
    int kb_acpi = 0;
    int kb_bad = 0;

    int kb_free_low = 0;    // between 0 and 640k
    int kb_free_1M = 0;     // between 1M and 16M
    int kb_free_16M = 0;    // between 1M and 4G

    if (!g_boot->mem_map) {
        kprint("bios-e820: memory map not available\n");
        if (g_boot->kb_high_e801h != 0) {
            kb_free_1M = g_boot->kb_high_e801h;
            kb_free_16M = (g_boot->kb_extended << 6);
        }
        else {
            kprint("bios-e801: memory map not available\n");
            kb_free_1M = g_boot->kb_high;
        }
        kb_free_low = g_boot->kb_low;
        kb_free = kb_free_low + kb_free_1M + kb_free_16M;
    }
    else {
        const acpi_mmap_t *e = (const acpi_mmap_t *) KERNEL_ADDR(g_boot->mem_map);
        while (e->type != 0)
        {
#if PRINT_MEMORY_MAP
            uint32_t base = (uint32_t) e->base;
            uint32_t limit = (uint32_t) e->length - 1;

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

    if (kb_total) kprint("%dk total, ", kb_total);
    kprint("%dk free", kb_free);
    if (kb_bad) kprint(", %dk bad", kb_bad);
    kprint("\n");

    if (kb_free < MIN_KB) {
        panic("not enough memory -- OH-WES needs least %dk to operate!", MIN_KB);
    }
}

static void print_page_mappings(void)
{
    struct pginfo *pgdir = g_mm->pgdir;
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
