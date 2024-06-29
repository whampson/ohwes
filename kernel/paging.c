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
 *      Created: June 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <paging.h>

// uint32_t get_pfn(uint32_t addr)
// {
//     return addr >> PAGE_SHIFT;
// }

// uint32_t get_pdn(uint32_t addr)
// {
//     return (addr >> PGDIR_SHIFT) & (PDE_COUNT - 1);
// }

// uint32_t get_ptn(uint32_t addr)
// {
//     return (addr >> PAGE_SHIFT) & (PTE_COUNT - 1);
// }

// uint32_t pde_page(pde_t pde)
// {
//     return pde & PAGE_MASK;
// }

pde_t * get_pde(struct mm_info *mm, uint32_t addr)
{
    uint32_t pdn = __pdn(addr);
    return &((pde_t *) mm->pgdir)[pdn];
}

pte_t * get_pte(pde_t *pde, uint32_t addr)
{
    uint32_t pgtbl = pde_page(*pde);    // page table address
    uint32_t ptn = __ptn(addr);         // page table index

    return &((pte_t *) pgtbl)[ptn];
}

pte_t set_pde(pde_t pde, uint32_t addr, uint32_t flags)
{
    pde = 0;
    pde |= _PAGE_PDE;
    pde |= flags & (PAGE_SIZE - 1);
    pde |= __ptn(addr) << PAGE_SHIFT;

    return pde;
}

pte_t set_pte(pte_t pte, uint32_t addr, uint32_t flags)
{
    pte = 0;
    pte |= flags & (PAGE_SIZE - 1);
    pte |= __pfn(addr) << PAGE_SHIFT;

    return pte;
}

pte_t * alloc_pte(pde_t *pde, uint32_t addr, pgflags_t flags)
{
    pte_t *pte = get_pte(pde, addr);
    *pte = set_pte(*pte, addr, flags);

    return pte;
}
