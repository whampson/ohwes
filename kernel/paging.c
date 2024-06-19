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
 *         File: kernel/paging.c
 *      Created: May 27, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <boot.h>
#include <cpu.h>
#include <ohwes.h>
#include <paging.h>
#include <errno.h>

struct paging_info
{
    bool large_page_support;
};
struct paging_info _pginfo;
struct paging_info *g_paging_info = &_pginfo;

static void init_page_mappings(const struct boot_info *boot_info, uint32_t pgtbl)
{
    // map page table (addressability: 0-4M)
    // TODO: USERMODE needed because we have some usermode pages in here for
    // init.exe and we  haven't given init.exe it's own page table yet...
    map_page(0x0, get_pfn(pgtbl), MAP_PAGETABLE | MAP_USERMODE);
    if (identity_map(pgtbl, 0) < 0) {
        panic("failed to map kernel page table!");
    }

    // map system page directory
    if (identity_map(SYSTEM_PAGE_DIRECTORY, 0) < 0) {
        panic("failed to map page directory!");
    }

    // map GDT/IDT/LDT/TSS etc.
    if (identity_map(SYSTEM_CPU_PAGE, 0) < 0) {
        panic("failed to map CPU page!");
    }

    // map memory info area
    if (identity_map(SYSTEM_MEMORY_PAGE, 0) < 0) {
        panic("failed to map memory info page!");
    }

    // map video frame buffer
    uint32_t framebuf_pages = boot_info->framebuffer_pages;
    assert(boot_info->framebuffer == SYSTEM_FRAME_BUFFER);
    for (int i = 0; i < framebuf_pages; i++) {
        uint32_t va = boot_info->framebuffer + (i << PAGE_SHIFT);
        if (identity_map(va, 0) < 0) {
            panic("failed to map frame buffer page!");
        }
    }

    // map kernel code
    uint32_t num_kernel_code_pages = div_ceil(boot_info->kernel_size, PAGE_SIZE);
    assert(boot_info->kernel == KERNEL_BASE);
    for (int i = 0; i < num_kernel_code_pages; i++) {
        uint32_t va = boot_info->kernel + (i << PAGE_SHIFT);
        if (identity_map(va, 0) < 0) {
            panic("failed to map kernel code page!");
        }
    }

    // map kernel stack
    uint32_t stack_page = boot_info->stack - PAGE_SIZE;
    assert(stack_page == KERNEL_STACK_PAGE);
    if (identity_map(stack_page, 0) < 0) {
        panic("failed to map kernel stack page!");
    }

#if TEST_BUILD
    // map test code
    uint32_t num_test_code_pages = (2 << 16) >> PAGE_SHIFT;
    for (int i = 0; i < num_test_code_pages; i++) {
        uint32_t va = TEST_BASE + (i << PAGE_SHIFT);
        if (identity_map(va, MAP_USERMODE) < 0) {
            panic("failed to map test code pages!");
        }
    }
#else
    // map user code
    uint32_t num_user_code_pages = div_ceil(boot_info->init_size, PAGE_SIZE);
    for (int i = 0; i < num_user_code_pages; i++) {
        uint32_t va = INIT_BASE + (i << PAGE_SHIFT);
        if (identity_map(va, MAP_USERMODE) < 0) {
            panic("failed to map user code pages!");
        }
    }
#endif

    // map user stack page
    if (identity_map(USER_STACK_PAGE, MAP_USERMODE) < 0) {
        panic("failed to map user stack page!");
    }

    // TODO: configure GDT to reflect kernel and user data/code/stack pages
}

