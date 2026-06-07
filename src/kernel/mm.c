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
#include <stdio.h>
#include <i386/bitops.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/pool.h>
#include <sys/ohwes.h>

// struct free_page {
//     struct free_page *next;
//     uint16_t pfn;
// } __pack;

struct zone {
    const char *name;

    size_t free_pages;
    size_t mem_size_pages;
    size_t buddy_size_pages;

    // allocable physical memory address range
    //  TODO: eventually a list of ranges
    uintptr_t mem_start;            // start of allocable physical memory
    uintptr_t mem_end;              // end (limit) of allocable physical memory

    // address range visible to buddy allocator
    uintptr_t alloc_start;          // start address visible to buddy allocator
    uintptr_t alloc_end;            // end address visible to buddy allocator

    int bitmap_size;                // size of bitmap at highest order, in bits
    char *bitmap[MAX_ORDER+1];      // per-order bitmap, DWORD-aligned
};

static struct zone _zones[NR_ZONES];
static struct acpi_mmap_entry _phys_mmap[64];

static __init void check_memory(void);
static __init void init_phys_mmap(void);
static __init void init_phys_mmap_legacy(void);
static __init void init_zones(void);

extern __init struct boot_info *g_boot;

__init void init_mm(void)
{
    init_phys_mmap();
    check_memory();     // make sure we have enough!
    init_zones();
}

static __init void init_phys_mmap(void)
{
    int i;
    const struct acpi_mmap_entry *e;

    if (!g_boot->mem_map) {
        pr_warn("bios-e820: memory map not available\n");
        init_phys_mmap_legacy();
        return;
    }

    e = (const struct acpi_mmap_entry *) KERNEL_ADDR(g_boot->mem_map);
    for (i = 0; i < countof(_phys_mmap) && mmap_valid(e); i++, e++) {
        _phys_mmap[i] = *e;
    }

    if (i >= countof(_phys_mmap)) {
        panic("too many entries for physical memory map table!\n");
    }
}

static __init void init_phys_mmap_legacy(void)
{
    int kb_free_low;    //   0 - 640K
    int kb_free_1M;     //  1M - 16M
    int kb_free_16M;    // 16M - 4G
    struct acpi_mmap_entry *map;

    map = _phys_mmap;

    kb_free_low = g_boot->kb_low;
    if (g_boot->kb_high_e801h != 0) {
        kb_free_1M = g_boot->kb_high_e801h;
        kb_free_16M = (g_boot->kb_extended << 6);
    }
    else {
        pr_warn("bios-e801: memory map not available\n");
        kb_free_1M = g_boot->kb_high;
        kb_free_16M = 0;
    }

    if (kb_free_low) {
        map->base = 0;
        map->length = (kb_free_low << 10);
        map->type = ACPI_MMAP_TYPE_USABLE;
        map->attr = 0;
        map++;
    }
    if (kb_free_1M) {
        map->base = (1 * MB);
        map->length = (kb_free_1M << 10);
        map->type = ACPI_MMAP_TYPE_USABLE;
        map->attr = 0;
        map++;
    }
    if (kb_free_16M) {
        map->base = (16 * MB);
        map->length = (kb_free_16M << 10);
        map->type = ACPI_MMAP_TYPE_USABLE;
        map->attr = 0;
        map++;
    }
}

static __init void check_memory(void)
{
    int free_mem_kilobytes = 0;
    struct acpi_mmap_entry *e;

    // tally up the amount of usable RAM
    for (e = _phys_mmap; mmap_valid(e); e++) {
        size_t size_kb = (e->length) >> KB_SHIFT;
        if (mmap_usable(e)) {
            free_mem_kilobytes += size_kb;
        }
    }

    if (free_mem_kilobytes < (MEMORY_REQUIRED >> KB_SHIFT)) {
        panic("not enough memory! " OS_NAME " needs least %dk to operate!",
            (MEMORY_REQUIRED >> KB_SHIFT));
    }
}

