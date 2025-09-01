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

void update_page_mappings(uint32_t va, uint32_t pa, size_t count, pgflags_t flags)
{
    pde_t *pgdir;
    pde_t *pde;
    pte_t *pte;
    bool map;

    // TODO: these "page mappings" should probably be managed somewhere in a
    // structure if we're going to be "updating" them. So we can disallow
    // duplicate mappings and know how many pages are in each "mapping"

    // ... or we can be hardcore and leave it up to the caller!
    // TODO: allow map modding by calling again with valid VA?
    //       or warn about double-mapping?

    pgdir = (pde_t *) get_pgdir();
    map = (flags != 0 && pa >= PAGE_SIZE);
    // TODO: check/validate/filter flags

    if (map) kprint("mem: mapping %d pages at PA:%08X VA:%08X flags %02Xh...\n", count, pa, va, flags);
    else     kprint("mem: unmapping %d pages at VA:%08X...\n", count, va);

    for (int i = 0; i < count; i++) {
        pde = (pde_t *) KERNEL_ADDR(pde_offset(pgdir, va));
        if (!pde_present(*pde)) {
            // TODO: map a new PDE and associated page table...
            panic("mem: mappings that require a new PDE and page table not yet implemented! pa(%08X) va(%08X)\n", pa, va);
        }

        pte = (pte_t *) KERNEL_ADDR(pte_offset(pde, va));
        pte_clear(pte);

        if (map) {
            *pte = __mkpte(pa, flags);
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    // TODO: be able to control this w/ flag
    flush_tlb();
}
