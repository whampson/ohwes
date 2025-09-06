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
#include <i386/bitops.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <kernel/config.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

// struct free_page {
//     struct free_page *next;
//     uint16_t pfn;
// } __pack;

struct zone {
    uint32_t base_pfn;

    size_t size_pages;
    size_t free_pages;
    // TODO: maintain a free list of pages?
    //       could have an O(1) page frame allocator!!

    // struct range phys_mem;          // allocable physical memory address range
    // struct range buddy_mem;         // address range visible to buddy allocator

    size_t bitmap_size_pages;       // TODO: can probably calculate this
    char *bitmap[MAX_ORDER+1];      // per-order bitmap, DWORD-aligned
    int bitmap_size[MAX_ORDER+1];   // num bits per order
};

static struct zone _zones[NR_ZONES];
static struct acpi_mmap_entry phys_mmap[64];

static char *mem_start;
static char *mem_end;

extern void init_pool(void);
static void print_kernel_sections(void);

static void init_mmap(struct acpi_mmap_entry *map, struct boot_info *boot);
static void init_mmap_legacy(struct acpi_mmap_entry *map, struct boot_info *boot);

void init_mm(struct boot_info *boot)
{
    // initialize physical memory map
    zeromem(phys_mmap, sizeof(phys_mmap));
    init_mmap(phys_mmap, boot);

    // ------------------------------------------------------------------------

    int total_kb = 0;
    int free_kb = 0;
    int bad_kb = 0;
    int free_pages = 0;
    struct acpi_mmap_entry *e;

    // tally up the amount of usable RAM
    for (e = phys_mmap; mmap_valid(e); e++) {
        size_t size_kb = (e->length) >> KB_SHIFT;
        char size_char = 'k';
        size_t disp_size = size_kb;
        if (disp_size >= 1024) {
            size_char = 'M';
            disp_size = div_ceil(disp_size, 1024);
        }
        kprint("phys-mem: %08llX-%08llX % 4lu%c %s",
            e->base, e->base+e->length-1,
            disp_size, size_char,
            mmap_bad(e)     ? "*** BAD ***" :
            mmap_acpi(e)    ? "ACPI" :
            mmap_usable(e)  ? "" : "reserved");
        if (e->attr) {
            kprint(" (attr = 0x%X)", e->attr);
        }
        kprint("\n");

        total_kb += size_kb;
        if (mmap_bad(e)) {
            bad_kb += size_kb;
        }
        if (mmap_usable(e)) {
            free_kb += size_kb;
        }
    }

    free_pages = (free_kb >> (PAGE_SHIFT - KB_SHIFT));
    kprint("phys-mem: %dk total, %dk usable\n", total_kb, free_kb);
    kprint("phys-mem: %d usable pages\n", free_pages);
    if (bad_kb > 0) {
        warn("phys-mem: found %dk of bad memory!\n", bad_kb);
    }

    if (free_kb < (MEMORY_REQUIRED >> KB_SHIFT)) {
        panic("not enough memory! " OS_NAME " needs least %dk to operate!",
            (MEMORY_REQUIRED >> KB_SHIFT));
    }

    print_kernel_sections();

    init_pool();

    // ------------------------------------------------------------------------

    char *bitmap;
    size_t mem_size_pages;
    size_t bitmap_size_pages;
    struct zone *zone;

    zone = &_zones[ZONE_NORMAL];

    // find size of first contiguous free region above 1M
    // TODO: expand this... or define a max-sized region
    mem_end = NULL;
    for (e = phys_mmap; mmap_valid(e); e++) {
        if (e->base >= (1 * MB)) {
            mem_end = (char *) PHYSICAL_ADDR(e->base + e->length - 1);
            break;
        }
    }

    // physical memory bounds
    mem_start = (char *) PHYSICAL_ADDR(PAGE_ALIGN(__kernel_end));
    (void) mem_end;
    mem_size_pages = PAGE_ALIGN(mem_end - mem_start + 1) >> PAGE_SHIFT;

    // set up a bitmap for keeping track of physical page alloc status
    bitmap = (char *) KERNEL_ADDR(mem_start);

    // determine the range of addresses covered by highest order allocations
    const uint32_t max_order_lo = ((uint32_t) mem_start) & ~(MAX_ORDER_SIZE - 1);
    const uint32_t max_order_hi = align(mem_end, MAX_ORDER_SIZE);
    const uint32_t max_order_range_pages = (max_order_hi - max_order_lo) >> PAGE_SHIFT;

    // figure out bitmap layout for each order
    size_t total_num_bits = 0;
    for (int i = MAX_ORDER; i >= 0; i--) {
        size_t num_bits = div_ceil(max_order_range_pages, (1 << i));
        size_t num_bits_aligned = align(num_bits, 32);
        zone->bitmap_size[i] = num_bits;
        zone->bitmap[i] = bitmap + (total_num_bits >> 3);
        total_num_bits += num_bits_aligned;
    }
    assert(aligned(total_num_bits, 32));

    // now we know the size of the bitmap, initialize it!
    bitmap_size_pages = PAGE_ALIGN(div_ceil(total_num_bits, 8)) >> PAGE_SHIFT;
    memset(bitmap, 0xFF, bitmap_size_pages << PAGE_SHIFT);

    // adjust allocable memory range to account for bitmap
    mem_start += bitmap_size_pages << PAGE_SHIFT;
    mem_size_pages -= bitmap_size_pages;

    // now, clear out the bits that represent unreachable regions;
    for (int i = MAX_ORDER; i >= 0; i--) {
        const uint32_t order_size = (1 << (i+PAGE_SHIFT));

        for (uint32_t lo = max_order_lo, bit = 0;
                lo < (uint32_t) mem_start;
                lo += order_size, bit++) {
            clear_bit(zone->bitmap[i], bit);
        }

        for (uint32_t hi = max_order_hi, bit = zone->bitmap_size[i] - 1;
                hi > (uint32_t) (mem_end+1);
                hi -= order_size, bit--) {
            clear_bit(zone->bitmap[i], bit);
        }
    }

    kprint("mem: initialized bitmap at %08X size_pages=%d\n", bitmap, bitmap_size_pages);

    zone->base_pfn = __pfn(mem_start);
    zone->size_pages = mem_size_pages;
    zone->free_pages = mem_size_pages;
    zone->bitmap_size_pages = bitmap_size_pages;

    kprint("mem: mem_start=%08X, mem_end=%08X, mem_size_pages=%d\n", mem_start, mem_end, mem_size_pages);

    // ensure pages are mapped to speed up allocation time
    char *top = min((char *) (4*MB), mem_end+1); // TODO: temp workaround for update_page_mappings 4M limit...
    size_t size_pages = (top - mem_start) >> PAGE_SHIFT;
    pgflags_t flags = _PAGE_RW | _PAGE_PRESENT;
    update_page_mappings(KERNEL_ADDR(mem_start), (uint32_t) mem_start, size_pages, flags);

    // TODO: could calculate how many page tables are needed to alloc all of
    // "Normal" memory, then stuff them before the bitmap too...
}

