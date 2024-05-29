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
 *         File: kernel/memory.c
 *      Created: January 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <boot.h>
#include <ohwes.h>
#include <errno.h>
#include <paging.h>

/*
 Physical Address Map:

FFFFFFFF +----------------------------+ 4G
         |                            |
         |                            |
         |                            |
         |                            |
         | (free, if available)       |
         |                            |
         |                            |
         |                            |
         |                            |
 1000000 +----------------------------+ 16M
         |////////////////////////////|
         | (reserved for hardware) ///|
         |////////////////////////////|
  F00000 +----------------------------+ 15M
         |                            |
         |                            |
         | (free, if available)       |
         |                            |
         |                            |
  100000 +----------------------------+ 1M
         |////////////////////////////|
         | (reserved for hardware) ///|
         |////////////////////////////|
   C0000 +----------------------------+ 768K
         |                            |
         | VGA Frame Buffer           |
         |                            |
   B8000 +----------------------------+ 736K
         |////////////////////////////|
         | (reserved for VGA) ////////|
         |////////////////////////////|
   A0000 +----------------------------+ 640K
         |////////////////////////////|
         | (reservred for EBDA) //////|
         |////////////////////////////|
   9FC00 +----------------------------+ 639K (this address can vary by hardware)
         |                            | -----------+
         |                            |            |
         |                            |            |
         | Kernel Stack               |            +-- consider dynamically allocating process kernel stacks...
         |                            |            |
         |                            |            |
         |                            |            |
         +----------------------------+ (varies) --+
         |                            |
         |                            |
         |                            |
         | (free)                     |
         |                            |
         |                            |
         |                            |
         +----------------------------+ (varies)
         |                            |
         |                            |
         |                            |
         | Kernel Code                |
         |                            |
         |                            |
         |                            |
   20000 +----------------------------+ 128K
         |                            |
         | DMA Buffer                 |
         |                            |
   10000 +----------------------------+ 64K
         |                            |
         | (free)                     |
         |                            |
    C000 +----------------------------+ 52K  ---+
         | Console 3 Input Buffer     |         |
    B000 +----------------------------+ 48K     |
         | Console 3 Frame Buffer     |         |
    A000 +----------------------------+ 40K     |
         | Console 2 Input Buffer     |         |
    9000 +----------------------------+ 36K     |
         | Console 2 Frame Buffer     |         |
    8000 +----------------------------+ 32K     + -- consider dynamically allocating these...
         | Console 1 Input Buffer     |         |
    7000 +----------------------------+ 28K     |
         | Console 1 Frame Buffer     |         |
    6000 +----------------------------+ 24K     |
         | Console 0 Input Buffer     |         |
    5000 +----------------------------+ 20K     |
         | Console 0 Frame Buffer     |         |
    4000 +----------------------------+ 16K ----+
         | Kernel Page Table (0-4M)   |
    3000 +----------------------------+ 12K
         | System Page Directory      |
    2000 +----------------------------+ 8K
         | IDT/GDT/LDT/TSS            |
    1000 +----------------------------+ 4K
         | Zero Page                  |
       0 +============================+ 0K

NOTES:
    * Console frame buffer assumed to be 80x25 (2 bytes per char), therefore it
      fits within one page (4000 bytes, 96 extra bytes)
    * VGA frame buffer hardware is mapped at 0xA0000-0xBFFFF. Which chunk of
      this range is used depends on the VGA hardware configuration (see vga.c).
      For now, we are using the region from 0xB8000-0xBFFFF (CGA).
*/

static void print_meminfo(const struct boot_info *info);
extern int init_paging(void);

void init_memory(const struct boot_info *info)
{
    print_meminfo(info);
    init_paging();

    // uint32_t addr = 0x100000;
    // assert(map_page(addr, get_pfn(addr), 0) == 0);
    // assert(map_page(addr, get_pfn(addr), 0) == -EINVAL);

    // addr = 0x400000;
    // assert(map_page(addr, get_pfn(addr), MAP_LARGE) == 0);
    // assert(map_page(addr, get_pfn(addr), MAP_LARGE) == -EINVAL);
}

static void print_meminfo(const struct boot_info *info)
{
    int kb_total = 0;
    int kb_free = 0;
    int kb_reserved = 0;
    int kb_acpi = 0;
    int kb_bad = 0;

    int kb_free_low = 0;    // between 0 and 640k
    int kb_free_1M = 0;     // between 1M and 16M
    int kb_free_16M = 0;    // between 1M and 4G

    if (!info->mem_map) {
        kprint("bios-e820: memory map not available\n");
        if (info->kb_high_e801h != 0) {
            kb_free_1M = info->kb_high_e801h;
            kb_free_16M = (info->kb_extended << 6);
        }
        else {
            kprint("bios-e801: memory map not available\n");
            kb_free_1M = info->kb_high;
        }
        kb_free_low = info->kb_low;
        kb_free = kb_free_low + kb_free_1M + kb_free_16M;
    }
    else {
        const acpi_mmap_t *e = info->mem_map;
        while (e->type != 0) {
            uint32_t base = (uint32_t) e->base;
            uint32_t limit = (uint32_t) e->length - 1;

#if SHOW_MEMMAP
            kprint("bios-e820: %08lX-%08lX ", base, base+limit, e->attributes, e->type);
            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE: kprint("free"); break;
                case ACPI_MMAP_TYPE_RESERVED: kprint("reserved"); break;
                case ACPI_MMAP_TYPE_ACPI: kprint("reserved ACPI"); break;
                case ACPI_MMAP_TYPE_ACPI_NVS: kprint("reserved ACPI non-volatile"); break;
                case ACPI_MMAP_TYPE_BAD: kprint("bad"); break;
                default: kprint("unknown (%d)", e->type); break;
            }
            if (e->attributes) {
                kprint(" (attributes = %X)", e->attributes);
            }
            kprint("\n");
#endif

            // TODO: kb count does not account for overlapping regions

            int kb = (e->length >> 10);
            kb_total += kb;

            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE:
                    kb_free += kb;
                    break;
                case ACPI_MMAP_TYPE_ACPI:
                case ACPI_MMAP_TYPE_ACPI_NVS:
                    kb_acpi += kb;
                    break;
                case ACPI_MMAP_TYPE_BAD:
                    kb_bad += kb;
                    break;
                default:
                    kb_reserved += kb;
                    break;
            }

            e++;
        }
    }

    kprint("boot: %dk free", kb_free);
    if (kb_total) kprint(", %dk total", kb_total);
    if (kb_bad) kprint(", %dk bad", kb_bad);
    kprint("\n");
    if (kb_free < MIN_KB_REQUIRED) {
        panic("we need at least %dk of RAM to operate!", MIN_KB_REQUIRED);
    }
}
