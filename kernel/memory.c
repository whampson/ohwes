/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/memory.c                                                   *
 * Created: December 19, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <nb/memory.h>
#include <nb/kernel.h>
#include <nb/init.h>
#include <nb/acpi.h>
#include <x86/paging.h>
#include <x86/cntrl.h>
#include <drivers/vga.h>

#define p64(x) (uint32_t)((x) >> 32), (uint32_t)((x) & 0xFFFFFFFF)

struct pgdir_entry *g_pgdir   = (struct pgdir_entry *) PGDIR;
struct pgtbl_entry *g_pgtbl0  = (struct pgtbl_entry *) PGTBL0;

void mem_init(void)
{
    uint16_t above1M;           /* 1K blocks */
    uint16_t above1M_e801;      /* 1K blocks */
    uint16_t above16M;          /* 64K blocks */
    struct smap_entry *smap;    /* a nice table */
    bool has_smap;
    int kb_free;
    int pages_free;

    above1M = *((uint16_t *) MEMINFO_88);
    above1M_e801 = *((uint16_t *) MEMINFO_E801A);
    above16M = *((uint16_t *) MEMINFO_E801B);

    if (above1M_e801 != 0) {
        kb_free = above1M_e801;
        kb_free += (above16M << 6);
    }
    else {
        kb_free = above1M;
    }
    pages_free = (kb_free >> (PAGE_SHIFT-KB_SHIFT));

    printk("%u KiB free\n", kb_free);
    printk("%u pages free\n", pages_free);

    smap = (struct smap_entry *) MEMINFO_SMAP;
    if (smap->limit != 0 && smap->type != SMAP_TYPE_INVALID) {
        has_smap = true;
    }

    if (has_smap) {
        while (smap->limit != 0) {
            int kb = (uint32_t) (smap->limit >> 10);
            printk("%p%p %p%p %d %-2d %6d KiB\n",
                p64(smap->addr),
                p64(smap->addr+smap->limit-1),
                smap->type, smap->extra,
                kb);
            
            /* TODO: read SMAP */
            smap++;
        }
    }

    memset(g_pgdir, 0, 4*KB);
    memset(g_pgtbl0, 0, 4*KB);

    /* Open 0-4MiB to kernel as 4K pages */
    g_pgdir[0].pde4k.base = ((uint32_t)g_pgtbl0) >> PAGE_SHIFT;
    g_pgdir[0].pde4k.rw = 1;
    g_pgdir[0].pde4k.us = 0;
    g_pgdir[0].pde4k.ps = 0;
    g_pgdir[0].pde4k.p = 1;

    /* Mark all 4K pages in 0-4M region as present,
       except for pages in the hardware-mapped area (0x9FC00-0xFFFFF),
       but keep video memory accessible. */
    for (int i = 0; i < 1024; i++) {
        uint32_t pg_addr = (i << PAGE_SHIFT);
        g_pgtbl0[i].base = i;
        g_pgtbl0[i].rw = 1;
        g_pgtbl0[i].us = 0;
        g_pgtbl0[i].g = 1;
        g_pgtbl0[i].p = 1;
        if (pg_addr >= 0x9FC00 && pg_addr < 0x100000) {
            g_pgtbl0[i].p = 0;
        }
        if (pg_addr >= 0xB0000 && pg_addr < 0xC0000) {
            g_pgtbl0[i].p = 1;
        }
    }

    /* Enable paging */
    struct cr0 cr0;
    struct cr3 cr3;
    struct cr4 cr4;
    rdcr0(cr0._value);
    rdcr3(cr3._value);
    rdcr4(cr4._value);

    cr3.pgdir_base = ((uint32_t) g_pgdir) >> PAGE_SHIFT;
    cr3.pcd = 0;
    cr3.pwt = 0;
    wrcr3(cr3._value);

    cr0.pg = 1;
    wrcr0(cr0._value);

    cr4.pse = 1;
    cr4.pae = 0;
    wrcr4(cr4._value);
}