void init_paging(const struct boot_info *boot_info, uint32_t pgtbl)
{
    // clear paging info
    zeromem(g_paging_info, sizeof(struct paging_info));

    // zero the system page directory
    void *pgdir = get_page_directory();
    assert(aligned((uint32_t) pgdir, PAGE_SIZE));
    zeromem(pgdir, PAGE_SIZE);

    // zero the kernel page table
    assert(aligned(pgtbl, PAGE_SIZE));
    zeromem((void *) pgtbl, PAGE_SIZE);

    // // TODO: page mapping API sanity checks
    // assert(map_page(0, 0, MAP_PAGETABLE | MAP_LARGE) == -EINVAL);   // bad flag combination
    // assert(map_page(0x0, 1, MAP_LARGE) == -EINVAL);                 // invalid PFN for large page

    // map the pages necessary to continue code execution
    init_page_mappings(boot_info, pgtbl);

    // check large page support
    struct cpuid cpuid;
    get_cpuid(&cpuid);
    g_paging_info->large_page_support = cpuid.pse_support;

    // configure CR4
    if (large_page_support()) {
        uint32_t cr4 = 0;
        read_cr4(cr4);
        cr4 |= CR4_PSE;         // allow 4M pages
        write_cr4(cr4);
    }

    // configure CR3
    uint32_t cr3 = 0;
    cr3 |= (uint32_t) pgdir;    // set page directory address
    write_cr3(cr3);

    // configure CR0
    uint32_t cr0 = 0;
    read_cr0(cr0);
    cr0 |= CR0_PG;              // enable paging
    cr0 |= CR0_WP;              // enable write-protection for supervisor
    write_cr0(cr0);             //   to prevent kernel from writing read-only page
}

bool large_page_support(void)
{
    return g_paging_info->large_page_support;
}

void * get_page_directory(void)
{
    return (void *) SYSTEM_PAGE_DIRECTORY;
}

void * get_pde(uint32_t addr)
{
    struct page *pgdir = get_page_directory();
    return &pgdir[get_pdn(addr)];
}

void * get_pte(uint32_t addr)
{
    struct page *pde = get_pde(addr);
    if (!PAGE_IS_MAPPED(pde)) {
        return NULL;    // page table is not mapped!
    }

    struct page *pgtbl = (struct page *) (pde->pfn << PAGE_SHIFT);
    return &pgtbl[get_ptn(addr)];
}

uint32_t get_pfn(uint32_t addr)
{
    return addr >> PAGE_SHIFT;
}

uint32_t get_pdn(uint32_t addr)
{
    return addr >> LARGE_PAGE_SHIFT;
}

uint32_t get_ptn(uint32_t addr)
{
    return (addr >> PAGE_SHIFT) & 0x3FF;
}

static int set_page_mapping(
    struct page *page,
    uint32_t pfn,
    uint32_t flags,
    uint32_t pte)
{
    bool flag_ro = has_flag(flags, MAP_READONLY);
    bool flag_user = has_flag(flags, MAP_USERMODE);
    bool flag_pgtbl = has_flag(flags, MAP_PAGETABLE);
    bool flag_large = has_flag(flags, MAP_LARGE);
    bool flag_global = has_flag(flags, MAP_GLOBAL);

    // TODO: ensure pfn does not point to a reserved physical region

    if (pfn > 0xFFFFF) {
        panic("invalid PFN");
    }

    if (flag_large && !aligned(pfn << PAGE_SHIFT, LARGE_PAGE_SIZE)) {
        return -EINVAL;     // PFN not valid for large page
    }

    if (flag_large && flag_pgtbl) {
        return -EINVAL;     // invalid flag combination
    }

    if (pte && (flag_large || flag_pgtbl)) {
        return -EINVAL;     // flags are not valid for PTE mappings
    }

    if (PAGE_IS_MAPPED(page)) {
        return -ENOMEM;     // page is already mapped!
    }

    if (flag_large && !large_page_support()) {
        return -ENOMEM;     // CPU does not support large pages!
    }

    zeromem(page, sizeof(struct page));

    if (!flag_ro) {
        page->rw = 1;
    }
    if (flag_user) {
        page->us = 1;
    }

    page->p = 1;                        // mark present (mapped)
    page->pte = pte;                    // indicate PDE/PTE
    page->pspat = !pte && flag_large;   // for PDEs: 1=4M (large) page, 0=4K table
    page->pfn = pfn;                    // page file number
    page->g = flag_global;              // global flag (TLB pinned)

    return 0;
}