static __init void init_zones(void)
{
    // TODO: DMA, HIGHMEM...

    struct zone *zone = &_zones[ZONE_NORMAL];
    zone->name = STRINGIFY(ZONE_NORMAL);

    // determine the end of the first contiguous physical memory region above 1M
    //  TODO: expand this; include other regions, or define a max-sized region
    struct acpi_mmap_entry *e;
    for (e = _phys_mmap; mmap_valid(e); e++) {
        if (e->base >= (1 * MB)) {
            break;
        }
    }
    if (!mmap_valid(e)) {
        panic("what?? could not locate contiguous memory region above 1M");
    }

    // allocable physical address range
    zone->mem_start = PHYSICAL_ADDR(PAGE_ALIGN(__kernel_end));
    zone->mem_end = PHYSICAL_ADDR(e->base + e->length - 1);
    zone->mem_size_pages = PAGE_ALIGN(zone->mem_end+1 - zone->mem_start) >> PAGE_SHIFT;

    // address range visible to buddy allocator
    zone->alloc_start = (zone->mem_start & ~(MAX_ORDER_SIZE - 1));
    zone->alloc_end = align(zone->mem_end, MAX_ORDER_SIZE) - 1;
    zone->buddy_size_pages = PAGE_ALIGN(zone->alloc_end+1 - zone->alloc_start) >> PAGE_SHIFT;

    assert(zone->alloc_start <= zone->mem_start);
    assert(zone->alloc_end >= zone->mem_end);

    // set up a bitmap for keeping track of physical page alloc status
    char *bitmap = KERNEL_ADDR(zone->mem_start);

    // figure out bitmap layout for each order
    size_t total_num_bits = 0;
    for (int i = MAX_ORDER; i >= 0; i--) {
        size_t num_bits = div_ceil(zone->buddy_size_pages, (1 << i));
        size_t num_bits_aligned = align(num_bits, 32);
        zone->bitmap[i] = bitmap + (total_num_bits >> 3);
        total_num_bits += num_bits_aligned;
        if (i == MAX_ORDER) {
            zone->bitmap_size = num_bits;
        }
    }
    assert(isaligned(total_num_bits, 32));

    // now we know the size of the bitmap, initialize it!
    size_t bitmap_size_pages = PAGE_ALIGN(div_ceil(total_num_bits, 8)) >> PAGE_SHIFT;
    memset(bitmap, 0xFF, bitmap_size_pages << PAGE_SHIFT);

    // adjust allocable memory range to account for bitmap
    zone->mem_start += bitmap_size_pages << PAGE_SHIFT;
    zone->mem_size_pages -= bitmap_size_pages;
    zone->free_pages = zone->mem_size_pages;

    // now, clear out the bits that represent unreachable regions;
    for (int i = MAX_ORDER; i >= 0; i--) {
        const uint32_t order_size = (1 << (i+PAGE_SHIFT));
        const uint32_t bitmap_size = (zone->bitmap_size << (MAX_ORDER-i));

        for (uint32_t lo = zone->alloc_start, bit = 0;
                lo < (uint32_t) zone->mem_start;
                lo += order_size, bit++) {
            clear_bit(zone->bitmap[i], bit);
        }

        for (uint32_t hi = zone->alloc_end+1, bit = bitmap_size - 1;
                hi > (uint32_t) (zone->mem_end+1);
                hi -= order_size, bit--) {
            clear_bit(zone->bitmap[i], bit);
        }
    }

    pr_info("mem-init: init zone %s mem_start=%p mem_end=%p mem_size_pages=%zd\n",
        zone->name, _P(zone->mem_start), _P(zone->mem_end), zone->mem_size_pages);
    pr_info("mem-init: init zone %s bitmap=%p size_pages=%ld\n",
        zone->name, bitmap, bitmap_size_pages);

    // ensure pages are mapped to speed up allocation time
    uintptr_t top = min((4*MB), zone->mem_end+1); // TODO: temp workaround for update_page_mappings 4M limit...
    size_t size_pages = (top - zone->mem_start) >> PAGE_SHIFT;
    pr_warn("mem-init: only mappings up to 4M supported until multiple PDEs implemented!\n");
    pgflags_t flags = _PAGE_RW | _PAGE_PRESENT;
    update_page_mappings(KERNEL_ADDR(zone->mem_start), zone->mem_start, size_pages, flags);

    // TODO: could calculate how many page tables are needed to alloc all of
    // "Normal" memory, then stuff them before the bitmap too...
}

