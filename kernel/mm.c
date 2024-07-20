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
 * =============================================================================
 */

#include <assert.h>
#include <boot.h>
#include <cpu.h>
#include <mm.h>
#include <errno.h>
#include <ohwes.h>
#include <paging.h>

// see doc/mm.txt for memory map

static void print_memory_map(const struct boot_info *info);
static void print_page_info(uint32_t vaddr, const struct pginfo *page);
static void print_page_mappings(struct mm_info *mm);

struct mm_info kernel_mm;

void init_mm(const struct boot_info *boot_info)
{
    bool large_page_support;
    struct cpuid cpuid;
    struct mm_info *kmm;
    pde_t *pde;
    pte_t *pte;
    pgflags_t flags;
    uint32_t addr;

    print_memory_map(boot_info);

    kmm = &kernel_mm;

    zeromem((void *) SYSTEM_PAGE_DIRECTORY, PAGE_SIZE); // TODO: get from boot info
    zeromem((void *) KERNEL_PAGE_TABLE, PAGE_SIZE);

    zeromem(kmm, sizeof(struct mm_info));
    kmm->pgdir = (pde_t *) SYSTEM_PAGE_DIRECTORY;

    flags = _PAGE_USER | _PAGE_RW;

    // create kernel page directory entry
    pde = pde_offset(kmm, KERNEL_PAGE_TABLE);
    *pde = __mkpde(KERNEL_PAGE_TABLE, flags);

    // map cpu structs
    pte = pte_offset(pde, SYSTEM_CPU_PAGE);
    *pte = __mkpte(SYSTEM_CPU_PAGE, flags);

    // map frame buffer
    for (int i = 0; i < boot_info->framebuffer_pages; i++) {
        addr = boot_info->framebuffer + (i << PAGE_SHIFT);
        pte = pte_offset(pde, addr);
        *pte = __mkpte(addr, flags);
    }

    // map kernel code
    uint32_t num_kernel_code_pages = div_ceil(boot_info->kernel_size, PAGE_SIZE);
    for (int i = 0; i <  num_kernel_code_pages; i++) {
        addr = boot_info->kernel + (i << PAGE_SHIFT);
        pte = pte_offset(pde, addr);
        *pte = __mkpte(addr, flags);
    }

    // map kernel stack
    addr = boot_info->stack - PAGE_SIZE;
    pte = pte_offset(pde, addr);
    *pte = __mkpte(addr, flags);

    // map user stack
    addr = USER_STACK_PAGE;
    pte = pte_offset(pde, addr);
    *pte = __mkpte(addr, flags);

    (void) pde;
    (void) pte;
    (void) flags;
    (void) addr;

    // check large page support
    get_cpuid(&cpuid);
    large_page_support = cpuid.pse_support;

    // configure CR4
    if (large_page_support) {
        uint32_t cr4 = 0;
        read_cr4(cr4);
        cr4 |= CR4_PSE; // allow 4M pages
        write_cr4(cr4);
    }
    else {
        kprint("\e[1;31mno large page support!\e[0m\n");    // TODO: deal with this
    }

    // configure CR3
    uint32_t cr3 = 0;
    cr3 |= SYSTEM_PAGE_DIRECTORY;
    write_cr3(cr3);

    // configure CR0
    uint32_t cr0 = 0;
    read_cr0(cr0);
    cr0 |= CR0_PG;      // enable paging
    cr0 |= CR0_WP;      // enable write-protection for supervisor
    write_cr0(cr0);     //   to prevent kernel from writing read-only page
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
    printf("page: v(%08X-%08X) -> p(%08X-%08X) %c %-2s %c %c %c %s%s\n",
        vaddr, vlimit, paddr, plimit,
        page->pde ? (page->ps ? 'M' : 'T') : 'k',       // (k) small page, (M) large page, (T) page table
        page->rw ? "rw" : "r",                          // read/write
        page->us ? 'u' : 's',                           // user/supervisor
        page->a ? (page->d ? 'd' : 'a') : ' ',          // accessed/dirty
        page->g ? 'g' : ' ',                            // global
        page->pwt ? "wt " : "  ",                       // write-through
        page->pcd ? "nc " : "  ");                      // no-cache
}

static void print_page_mappings(struct mm_info *mm)
{
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
}
