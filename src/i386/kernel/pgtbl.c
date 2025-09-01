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
 *         File: i386/kernel/pgtbl.c
 *      Created: July 11, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <kernel/kernel.h>
#include <kernel/mm.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <i386/x86.h>

bool virt_addr_valid(void *va)
{
    pte_t *pte;
    if (!walk_page_table((uint32_t) va, &pte)) {
        return false;
    }

    return pte_present(*pte);
}

bool walk_page_table(uint32_t va, pte_t **pte)
{
    pde_t *pgdir;
    pde_t *pde;

    if (!pte) {
        return false;
    }

    pgdir = (pde_t *) get_pgdir();
    pde = (pde_t *) KERNEL_ADDR(pde_offset(pgdir, va));
    if (!pde_present(*pde)) {
        return false;
    }

    *pte = (pte_t *) KERNEL_ADDR(pte_offset(pde, va));
    return true;
}

pte_t * map_page(uint32_t va, uint32_t pa, pgflags_t flags)
{
    pde_t *pgdir;
    pde_t *pde;
    pte_t *pte;

    pgdir = (pde_t *) get_pgdir();
    pde = (pde_t *) KERNEL_ADDR(pde_offset(pgdir, va));
    if (!pde_present(*pde)) {
        // TODO: map a new PDE and associated page table...
        panic("mm: mappings that require a new new page table not yet implemented! pa(%08X) va(%08X)\n", pa, va);
        return NULL;
    }

    // TODO: check flags
    // TODO: allow map modding by calling again with valid VA?
    //       ...or warn about double-mapping?

    pte = (pte_t *) KERNEL_ADDR(pte_offset(pde, va));
    pte_clear(pte);
    *pte = __mkpte(pa, flags | _PAGE_PRESENT);

    flush_tlb();        // TODO: be able to control this w/ flag
    return pte;
}

bool unmap_page(uint32_t va)
{
    pte_t *pte;
    if (!walk_page_table(va, &pte)) {
        panic("mm: attempt to free a non-mapped page! va(%08X)\n", va);
        return false;
    }

    pte_clear(pte);

    flush_tlb();
    return true;
}
