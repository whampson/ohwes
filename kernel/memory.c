/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
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
#include <ohwes/memory.h>
#include <ohwes/kernel.h>
#include <ohwes/init.h>
#include <ohwes/acpi.h>
#include <x86/paging.h>
#include <x86/cntrl.h>
#include <drivers/vga.h>

#define p64(x) (uint32_t)((x) >> 32), (uint32_t)((x) & 0xFFFFFFFF)

#define MIN_KB  4096

struct pgdir_entry *g_pgdir   = (struct pgdir_entry *) PGDIR;
struct pgtbl_entry *g_pgtbl0  = (struct pgtbl_entry *) PGTBL0;
size_t g_tom;

void mem_init(void)
{
    /* TODO: check for/handle 15-16M hole */
    
    uint16_t above1M;           /* 1K blocks */
    uint16_t above1M_e801;      /* 1K blocks */
    uint16_t above16M;          /* 64K blocks */
    struct smap_entry *smap;    /* a nice table */
    bool has_smap;
    int kb_free;

    above1M = *((uint16_t *) MEMINFO_88);
    above1M_e801 = *((uint16_t *) MEMINFO_E801A);
    above16M = *((uint16_t *) MEMINFO_E801B);

    kb_free = 640;
    g_tom = 1*MB;
    if (above1M_e801 != 0) {
        kb_free += above1M_e801;
        kb_free += (above16M << 6);
        g_tom += (above1M_e801 << KB_SHIFT);
        g_tom += (above16M << (KB_SHIFT+6));
    }
    else {
        kb_free += above1M;
        g_tom += (above1M << KB_SHIFT);
    }

    smap = (struct smap_entry *) MEMINFO_SMAP;
    if (smap->limit != 0 && smap->type != SMAP_TYPE_INVALID) {
        has_smap = true;
    }

    if (has_smap) {
        kb_free = 0;
        g_tom = 0;
        while (smap->limit != 0) {
            int kb = (uint32_t) (smap->limit >> 10);
            uint64_t end = smap->addr+smap->limit;
            if (smap->type == SMAP_TYPE_FREE) {
                kb_free += kb;
                if (end > g_tom) {
                    g_tom = end;
                }
            }
            printk("%p%p %p%p %d %-2d\n",
                p64(smap->addr), p64(end-1),
                smap->type, smap->extra);
            smap++;
        }
    }

    printf("%d KiB free\n", kb_free);
    if (kb_free < MIN_KB) {
        panic("Not enough memory!");
    }
}