static void init_mmap_legacy(struct acpi_mmap_entry *map, struct boot_info *boot)
{
    int kb_free_low;    //   0 - 640K
    int kb_free_1M;     //  1M - 16M
    int kb_free_16M;    // 16M - 4G

    kb_free_low = boot->kb_low;
    if (boot->kb_high_e801h != 0) {
        kb_free_1M = boot->kb_high_e801h;
        kb_free_16M = (boot->kb_extended << 6);
    }
    else {
        kprint("bios-e801: memory map not available\n");
        kb_free_1M = boot->kb_high;
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

static void init_mmap(struct acpi_mmap_entry *map, struct boot_info *boot)
{
    if (!boot->mem_map) {
        kprint("bios-e820: memory map not available\n");
        init_mmap_legacy(map, boot);
        return;
    }

    const struct acpi_mmap_entry *e;
    e = (const struct acpi_mmap_entry *) KERNEL_ADDR(boot->mem_map);

    int i;
    for (i = 0; i < countof(phys_mmap) && mmap_valid(e); i++, e++) {
        map[i] = *e;
    }

    if (i >= countof(phys_mmap)) {
        panic("too many entries for physical memory map table!\n");
    }
}

void * alloc_pages_buddy(int flags, int order)
{
    (void) flags;

    if (order < 0 || order > MAX_ORDER) {
        return NULL;
    }

    // locate next free region at current order
    struct zone *zone = &_zones[ZONE_NORMAL];
    size_t bitmap_size = zone->bitmap_size[order];
    size_t bitmap_size_bytes = div_ceil(bitmap_size, 32) << 2;  // DWORD-aligned size
    int index = bit_scan_forward(zone->bitmap[order], bitmap_size_bytes);
    if (index < 0 || index >= bitmap_size) {
        return NULL;
    }

    // mark the lower-order chunks in-use
    for (int o = order, i = index, c = 1; o >= 0; o--, i <<= 1, c <<= 1) {
        for (int n = 0; n < c; n++) {
            clear_bit(zone->bitmap[o], i + n);  // TODO: need a fast way to clear a range of bits...
        }
    }

    // mark the higher-order chunks in-use
    for (int o = order + 1, i = index >> 1; o <= MAX_ORDER; o++, i >>= 1) {
        clear_bit(zone->bitmap[o], i);
    }

    // calculate alloc address
    const uint32_t max_order_lo = ((uint32_t) mem_start) & ~(MAX_ORDER_SIZE - 1);
    const uint32_t order_size = (1 << (order+PAGE_SHIFT));
    uint32_t addr = max_order_lo + (index * order_size);

    // range check!
    if (addr < (uint32_t) mem_start || addr + order_size > (uint32_t) mem_end + 1) {
        panic("phys-mem: alloc: order=%d addr=%08X size=%d index=%d; out of bounds!!",
            order, addr, order_size, index);
    }

    zone->free_pages -= (order_size >> PAGE_SHIFT);
    kprint("phys-mem: alloc: order=%d addr=%08X size=%d index=%d; %d pages left\n",
        order, addr, order_size, index, zone->free_pages);

    return (void *) addr;
}

void free_pages_buddy(void *addr, int order)
{

}

void * alloc_pages(int flags, size_t count)
{
    (void) flags;

    struct zone *zone = &_zones[ZONE_NORMAL];   // TODO: zone selecton flag
    size_t bitmap_size_bytes = (zone->bitmap_size_pages << PAGE_SHIFT);
    int start_index = 0;
    bool found = false;

    if (count == 0 || count > zone->free_pages) {
        return NULL;    // not enough free pages left!
    }

    // locate start of contiguous region
    do {
        bool found_start = false;

        int bit_offset = (start_index % 8);
        char *bitmap_ptr = (char *) (zone->bitmap +
            (align(start_index, 8) >> 3) +
            ((bit_offset > 0) ? -1 : 0));

        // check range: non-DWORD-aligned case
        if (!aligned(start_index, 32)) {
            while (!found_start) {
                for (; bit_offset < 8; bit_offset++) {
                    if (test_bit(bitmap_ptr, bit_offset)) {
                        found_start = true;
                        break;
                    }
                    start_index++;
                }
                bitmap_ptr++;
                bit_offset = 0;
                if (aligned(start_index, 32)) {
                    break;
                }
            }
        }

        // check range: DWORD aligned case
        if (!found_start) {
            assert(aligned(start_index, 32));
            assert(aligned(bitmap_ptr, 4));
            int bitmap_offset = (bitmap_ptr - zone->bitmap[0]);
            start_index = bit_scan_forward(bitmap_ptr, bitmap_size_bytes - bitmap_offset);
            start_index += (bitmap_offset << 3);
        }

        if (start_index + count > zone->size_pages) {
            return NULL;    // not enough contiguous memory to fit allocation!
        }

        // check if contiguous region will fit requested memory
        found = true;
        for (int i = 0; i < count; i++) {
            // TODO: optimize by checking whole DWORDs or larger
            if (test_bit(zone->bitmap, start_index + i) == 0) {
                found = false;
                start_index += (i + 1); // not enough pages in range
                break;                  // advance start_index and continue search
            }
        }
    } while (!found && start_index + count < zone->size_pages);

    if (!found) {
        return NULL;
    }

    // zero the bitmap region
    for (int i = 0; i < count; i++) {
        // TODO: optimize by setting whole DWORDs or larger
        clear_bit(zone->bitmap, start_index + i);
    }
    zone->free_pages -= count;
    assert(zone->free_pages <= zone->size_pages);

    return (void *) KERNEL_ADDR((zone->base_pfn + start_index) << PAGE_SHIFT);
}

void free_pages(void *addr, size_t count)
{
    struct zone *zone = &_zones[ZONE_NORMAL];
    uint32_t index = __pfn(PHYSICAL_ADDR(addr)) - zone->base_pfn;

    if (index >= zone->size_pages ||
        zone->free_pages + count > zone->size_pages) {
        return;
    }

    for (int i = 0; i < count; i++) {
        // TODO: optimize by setting whole DWORDs or larger
        set_bit(zone->bitmap, index + i);
    }
    zone->free_pages += count;
    assert(zone->free_pages <= zone->size_pages);
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
        { "kernel image:",  __kernel_start,     __kernel_end },
        { ".setup",         __setup_start,      __setup_end },
        { ".text",          __text_start,       __text_end },
        { ".rodata",        __rodata_start,     __rodata_end },
        { ".data",          __data_start,       __data_end },
        { ".bss",           __bss_start,        __bss_end },
        { ".idt",           __idt_start,        __idt_end },
        { ".pgdir",         __pgdir_start,      __pgdir_end },
        { ".pgtbl",         __pgtbl_start,      __pgtbl_end },
        { ".pgmap",         __pgmap_start,      __pgmap_end },
        { ".klog",          __klog_start,       __klog_end },
        { ".kstack",        __kstack_start,     __kstack_end },
        { ".ustack",        __ustack_start,     __ustack_end },
        { ".estack",        __estack_start,     __estack_end },
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
        align(__kernel_size, KB) >> KB_SHIFT,
        PAGE_ALIGN(__kernel_size) >> PAGE_SHIFT);
}
