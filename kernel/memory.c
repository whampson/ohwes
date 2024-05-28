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

/*
 Physical Address Map:

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
   9FC00 +----------------------------+ 639K (this address can vary by hardware)
         |                            |
         | Kernel Stack               |
         |                            |
(varies) +----------------------------+ (varies)
         |                            |
         |                            |
         | (free)                     |
         |                            |
         |                            |
(varies) +----------------------------+ (varies)
         |                            |
         | Kernel Code                |
         |                            |
   20000 +----------------------------+ 128K
         |                            |
         | DMA Buffer                 |
         |                            |
   10000 +----------------------------+ 64K
         |                            |
         | (free)                     |
         |                            |
    C000 +----------------------------+ 52K
         | Console 3 Input Buffer     |
    B000 +----------------------------+ 48K
         | Console 3 Frame Buffer     |
    A000 +----------------------------+ 40K
         | Console 2 Input Buffer     |
    9000 +----------------------------+ 36K
         | Console 2 Frame Buffer     |
    8000 +----------------------------+ 32K
         | Console 1 Input Buffer     |
    7000 +----------------------------+ 28K
         | Console 1 Frame Buffer     |
    6000 +----------------------------+ 24K
         | Console 0 Input Buffer     |
    5000 +----------------------------+ 20K
         | Console 0 Frame Buffer     |
    4000 +----------------------------+ 16K
         | Kernel Page Table (0-4M)   |
    3000 +----------------------------+ 12K
         | System Page Directory      |
    2000 +----------------------------+ 8K
         | IDT/GDT/LDT/TSS            |
    1000 +----------------------------+ 4K
         | Real Mode IVT and BDA      |
       0 +============================+ 0K

NOTES:
    * Console frame buffer assumed to be 80x25 (2 bytes per char), therefore it
      fits within one page (4000 bytes, 96 extra bytes)
    * VGA frame buffer hardware is mapped at 0xA0000-0xBFFFF. Which chunk of
      this range is used depends on the VGA hardware configuration (see vga.c).
      For now, we are using the region from 0xB8000-0xBFFFF (CGA).
*/

#define PAGE_DIR    0x2000
#define PAGE_TABLE  0x3000

static void print_meminfo(const struct boot_info *info);

#define READ_ONLY   (1 << 0)
#define USER_ACCESS (1 << 2)
#define LARGE_PAGE  (1 << 31)

struct page
{
    uint32_t p      : 1;    // Present; 0 = page is free
    uint32_t rw     : 1;    // Read/Write; 1 = writable
    uint32_t us     : 1;    // User/Supervisor; 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed; software has accessed this page
    uint32_t d      : 1;    // Dirty; software has written this page
    uint32_t ps     : 1;    // Page Size; 1 = 4M, 0 = 4K
    uint32_t g      : 1;    // Global; pins page to TLB (requires CR4.PGE=1)
    uint32_t        : 1;    // (available for use)
    uint32_t large  : 1;    // Large page; this is a PDE (OH-WES specific)
    uint32_t rsvd   : 1;    // Reserved; page is inallocable (OH-WES specific)
    uint32_t address: 20;   // Address of page (4K-aligned)
};

// PDE vs PTE determination:
//   PDEs will always have the 'large' bit set. To determine if the PDE points
//   to a large page or a 4K page table, check whether the PS bit is set. Note
//   that for 4K pages, the PS bit is repurposed as the PAT bit, which should
//   be left zero (for now).

#define align(x, n)     (((x) + (n) - 1) & ~((n) - 1))
#define aligned(x,n)    ((x) == align(x,n))

