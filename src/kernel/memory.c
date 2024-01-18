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

#include <boot.h>

void init_memory(const struct bootinfo * const info)
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
        printf("  bios-e820 memory map not available\n");
        if (info->kb_high_e801h != 0) {
            kb_free_1M = info->kb_high_e801h;
            kb_free_16M = (info->kb_extended << 6);
            // printf("\nbios-e801: ");
        }
        else {
            printf("  bios-e801 memory map not available\n");
            kb_free_1M = info->kb_high;
            // printf("\nbios-88: ");
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
            printf("  %08lx-%08lx ", base, base+limit, e->attributes, e->type);
            switch (e->type) {
                case ACPI_MMAP_TYPE_USABLE: printf("free"); break;
                case ACPI_MMAP_TYPE_RESERVED: printf("reserved"); break;
                case ACPI_MMAP_TYPE_ACPI: printf("acpi"); break;
                case ACPI_MMAP_TYPE_ACPI_NVS: printf("acpi non-volatile"); break;
                case ACPI_MMAP_TYPE_BAD: printf("bad"); break;
                default: printf("unknown (%d)", e->type); break;
            }
            if (e->attributes) {
                printf(" (attributes = %x)", e->attributes);
            }
            printf("\n");
#endif

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

    printf("%10d KB free\n", kb_free);
    if (kb_reserved) printf("%10d KB reserved\n", kb_reserved);
    if (kb_acpi) printf("%10d KB reserved for ACPI\n", kb_acpi);
    if (kb_bad) printf("%10d KB deemed bad :(\n", kb_bad);
    if (kb_total) printf("%10d KB total\n", kb_total);

    if (kb_free < MIN_KB_REQUIRED) {
        panic("need at least %d KB of RAM to operate!", MIN_KB_REQUIRED);
    }
}
