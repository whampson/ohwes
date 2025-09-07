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
    bool unmap;

    // TODO: these "page mappings" should probably be managed somewhere in a
    // structure if we're going to be "updating" them. So we can disallow
    // duplicate mappings and know how many pages are in each "mapping"

    // ... or we can be hardcore and leave it up to the caller!
    // TODO: allow map modding by calling again with valid VA?
    //       or warn about double-mapping?

    pgdir = (pde_t *) get_pgdir();
    unmap = (flags == 0);
    // TODO: check/validate/filter flags

    if (count > 4096) {
        assert(count <= 4096);
        return;
    }

    for (int i = 0; i < count; i++) {
        pde = (pde_t *) KERNEL_ADDR(pde_offset(pgdir, va));
        if (!pde_present(*pde)) {
            // TODO: map a new PDE and associated page table...
            panic("phys-mem: mappings that require a new PDE and page table not yet implemented! pa(%08X) va(%08X)\n", pa, va);
        }
        pte = (pte_t *) KERNEL_ADDR(pte_offset(pde, va));

        pte_clear(pte);
        if (!unmap) {   // map
            *pte = __mkpte(pa, flags);
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    // TODO: be able to control this w/ flag
    flush_tlb();

    size_t size_bytes = (count << PAGE_SHIFT);
    if (unmap) {
        kprint("phys-mem: unmap p:%08X-%08X v:%08X-%08X size_pages=%d flags=%02Xh\n",
            pa, pa+size_bytes-1, va, va+size_bytes-1, count, flags);
    }
    else {
        kprint("phys-mem: map p:%08X-%08X v:%08X-%08X size_pages=%d flags=%02Xh\n",
            pa, pa+size_bytes-1, va, va+size_bytes-1, count, flags);
    }
}