int map_page(uint32_t addr, uint32_t pfn, int flags)
{
    struct page *pde;
    struct page *pte;

    // check alignment
    if (!aligned(addr, PAGE_SIZE)) {
        return -EINVAL;     // address not aligned to a page boundary!
    }

    // grab the corresponding PDE
    pde = get_pde(addr);
    if (PAGE_IS_PTE(pde)) {
        panic("expected PDE marked PTE!");
    }

    // are we trying to map a large page or a page table?
    if (has_flag(flags, MAP_LARGE) || has_flag(flags, MAP_PAGETABLE)) {
        // map the large page or page table into the page directory
        return set_page_mapping(pde, pfn, flags, false);
    }

    // grab the corresponding PTE
    pte = get_pte(addr);
    if (pte == NULL) {
        return -ENOMEM;     // page table is not mapped!
    }

    // map it!
    return set_page_mapping(pte, pfn, flags, true);
}

static int clear_page_mapping(struct page* page, int flags)
{
    bool flag_pgtbl = has_flag(flags, MAP_PAGETABLE);
    bool flag_large = has_flag(flags, MAP_LARGE);

    if (flag_pgtbl && flag_large) {
        return -EINVAL;     // invalid flag combination
    }

    if (!PAGE_IS_MAPPED(page)) {
        return -ENOMEM;     // page not mapped!
    }

    // TODO: INVLPG (486+ only)

    page->p = 0;            // clear present bit (unmap)
    return 0;
}

int unmap_page(uint32_t addr, int flags)
{
    struct page *pde;
    struct page *pte;

    // check alignment
    if (!aligned(addr, PAGE_SIZE)) {
        return -EINVAL;     // address not aligned to a page boundary!
    }

    // grab the corresponding PDE
    pde = get_pde(addr);
    if (PAGE_IS_PTE(pde)) {
        panic("expected PDE marked PTE!");
    }

    // are we unmapping a large page or page table?
    if (has_flag(flags, MAP_LARGE) || has_flag(flags, MAP_PAGETABLE)) {
        // unmap from page directory
        return clear_page_mapping(pde, flags);
    }

    // grab the corresponding PTE
    pte = get_pte(addr);
    if (pte == NULL) {
        return -ENOMEM;     // page table is not mapped!
    }

    // unmap
    return clear_page_mapping(pte, flags);
}

// #if DEBUG
static void print_page_info(uint32_t vaddr, const struct page *page)
{
    uint32_t paddr = page->pfn << PAGE_SHIFT;
    uint32_t plimit = paddr + PAGE_SIZE - 1;
    uint32_t vlimit = vaddr + PAGE_SIZE - 1;
    if (!PAGE_IS_PTE(page)) {
        if (PAGE_IS_LARGE(page)) {
            plimit = paddr + LARGE_PAGE_SIZE - 1;
        }
        vlimit = vaddr + LARGE_PAGE_SIZE - 1;
    }

    //            vaddr-vlimit -> paddr-plimit k/M/T rw u/s a/d g wt nc
    printf("page: v(%08X-%08X) -> p(%08X-%08X) %c %-2s %c %c %c %s%s\n",
        vaddr, vlimit, paddr, plimit,
        page->pte ? 'k' : (page->pspat ? 'M' : 'T'),    // (k) small page, (M) large page, (T) page table
        page->rw ? "rw" : "r",                          // read/write
        page->us ? 'u' : 's',                           // user/supervisor
        page->a ? (page->d ? 'd' : 'a') : ' ',          // accessed/dirty
        page->g ? 'g' : ' ',                            // global
        page->pwt ? "wt " : "  ",                       // write-through
        page->pcd ? "nc " : "  ");                      // no-cache

    // kbwait();
}

void print_page_mappings(void)
{
    struct page *pgdir = get_page_directory();
    struct page* pgtbl;
    struct page *page;
    uint32_t vaddr;

    for (int i = 0; i < PAGE_SIZE / PDE_SIZE; i++) {
        page = &pgdir[i];
        if (!PAGE_IS_MAPPED(page)) {
            continue;
        }

        vaddr = i << LARGE_PAGE_SHIFT;
        print_page_info(vaddr, page);

        if (PAGE_IS_LARGE(page)) {
            continue;
        }

        pgtbl = (struct page *) (page->pfn << PAGE_SHIFT);
        for (int j = 0; j < PAGE_SIZE / PTE_SIZE; j++) {
            page = &pgtbl[j];
            if (!PAGE_IS_MAPPED(page)) {
                continue;
            }
            vaddr = (i << LARGE_PAGE_SHIFT) | (j << PAGE_SHIFT);
            print_page_info(vaddr, page);
        }
    }
}
// #endif
