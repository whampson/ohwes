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
 *         File: include/paging.h
 *      Created: June 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

// Inspiration:
// https://www.kernel.org/doc/gorman/html/understand/understand006.html

#ifndef __PAGING_H
#define __PAGING_H

#define PAGE_SHIFT              12
#define PAGE_SIZE               (1 << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#define PGDIR_SHIFT             22
#define PGDIR_SIZE              (1 << PGDIR_SHIFT)
#define PGDIR_MASK              (~(PGDIR_SIZE - 1))

#define LARGE_PAGE_SHIFT        PGDIR_SHIFT
#define LARGE_PAGE_SIZE         (1 << LARGE_PAGE_SHIFT)
#define LARGE_PAGE_MASK         (~(LARGE_PAGE_SIZE - 1))

#define PDE_COUNT               1024                // PDEs per page directory
#define PTE_COUNT               1024                // PTEs per page table

// TODO: move these elsewhere
#define KB_SHIFT                10
#define MB_SHIFT                20
#define GB_SHIFT                30

#define KB                      (1 << KB_SHIFT)
#define MB                      (1 << MB_SHIFT)
#define GB                      (1 << GB_SHIFT)

//   10987654321098765432109876543210
//  +---------+---------+-----------+
//  |   PDN   |   PTN   |  OFFSET   | Linear Address
//  +---------+---------+-----------+
//  |        PFN        | ATTR BITS | pte_t/pde_t
//  +---------+---------+-----------+
//
// PDN = Page Directory Number  offset of pde_t in page directory
// PTN = Page Table Number      offset of pte_t in page table
// PFN = Page Frame Number      physical page number

#define __ptn(addr)             (((uintptr_t) (addr) >> PAGE_SHIFT) & (PTE_COUNT - 1))
#define __pdn(addr)             (((uintptr_t) (addr) >> PGDIR_SHIFT) & (PDE_COUNT - 1))
#define __pfn(addr)             ((uintptr_t)  (addr) >> PAGE_SHIFT)

#define PAGE_ALIGN(addr)        ((uintptr_t) ((addr) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define LARGE_PAGE_ALIGN(addr)  ((uintptr_t) ((addr) + (LARGE_PAGE_SIZE - 1)) & LARGE_PAGE_MASK)

//
// Page Attribute Flags
//
#define _PAGE_PRESENT           (1 << 0)        // present in memory
#define _PAGE_RW                (1 << 1)        // read/write accessible
#define _PAGE_USER              (1 << 2)        // user accessible
#define _PAGE_PWT               (1 << 3)        // cache: write-through
#define _PAGE_PCD               (1 << 4)        // cache: disable
#define _PAGE_ACCESSED          (1 << 5)        // page accessed
#define _PAGE_DIRTY             (1 << 6)        // page written
#define _PAGE_PS                (1 << 7)        // 4M page (PDEs); PAT (PTEs)
#define _PAGE_GLOBAL            (1 << 8)        // TLB pinned
#define _PAGE_PDE               (1 << 9)        // this is a PDE
#define _PAGE_LARGE             (_PAGE_PS)

#if !defined(__ASSEMBLER__) && !defined(__LDSCRIPT__)

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t pte_t;
typedef uint32_t pde_t;
typedef uint32_t pgflags_t;

//
// Combined x86 PDE/PTE.
// Designed to map onto a `struct x86_pde` or `struct x86_pte` or be used cast
// from a `pde_t` or `pte_t`. Useful for debugging.
//
struct pginfo {
    union {
        struct {
            uint32_t p   : 1;   // Present: page mapping present in memory
            uint32_t rw  : 1;   // Read/Write: 1 = writable
            uint32_t us  : 1;   // User/Supervisor: 1 = user accessible
            uint32_t pwt : 1;   // Page-Level Cache Write-Through
            uint32_t pcd : 1;   // Page-Level Cache Disable
            uint32_t a   : 1;   // Accessed: software has accessed this page
            uint32_t d   : 1;   // Dirty: software has written this page
            uint32_t ps  : 1;   // Page Size: 1=4M, 0=4K (PDEs only, requires CR4.PSE=1)
            uint32_t g   : 1;   // Global: pins page to TLB (requires CR4.PGE=1)
            uint32_t pde : 1;   // PDE: this is a PDE (OH-WES addition)
            uint32_t     : 2;   // (available for use)
            uint32_t pfn : 20;  // Page Frame Number
        };
        uint32_t _value;
    };
};
static_assert(sizeof(struct pginfo) == sizeof(uint32_t), "bad size!");

#define PTE_NONE                    0
#define PDE_NONE                    0

#define __pte(x)                    ((pte_t) (x))
#define __pde(x)                    ((pde_t) (x))
#define __pgflags(x)                ((pgflags_t) (x))
#define __pginfo(x)                 ((struct pginfo) { ._value = (x) })

#define __mkpde(addr,flags)         __pde(      PAGE_ALIGN(addr) | __pgflags((flags)|_PAGE_PRESENT|_PAGE_PDE))
#define __mkpde_large(addr,flags)   __pde(LARGE_PAGE_ALIGN(addr) | __pgflags((flags)|_PAGE_PRESENT|_PAGE_PDE|_PAGE_LARGE))

#define __mkpte(addr,flags)         __pte(PAGE_ALIGN(addr) | __pgflags((flags)|_PAGE_PRESENT))