int map_page(uint32_t vaddr, uint32_t paddr, int flags)
{
    struct page *pgdir;
    struct page *pgtbl;
    struct page *pg;
    uint32_t pdn, pfn;

    if (!aligned(vaddr, PAGE_SIZE) || !aligned(paddr, PAGE_SIZE)) {
        return -EINVAL;
    }

    if (flags & LARGE_PAGE) {   // large pages not supported yet
        return -EINVAL;
    }

    pgdir = (struct page *) PAGE_DIR;
    pdn = vaddr >> LARGE_PAGE_SHIFT;    // page directory number
    pfn = (vaddr & (LARGE_PAGE_SIZE-1)) >> PAGE_SHIFT;
    assert(pdn <= 1024);
    assert(pfn <= 1024);

    kprint("pdn = 0x%X, pfn = 0x%X\n", pdn, pfn);

    pg = &pgdir[pdn];
    if (pg->rsvd) {
        // entire 4M region is reserved, fail
        return -1;
    }

    if (!pg->p) {
        // TODO: PDE is free, but we need to allocate a page table!! fix this later
        return -1;
        // // page is not present, i.e. free, use it!
        // zeromem(pg, sizeof(struct page));
        // pg->p = 1;  // present (in-use)
        // pg->us = 1; // user-accessible (TODO: make param)
        // pg->rw = 1; // writable (TODO: make param)
        // pg->address = paddr >> PAGE_SHIFT;
    }

    assert(pg->large);

    if (pg->ps) {
        // PDE points to a 4M page that'a already mapped, fail!
        return -1;
    }

    // now we have a PDE that's present, not reserved, and points to 4K PTE
    // locate the PTE
    pgtbl = (struct page *) (pg->address << PAGE_SHIFT);
    pg = &pgtbl[pfn];

    // repeat the bit-checking process
    if (pg->rsvd) {
        // page is reserved, fail!
        return -1;
    }
    if (pg->p) {
        // page is already mapped, fail!
        return -1;
    }
    assert(!pg->large);

    // we have a non-present PTE
    zeromem(pg, sizeof(struct page));
    pg->p = 1;
    pg->us = 1;
    pg->rw = 1;
    pg->address = paddr >> PAGE_SHIFT;

    kprint("mmap: virt 0x%08X -> phys 0x%08X\n", vaddr, paddr);

    return 0;
}

void init_memory(const struct boot_info *info)
{
    print_meminfo(info);

    // zero the system page directory
    struct pde *pgdir = (struct pde *) PAGE_DIR;
    zeromem(pgdir, PAGE_SIZE);

    // zero the kernel's page table.
    // for now we only have one page table that maps to 0-4M,
    // we can allocate new page tables later once the memory
    // subsystem is figured out.
    struct pte *pgtbl = (struct pte *) PAGE_TABLE;
    zeromem(pgtbl, PAGE_SIZE);

    // put our kernel page table in the page directory.
    // for now we are using a 1:1 mapping, eventally I want to
    // put the kernel up high, at 0xFC000000 or something
    pgdir[0].p = 1;     // present
    pgdir[0].rw = 1;    // writable
    pgdir[0].us = 1;    // user-accessible (for now)
    pgdir[0].ps = 0;    // point to a 4k page table
    pgdir[0].address = PAGE_TABLE >> PAGE_SHIFT;
    ((struct page *) pgdir)[0].large = 1;

    // open up the first 640K of RAM, plus the VGA frame buffer,
    // leave 0x0-0x1000 inaccessable so we can catch page faults.
    // we are guaranteed to have at least 640K
    for (int i = 0; i < 1024; i++) {
        if ((i > 0 && i < 0xA0) || (i >= 0xB8 && i <= 0xBF)) {
            pgtbl[i].p = 1;     // present
            pgtbl[i].rw = 1;    // writable
            pgtbl[i].us = 1;    // user-accessible (for now)
            pgtbl[i].address = i;
        }
    }

    uint32_t cr3 = 0;
    cr3 |= PAGE_DIR;
    // cr3 |= CR3_PCD;     // disable cache
    // cr3 |= CR3_PWT;     // write-through (vs write-back)
    load_cr3(cr3);

    uint32_t cr0 = 0;
    store_cr0(cr0);
    cr0 |= CR0_PG;      // enable paging
    load_cr0(cr0);


    map_page(0x100000, 0x100000, 0);
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