void * alloc_pages(int flags, int order)
{
    if (order < 0 || order > MAX_ORDER) {
        pr_warn("mem: alloc_pages failed - invalid order '%d'\n", order);
        return NULL;
    }

    // TODO: validate flags

    // locate next free region at current order
    struct zone *zone = &_zones[ZONE_NORMAL];
    size_t bitmap_size = (zone->bitmap_size << (MAX_ORDER-order));
    size_t bitmap_size_bytes = div_ceil(bitmap_size, 32) << 2;  // DWORD-aligned size
    int index = bit_scan_forward(zone->bitmap[order], bitmap_size_bytes);
    if (index < 0 || index >= bitmap_size) {
        pr_fatal("mem: alloc_pages order %d failed - out of memory!\n", order);
        return NULL;
    }

    // mark the current- and lower-order chunks used
    for (int o = order, i = index, c = 1; o >= 0; o--, i <<= 1, c <<= 1) {
        for (int n = 0; n < c; n++) {
            clear_bit(zone->bitmap[o], i + n);  // TODO: need a fast way to clear a range of bits...
        }
    }

    // mark the higher-order chunks used
    for (int o = order + 1, i = index >> 1; o <= MAX_ORDER; o++, i >>= 1) {
        clear_bit(zone->bitmap[o], i);
    }

    // calculate alloc address
    const uint32_t order_size = (1 << (order+PAGE_SHIFT));
    uint32_t addr = zone->alloc_start + (index * order_size);

    // range check!
    if (addr < zone->mem_start || addr + order_size > zone->mem_end + 1) {
        panic("%s: alloc out of bounds!!", zone->name);
    }

    void *kern_addr = KERNEL_ADDR(addr);

    zone->free_pages -= (order_size >> PAGE_SHIFT);
    pr_info("mem: alloc_pages order %d %p-%p %s; %zd pages left\n",
        order, kern_addr, kern_addr+order_size-1, zone->name, zone->free_pages);

    if (flags & MEM_ZERO) {
        zeromem(kern_addr, order_size);
    }

    return kern_addr;
}

void free_pages(void *addr, int order)
{
    if (order < 0 || order > MAX_ORDER) {
        return;
    }

    struct zone *zone = &_zones[ZONE_NORMAL];
    const uint32_t order_size = (1 << (order+PAGE_SHIFT));
    uintptr_t phys_addr = PHYSICAL_ADDR(addr);

    if (phys_addr < zone->mem_start || phys_addr + order_size > zone->mem_end + 1) {
        return;
    }
    if (!isaligned(phys_addr, order_size)) {
        return;
    }

    int index = (phys_addr - zone->alloc_start) >> (order+PAGE_SHIFT);

    // free the current- and lower-order chunks

    for (int o = order, i = index, c = 1; o >= 0; o--, i <<= 1, c <<= 1) {
        for (int n = 0; n < c; n++) {
            set_bit(zone->bitmap[o], i + n);
        }
    }

    for (int o = order, i = index; o < MAX_ORDER; o++, i >>= 1) {
        int neighbor = (i % 2) ? i-1 : i+1;
        if (0 == test_bit(zone->bitmap[o], neighbor)) {
            break;
        }
        set_bit(zone->bitmap[o + 1], i >> 1);
    }

    zone->free_pages += (order_size >> PAGE_SHIFT);
    pr_info("mem: free_pages order %d %p-%p %s; %zd pages left\n",
        order, addr, addr+order_size-1, zone->name, zone->free_pages);
}

int get_order(size_t size)
{
    // TODO: do this without loop?
    size_t pages = PAGE_ALIGN(size) >> PAGE_SHIFT;
    for (int o = 0; o <= MAX_ORDER; o++) {
        if ((1 << o) >= pages) {
            return o;
        }
    }

    return -1;
}

size_t get_order_size(int order)
{
    if (order < 0 || order > MAX_ORDER) {
        return 0;
    }

    return (1 << (order+PAGE_SHIFT));
}
