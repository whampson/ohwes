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
 *         File: kernel/mm.c
 *      Created: July 3, 2024
 *       Author: Wes Hampson
 *
 * Physical Page Allocator, Buddy System:
 * https://www.kernel.org/doc/gorman/html/understand/understand009.html
 * =============================================================================
 */

#include <assert.h>
#include <errno.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <kernel/config.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

#define MAX_ORDER   11

struct free_page {
    struct free_page *next;
    uint16_t pfn;
} __pack;

// struct zone {
//     uintptr_t zone_base;
//     struct free_area free_area[MAX_ORDER];
//     pool_t free_list_pool;
// };

enum zone_type {
    ZONE_INVALID,
    ZONE_STATIC,
    ZONE_DYNAMIC,
    ZONE_DMA,
    ZONE_ACPI,
};

struct phys_mmap_entry {
    struct acpi_mmap_entry acpi_entry;

    uint64_t base;
    uint64_t limit;

    enum zone_type zone;

    bool reserved : 1;
    bool bad      : 1;
};

struct zone
{
    enum zone_type type;

    uint32_t base;
    uint32_t free_pages;

    struct page *page_map;
};

static struct phys_mmap_entry phys_mmap[64];

extern void init_pool(void);
static void print_kernel_sections(void);

void init_mm(struct boot_info *boot)
{
    struct phys_mmap_entry *p;
    int kb_free_low;    //   0 - 640K
    int kb_free_1M;     //  1M - 16M
    int kb_free_16M;    // 16M - 4G

    int total_kb;
    int total_kb_free;
    int free_pages;

    const int KernelEndPhys = PAGE_ALIGN(PHYSICAL_ADDR((uint32_t) &__kernel_end));

    kb_free_low = boot->kb_low;
    if (boot->kb_high_e801h != 0) {
        kb_free_1M = boot->kb_high_e801h;
        kb_free_16M = (boot->kb_extended << 6);
    }
    else {
        kprint("bios-e081: memory map not available\n");
        kb_free_1M = boot->kb_high;
        kb_free_16M = 0;
    }

#define __mkentry(ntry,bas,siz,zon) \
do { \
    (ntry)->base = (bas);   \
    (ntry)->limit = (bas)+(siz)-1;   \
    (ntry)->zone = (zon);   \
} while (0)

    if (!boot->mem_map) {
        kprint("bios-e820: memory map not available\n");
        p = phys_mmap;
        if (kb_free_low) {
            __mkentry(p, 0, (kb_free_low << 10), ZONE_DMA);
            p++;
        }
        if (kb_free_1M) {
            __mkentry(p, (1 << 20), (kb_free_1M << 10),
                (KernelEndPhys < (1 << 20)) ? ZONE_DYNAMIC : ZONE_STATIC);
            p++;
        }
        if (kb_free_16M) {
            __mkentry(p, (1 << 24), (kb_free_16M << 10),
                (KernelEndPhys < (1 << 24)) ? ZONE_DYNAMIC : ZONE_STATIC);
            p++;
        }
    }
    else {
        int i;
        const struct acpi_mmap_entry *e;

        p = phys_mmap;
        e  = (const struct acpi_mmap_entry *) KERNEL_ADDR(boot->mem_map);

        for (i = 0;
             i < countof(phys_mmap) && e->type != ACPI_MMAP_TYPE_INVALID;
             i++, e++, p++
        ) {
            kprint("bios-e820: %08llX-%08llX ", e->base, e->base + e->length-1);
            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE: kprint("free"); break;
                case ACPI_MMAP_TYPE_RESERVED: kprint("reserved"); break;
                case ACPI_MMAP_TYPE_ACPI: kprint("reserved (ACPI)"); break;
                case ACPI_MMAP_TYPE_NVS: kprint("reserved (ACPI, non-volatile)"); break;
                case ACPI_MMAP_TYPE_BAD: kprint("bad"); break;
                default: kprint("unknown (%d)", e->type); break;
            }
            if (e->attributes) {
                kprint(" (attributes = 0x%X)", e->attributes);
            }
            kprint("\n");

            __mkentry(p, e->base, e->length, ZONE_STATIC);
            p->acpi_entry = *e;

            p->bad = (e->type == ACPI_MMAP_TYPE_BAD);
            p->reserved = (e->type != ACPI_MMAP_TYPE_USABLE);

            if (e->type == ACPI_MMAP_TYPE_ACPI || e->type == ACPI_MMAP_TYPE_NVS) {
                p->zone = ZONE_ACPI;
                p->reserved = (e->type == ACPI_MMAP_TYPE_NVS);
            }
            else if (p->base < boot->ebda_base) {
                p->zone = ZONE_DMA;
            }
            else if (p->base >= KernelEndPhys && !p->reserved) {
                // TODO: need to account for paging structures
                // ... or just put them before the kernel
                p->zone = ZONE_DYNAMIC;
            }

            // TODO: always create 15M-16M hole??
            //   sometimes it doesn't show up in ACPI table despite
            //   e801 reporting a gap
        }

        if (i >= countof(phys_mmap)) {
            warn("physical memory map limit reached!\n");
        }
    }

