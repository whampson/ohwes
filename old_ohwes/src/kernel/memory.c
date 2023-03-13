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
#include <acpi.h>
#include <x86/paging.h>
#include <x86/cntrl.h>
#include <drivers/vga.h>

#define p64(x) (uint32_t)((x) >> 32), (uint32_t)((x) & 0xFFFFFFFF)

struct pgdir_entry *g_pgdir   = (struct pgdir_entry *) PGDIR;
struct pgtbl_entry *g_pgtbl0  = (struct pgtbl_entry *) PGTBL0;

static const char *smap_types[] =
{
    "invalid", "free", "reserved", "acpi", "non-volatile", "bad", "disabled"
};

void mem_init(void)
{
    uint16_t above1M;           /* 1K blocks */
    uint16_t above1M_e801;      /* 1K blocks */
    uint16_t above16M;          /* 64K blocks */
    struct smap_entry *smap;    /* a nice table */
    bool has_smap;
    int kb_free;
    char *map_source;

    above1M = *((uint16_t *) MEMINFO_88);
    above1M_e801 = *((uint16_t *) MEMINFO_E801A);
    above16M = *((uint16_t *) MEMINFO_E801B);

    smap = (struct smap_entry *) MEMINFO_SMAP;
    if (smap->size != 0 && smap->type != SMAP_TYPE_INVALID) {
        has_smap = true;
        map_source = "bios-e820";
    }

    if (!has_smap) {
        smap[0].type = SMAP_TYPE_FREE;
        smap[0].addr = 0x00000000;
        smap[0].size = 0x9F000;     /* many bioses report reserved regions
                                       between 0x9F000 and 0xA0000 */
        if (above1M_e801 != 0) {
            smap[1].type = (above1M_e801) ? SMAP_TYPE_FREE : SMAP_TYPE_RESERVED;
            smap[1].addr = (1 << MB_SHIFT);
            smap[1].size = (above1M_e801 << KB_SHIFT);
            if (above1M_e801) {
                smap[2].type = (above16M) ? SMAP_TYPE_FREE : SMAP_TYPE_RESERVED;
                smap[2].addr = (16 << MB_SHIFT);
                smap[2].size = (above16M << (KB_SHIFT+6));
            }
            map_source = "bios-e801";
        }
        else {
            smap[1].type = (above1M) ? SMAP_TYPE_FREE : SMAP_TYPE_RESERVED;
            smap[1].addr = (1 << MB_SHIFT);
            smap[1].size = (above1M << KB_SHIFT);
            map_source = "bios-88";
        }
    }

    kb_free = 0;
    kprintf("Physical Memory Map:\n");
    while (smap->size != 0) {
        int kb = (uint32_t) (smap->size >> KB_SHIFT);
        uint64_t limit = smap->addr + smap->size - 1;
        if (smap->type == SMAP_TYPE_FREE) {
            kb_free += kb;
        }
        kprintf("  %s: %p-%p (%s",
            map_source,
            smap->addr_lo, (uint32_t) limit,
            smap_types[smap->type]);
        if (smap->extra) kprintf(",%d", smap->extra);
        kprintf(")\n");
        smap++;
    }

    printf("%d KiB free\n", kb_free);
    if (kb_free < MIN_KB) {
        panic("not enough memory! OHWES needs at least %d KiB to run.", MIN_KB);
    }

    /* TODO: build memory map, check for overlapping regions,
       enable/disable pages based on mapping */
}