// Future-proofing a bit here...
// Intel paging structures are always read/execute in kernel mode so long as the
// present bit is set. We're going to treat these flags from the perspective of
// a user process, i.e. we can disable read/execute if we make the page
// accessible only by the kernel. Other architectures may be different.

static inline uint32_t pde_index(pde_t pde)     { return __pdn(pde); }
static inline void * pde_page(pde_t pde)        { return (void *) (pde & PAGE_MASK); }

static inline bool pde_none(pde_t pde)          { return !pde; }
static inline void pde_clear(pde_t *pde)        { *pde = 0; }

static inline pde_t * pde_offset(pde_t *pde, uint32_t va)
{
    return pde + __pdn(va);
}

static inline bool pde_read(pde_t pde)          { return (pde & _PAGE_USER) == _PAGE_USER; }
static inline bool pde_exec(pde_t pde)          { return (pde & _PAGE_USER) == _PAGE_USER; }
static inline bool pde_write(pde_t pde)         { return (pde & _PAGE_RW) == _PAGE_RW; }
static inline bool pde_user(pde_t pde)          { return (pde & _PAGE_USER) == _PAGE_USER; }
static inline bool pde_dirty(pde_t pde)         { return (pde & _PAGE_DIRTY) == _PAGE_DIRTY; }
static inline bool pde_large(pde_t pde)         { return (pde & _PAGE_LARGE) == _PAGE_LARGE; }
static inline bool pde_present(pde_t pde)       { return (pde & _PAGE_PRESENT) == _PAGE_PRESENT; }
static inline bool pde_bad(pde_t pde)   // i.e. misconfigured
{
    return ((pde & (_PAGE_PDE|_PAGE_PRESENT)) != (_PAGE_PDE|_PAGE_PRESENT))
        || (pde_large(pde) && __ptn(pde) != 0);
}

static inline pde_t pde_mkread(pde_t pde)       { pde |= _PAGE_USER; return pde; }
static inline pde_t pde_mkexec(pde_t pde)       { pde |= _PAGE_USER; return pde; }
static inline pde_t pde_mkwrite(pde_t pde)      { pde |= _PAGE_RW; return pde; }
static inline pde_t pde_mkuser(pde_t pde)       { pde |= _PAGE_USER; return pde; }
static inline pde_t pde_mkdirty(pde_t pde)      { pde |= _PAGE_DIRTY; return pde; }
static inline pde_t pde_mkclean(pde_t pde)      { pde &= ~_PAGE_DIRTY; return pde; }
static inline pde_t pde_mkpresent(pde_t pde)    { pde &= _PAGE_PRESENT; return pde; }

static inline pde_t pde_rdprotect(pde_t pde)    { pde &= ~_PAGE_USER; return pde; }
static inline pde_t pde_exprotect(pde_t pde)    { pde &= ~_PAGE_USER; return pde; }
static inline pde_t pde_wrprotect(pde_t pde)    { pde &= ~_PAGE_RW; return pde; }

// ------------------------------------------------------------------------------------------------

static inline uint32_t pte_index(pte_t pte)     { return __ptn(pte); }
static inline void * pte_page(pte_t pte)        { return (void *) (pte & PAGE_MASK); }

static inline bool pte_none(pte_t pte)          { return !pte; }
static inline void pte_clear(pte_t *pte)        { *pte = 0; }

static inline pte_t * pte_offset(pde_t *pde, uint32_t va)
{
    return ((pte_t *) pde_page(*pde)) + __ptn(va);
}

static inline bool pte_read(pte_t pte)          { return (pte & _PAGE_USER) == _PAGE_USER; }
static inline bool pte_exec(pte_t pte)          { return (pte & _PAGE_USER) == _PAGE_USER; }
static inline bool pte_write(pte_t pte)         { return (pte & _PAGE_RW) == _PAGE_RW; }
static inline bool pte_user(pte_t pte)          { return (pte & _PAGE_USER) == _PAGE_USER; }
static inline bool pte_dirty(pte_t pte)         { return (pte & _PAGE_DIRTY) == _PAGE_DIRTY; }
static inline bool pte_present(pte_t pte)       { return (pte & _PAGE_PRESENT) == _PAGE_PRESENT; }

static inline pte_t pte_mkread(pte_t pte)       { pte |= _PAGE_USER; return pte; }
static inline pte_t pte_mkexec(pte_t pte)       { pte |= _PAGE_USER; return pte; }
static inline pte_t pte_mkwrite(pte_t pte)      { pte |= _PAGE_RW; return pte; }
static inline pte_t pte_mkuser(pte_t pte)       { pte |= _PAGE_USER; return pte; }
static inline pte_t pte_mkdirty(pte_t pte)      { pte |= _PAGE_DIRTY; return pte; }
static inline pte_t pte_mkclean(pte_t pte)      { pte &= ~_PAGE_DIRTY; return pte; }
static inline pte_t pte_mkpresent(pte_t pte)    { pte &= _PAGE_PRESENT; return pte; }

static inline pte_t pte_rdprotect(pte_t pte)    { pte &= ~_PAGE_USER; return pte; }
static inline pte_t pte_exprotect(pte_t pte)    { pte &= ~_PAGE_USER; return pte; }
static inline pte_t pte_wrprotect(pte_t pte)    { pte &= ~_PAGE_RW; return pte; }

#endif  // __ASSEMBLER__

#endif  // __PAGING_H
