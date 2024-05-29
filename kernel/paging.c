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
#include <ohwes.h>
#include <paging.h>
#include <errno.h>

void * get_page_directory(void)
{
    return (void *) PAGE_DIR;
}

void * get_pde(uint32_t addr)
{
    struct pde *pgdir = get_page_directory();
    return &pgdir[get_pdn(addr)];
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
    return get_pfn(addr) & ((PAGE_SIZE / PTE_SIZE) - 1);
}

static int set_page_mapping(
    struct page *page,
    uint32_t pfn,
    bool rdonly, bool user,
    bool pde, bool large)
{
    if (pfn >= (1 << (32 - PAGE_SHIFT))) {
        return -EINVAL;     // PFN is out of range
    }

    if (large && !aligned(pfn << PAGE_SHIFT, LARGE_PAGE_SIZE)) {
        return -EINVAL;     // PFN not valid for large page
    }

    // assert(PAGE_IS_FREE(page));
    if (!PAGE_IS_FREE(page)) {
        return -EINVAL;     // page already mapped! TODO: change error code
    }

    zeromem(page, PTE_SIZE);

    if (!rdonly) {
        page->rw = 1;
    }
    if (user) {
        page->us = 1;
    }

    page->p = 1;
    page->pde = pde;
    page->pspat = pde && large;
    page->pfn = pfn;

    return 0;
}

int map_page(uint32_t addr, uint32_t pfn, int flags)
{
    struct page *pde;
    struct page *pte;
    struct page *pgtbl;

    bool flag_ro = has_flag(flags, MAP_READONLY);
    bool flag_user = has_flag(flags, MAP_USERMODE);
    bool flag_pgtbl = has_flag(flags, MAP_PAGETABLE);
    bool flag_large = has_flag(flags, MAP_LARGE);

    bool large_page = flag_large || flag_pgtbl;

    if (flag_pgtbl && flag_large) {
        return -EINVAL;         // invalid flag combination
    }

    // TODO: ensure pfn does not point to a reserved physical region
    // TODO: make sure CPU supports large pages (>= Pentium, use cpuid)

    pde = get_pde(addr);

    if (large_page) {
        return set_page_mapping(pde, pfn, flag_ro, flag_user, true, flag_large);
    }

    if (PAGE_IS_FREE(pde) || !PAGE_IS_PDE(pde)) {
        return -EINVAL;     // page table is not mapped! TODO: change error code
    }

    pgtbl = (struct page *) (pde->pfn << PAGE_SHIFT);
    pte = &pgtbl[get_ptn(addr)];

    return set_page_mapping(pte, pfn, flag_ro, flag_user, false, false);
}

void init_paging(void)
{
    int ret;
    int flags;

    // zero the system page directory
    struct pde *pgdir = get_page_directory();
    assert(aligned((uint32_t) pgdir, PAGE_SIZE));
    zeromem(pgdir, PAGE_SIZE);

    // zero the kernel's page table;
    // for now we only have one page table that maps to 0-4M,
    // we can allocate new page tables later once the memory
    // subsystem is figured out
    struct pte *pgtbl = (struct pte *) PAGE_TABLE;  // TODO: dynamically alloc
    zeromem(pgtbl, PAGE_SIZE);

    // // API sanity checks
    // assert(map_page(0, 0, MAP_PAGETABLE | MAP_LARGE) == -EINVAL);   // bad flag combination
    // assert(map_page(0x0, 1, MAP_LARGE) == -EINVAL);                 // invalid PFN for large page

    flags = MAP_USERMODE;   // temporarily allow usermode access, TODO: remove

    // map the kernel's page table into the PDE;
    // this divides the 0-4M virtual address region into 1024 4K pages
    ret = map_page(0x0, get_pfn((uint32_t) pgtbl), flags | MAP_PAGETABLE);
    assert(ret == 0);

    // open up the first 640K of RAM, plus the VGA frame buffer;
    // leave 0x0-0x1000 inaccessible so we can catch page faults.
    // we are guaranteed to have at least 640K
    for (int i = 0; i < 1024; i++) {
        if ((i > 0 && i < 0xA0) || (i >= 0xB8 && i <= 0xBF)) {
            ret = map_page(i << PAGE_SHIFT, i, flags);
            assert(ret == 0);
        }
    }

    // TODO: check cpuid for CR4.PSE before mapping large pages

    // list_page_mappings();

    uint32_t cr3 = 0;
    cr3 |= (uint32_t) pgdir;
    write_cr3(cr3);

    // uint32_t cr4 = 0;
    // read_cr4(cr4);
    // cr4 |= CR4_PSE;     // allow 4M pages
    // write_cr4(cr4);     // NOTE: CR4 was introduced on the Pentium! Should do a CPUID before running this

    uint32_t cr0 = 0;
    read_cr0(cr0);
    cr0 |= CR0_PG;      // enable paging
    write_cr0(cr0);
}

#if DEBUG
static void print_page_info(const struct page *page)
{
    uint32_t base = page->pfn << PAGE_SHIFT;
    uint32_t limit = base + PAGE_SIZE - 1;
    if (PAGE_IS_LARGE(page)) {
        limit = base + LARGE_PAGE_SIZE - 1;
    }

    // page: pfn base-limit dt us kM
    //   dt - directory/table entry
    //   us - user/supervisor access
    //   kM - 4k/4M frame
    printf("page: %d %08x-%08x %c %c %c\n",
        page->pfn, base, limit,
        page->pde ? 'd' : 't',
        page->us ? 'u' : 's',
        page->pspat ? 'M' : 'k');

}

static void list_page_mappings(void)
{
    struct page *pgdir = get_page_directory();
    struct page* pgtbl;
    struct page *page;

    for (int i = 0; i < PAGE_SIZE / PDE_SIZE; i++) {
        page = &pgdir[i];
        if (PAGE_IS_FREE(page)) {
            continue;
        }

        if (!PAGE_IS_LARGE(page)) {
            pgtbl = (struct page *) (page->pfn << PAGE_SHIFT);
            for (int j = 0; j < PAGE_SIZE / PTE_SIZE; j++) {
                page = &pgtbl[j];
                if (PAGE_IS_FREE(page)) {
                    continue;
                }
                print_page_info(page);
            }
            continue;
        }
        else {
            print_page_info(page);
        }
    }
}
#endif
