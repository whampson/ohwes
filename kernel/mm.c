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
 *      Created: June 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <boot.h>
#include <cpu.h>
#include <mm.h>
#include <ohwes.h>
#include <paging.h>

struct mm_info kernel_mm;

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

    // kbwait();
}

void print_page_mappings(struct mm_info *mm)
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

void init_mm(const struct boot_info *boot_info)
{
    bool large_page_support;
    struct cpuid cpuid;
    struct mm_info *kmm;
    pde_t *pde;
    pte_t *pte;
    pgflags_t flags;

    kmm = &kernel_mm;

    zeromem(kmm, sizeof(struct mm_info));
    kmm->pgdir = (pde_t *) SYSTEM_PAGE_DIRECTORY;

    zeromem((void *) SYSTEM_PAGE_DIRECTORY, PAGE_SIZE);
    zeromem((void *) KERNEL_PAGE_TABLE, PAGE_SIZE);

    flags = _PAGE_PRESENT | _PAGE_RW;

    pde = get_pde(kmm, KERNEL_PAGE_TABLE);
    // *pde = set_pde(*pde, KERNEL_PAGE_TABLE, _PAGE_TABLE);

    // pte = alloc_pte(pde, SYSTEM_CPU_PAGE, flags);
    // pte = alloc_pte(pde, SYSTEM_MEMORY_PAGE, flags);
    // for (int i = 0; i < boot_info->framebuffer_pages; i++) {
    //     pte = alloc_pte(pde, boot_info->framebuffer + (i << PAGE_SHIFT), flags);
    // }
    // uint32_t num_kernel_code_pages = div_ceil(boot_info->kernel_size, PAGE_SIZE);
    // for (int i = 0; i < num_kernel_code_pages; i++) {
    //     pte = alloc_pte(pde, boot_info->kernel + (i << PAGE_SHIFT), flags);
    // }
    // uint32_t stack_page = boot_info->stack - PAGE_SIZE;
    // pte = alloc_pte(pde, stack_page, flags);

    (void) pde;
    (void) pte;
    (void) flags;

    flags = _PAGE_RW | _PAGE_USER;
    pde = get_pde(kmm, KERNEL_PAGE_TABLE);
    *pde = __mkpde(KERNEL_PAGE_TABLE, flags);

    printf("pde bad: %s\n", YN(pde_bad(*pde)));
    printf("pde large: %s\n", YN(pde_large(*pde)));
    printf("pde index: %d\n", pde_index(*pde));
    printf("pde page: 0x%X\n", pde_page(*pde));


    pte = get_pte(pde, KERNEL_PAGE_TABLE);
    *pte = __mkpte(KERNEL_PAGE_TABLE, flags);
    printf("pte index: %d\n", pte_index(*pte));
    printf("pte page: 0x%X\n", pte_page(*pte));
    printf("pte read: %s\n", YN(pte_read(*pte)));
    printf("pte exec: %s\n", YN(pte_exec(*pte)));
    printf("pte write: %s\n", YN(pte_write(*pte)));
    printf("pte user: %s\n", YN(pte_user(*pte)));
    printf("pte dirty: %s\n", YN(pte_dirty(*pte)));

    print_page_mappings(kmm);

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
        panic("no large page support!");    // TODO: deal with this
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
    // write_cr0(cr0);     //   to prevent kernel from writing read-only page

    // // *((volatile uint32_t *) NULL) = 0xBADC0D3;
}