#undef __mkentry

    total_kb = 0;
    total_kb_free = 0;
    free_pages = 0;

    p = phys_mmap;
    while (p->zone != ZONE_INVALID) {
        char size_char = 'k';
        size_t size_kb = (p->limit - p->base + 1) >> 10;
        size_t disp_size = size_kb;
        if (disp_size >= 1024) {
            size_char = 'M';
            disp_size >>= 10;
        }
        kprint("phys-mem: %08llX-%08llX % 4lu%c %c%c %-12s\n",
            p->base, p->limit, disp_size, size_char,
            (p->reserved) ? 'r' : ' ',
            (p->bad)      ? 'b' : ' ',
            (p->zone == ZONE_STATIC)  ? STRINGIFY(ZONE_STATIC)  :
            (p->zone == ZONE_DYNAMIC) ? STRINGIFY(ZONE_DYNAMIC) :
            (p->zone == ZONE_DMA)     ? STRINGIFY(ZONE_DMA)     :
            (p->zone == ZONE_ACPI)    ? STRINGIFY(ZONE_ACPI)    :
                                        STRINGIFY(ZONE_INVALID));
        if (!p->reserved && !p->bad) {
            total_kb_free += size_kb;
        }
        total_kb += size_kb;
        p++;
    }

    free_pages = (total_kb_free >> 2);
    kprint("phys-mem: %dk total, %dk usable\n", total_kb, total_kb_free);
    kprint("phys-mem: %d usable pages\n", free_pages);

    if (total_kb_free < (MEMORY_REQUIRED >> 10)) {
        panic("not enough memory! " OS_NAME " needs least %dk to operate!",
            (MEMORY_REQUIRED >> 10));
    }

    print_kernel_sections();

    init_pool();
    // pool_create(
    //     PAGE_ALIGN(__kernel_end), "free_list",
    //     sizeof(struct free_page), 0 /* num_page_frames */);


    char *mem_start;
    char *mem_end = NULL;
    char *bitmap;
    size_t mem_size_pages;
    size_t bitmap_size_pages;

    // find size of first contiguous free region above 1M
    for (p = phys_mmap; p->zone != ZONE_INVALID; p++) {
        if (p->base >= (1 * MB)) {
            mem_end = (char *) PHYSICAL_ADDR((uint32_t) p->limit);
            break;
        }
    }

    // physical memory bounds
    mem_start = (char *) PHYSICAL_ADDR(PAGE_ALIGN((uint32_t) __kernel_end));
    (void) mem_end;
    mem_size_pages = PAGE_ALIGN(mem_end - mem_start + 1) >> PAGE_SHIFT;

    // bitmap for keeping track of physical page alloc status
    bitmap_size_pages = PAGE_ALIGN(div_ceil(mem_size_pages, 8)) >> PAGE_SHIFT;
    bitmap = (char *) KERNEL_ADDR(mem_start);
    mem_start += bitmap_size_pages << PAGE_SHIFT;
    mem_size_pages -= bitmap_size_pages;

    kprint("mem: zeroing bitmap at %08X size_pages=%d...\n", bitmap, bitmap_size_pages);
    zeromem(bitmap, bitmap_size_pages << PAGE_SHIFT);
    kprint("mem: mem_start=%08X, mem_end=%08X, mem_size_pages=%d\n", mem_start, mem_end, mem_size_pages);

    // ensure pages begin unmapped
    char *top = min((char *) (4*MB), mem_end+1); // TODO: temp workaround for update_page_mappings 4M limit...
    size_t size_pages = (top - mem_start) >> PAGE_SHIFT;
    update_page_mappings(KERNEL_ADDR(mem_start), (uint32_t) mem_start, size_pages, 0);

    // TODO: slab allocator
}

static void print_kernel_sections(void)
{
    struct section {
        const char *name;
        void *start, *end;
    };

    // TODO: pack kernel.elf header into image and extract info from there

    // TODO: make this into a sorted list; collect regions at boot
    struct section sections[] = {
        { "kernel image:",  __kernel_start,                         __kernel_end },
        { ".setup",         __setup_start,                          __setup_end },
        { ".text",          __text_start,                           __text_end },
        { ".rodata",        __rodata_start,                         __rodata_end },
        { ".data",          __data_start,                           __data_end },
        { ".bss",           __bss_start,                            __bss_end },
        { ".idt",           __idt_start,                            __idt_end },
        { ".pgdir",         __pgdir_start,                          __pgdir_end },
        { ".pgtbl",         __pgtbl_start,                          __pgtbl_end },
        { ".pgmap",         __pgmap_start,                          __pgmap_end },
        { ".klog",          __klog_start,                           __klog_end },
        { ".kstack",        __kstack_start,                         __kstack_end },
        { ".ustack",        __ustack_start,                         __ustack_end },
        { ".estack",        __estack_start,                         __estack_end },

    };

    for (int i = 0; i < countof(sections); i++) {
        struct section *sec = &sections[i];
        size_t sec_size = (sec->end - sec->start);
        kprint("kern-mem: PA:%08X-%08X VA:%08X-%08X % 6lu %s\n",
            PHYSICAL_ADDR(sec->start), PHYSICAL_ADDR(sec->end)-1,
            KERNEL_ADDR(sec->start), KERNEL_ADDR(sec->end)-1,
            sec_size, sec->name);
    }

    kprint("kern-mem: kernel uses %dk (%d pages) for static memory\n",
        align((size_t) __kernel_size, KB) >> KB_SHIFT,
        PAGE_ALIGN((size_t) __kernel_size) >> PAGE_SHIFT);
}
