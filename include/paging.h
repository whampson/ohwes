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
 *         File: include/paging.h
 *      Created: May 27, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __PAGING_H
#define __PAGING_H

#include <stdint.h>

#define PAGE_DIR            0x2000
#define PAGE_TABLE          0x3000  // TODO: dyn alloc this

#define PAGE_SHIFT          12
#define PAGE_SIZE           (1 << PAGE_SHIFT)

#define LARGE_PAGE_SHIFT    22
#define LARGE_PAGE_SIZE     (1 << LARGE_PAGE_SHIFT)

#define PDE_SIZE            4
#define PTE_SIZE            4

//
// Mapping Flags
//
#define MAP_READONLY    (1 << 0)    // Read-only page
#define MAP_USERMODE    (1 << 1)    // User accessible page
#define MAP_GLOBAL      (1 << 2)    // Global page
#define MAP_PAGETABLE   (1 << 30)   // Page table
#define MAP_LARGE       (1 << 31)   // Large (4M) page

//
// Combined x86 4K and 4M PDE/PTE.
// Designed to map onto a `struct pde` or `struct pte`
//
struct page
{
    uint32_t p      : 1;    // Present: page mapping in use
    uint32_t rw     : 1;    // Read/Write: 1 = writable
    uint32_t us     : 1;    // User/Supervisor: 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed: software has accessed this page
    uint32_t d      : 1;    // Dirty: software has written this page
    uint32_t pspat  : 1;    // Page Size: 1 = 4M, 0 = 4K (PDE); Page Attribute Table (PTE)
    uint32_t g      : 1;    // Global: pins page to TLB (requires CR4.PGE=1)
    uint32_t pte    : 1;    // PTE: this is a page table leaf (OH-WES addition)
    uint32_t        : 2;    // (available for use)
    uint32_t pfn    : 20;   // Page Frame Number
};
static_assert(sizeof(struct page) == sizeof(uint32_t), "bad PDE/PTE size!");

#define PAGE_IS_MAPPED(pg)  ((pg)->p)                   // Page is mapped
#define PAGE_IS_PTE(pg)     ((pg)->pte)                 // Page is a PTE (leaf)
#define PAGE_IS_LARGE(pg)   (!(pg)->pte && (pg)->pspat) // Page is a PDE that maps to a 4M region

/**
 * Maps a virtual address region to a physical page. The physical page is
 * specified by a page frame number (PFN), which is a number indexing physical
 * memory as a contiguous block of `PAGE_SIZE`-sized and -aligned chunks.
 *
 * @param addr base virtual address
 * @param pfn physical page frame number
 * @param flags mapping flags
 *
 * @return `0` if successful;
 *
 *         `EINVAL` if
 *           the desired virtual address is not aligned to a page boundary,
 *           both the MAP_LARGE and MAP_PAGETABLE flags are used;
 *
 *         `ENOMEM` if
 *           the desired virtual address is already in use,
 *           a page table for the desired virtual address does not exist,
 *           an attempt is made to map a large page when large pages are not
 *           supported by the hardware
 */
int map_page(uint32_t addr, uint32_t pfn, int flags);

/**
 * Unmaps a virtual address region.
 *
 * @param addr base virtual address
 * @param flags mapping flags
 *
 * @return `0` if successful;
 *
 *         `EINVAL` if
 *           the desired virtual address is not aligned to a page boundary,
 *           both the MAP_LARGE and MAP_PAGETABLE flags are used;
 *
 *         `ENOMEM` if
 *           the desired virtual address is not mapped,
 *           a page table for the desired virtual address does not exist
 */
int unmap_page(uint32_t addr, int flags);

void * get_page_directory(void);
void * get_pde(uint32_t addr);      // page directory entry
void * get_pte(uint32_t addr);      // page table entry

uint32_t get_pfn(uint32_t addr);    // page file number (bits 31:12)
uint32_t get_pdn(uint32_t addr);    // page directory number (bits 31:22)
uint32_t get_ptn(uint32_t addr);    // page table number (bits 21:12)

void list_page_mappings(void);

#endif // __PAGING_H
