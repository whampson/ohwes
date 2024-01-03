/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/init.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <boot.h>
#include <io.h>
#include <interrupt.h>
#include <os.h>

#define OS_NAME_STRING      "OHWES"
#define OS_VERSION_STRING   "0.1"
#define OS_COPYRIGHT_STRING "(C) 2020-2023 Wes Hampson. All Rights Reserved."

#define BootPrint(...)  printf("boot: " __VA_ARGS__)

extern bool run_tests(void);
extern void con_init(void);
extern void init_cpu(void);

BootInfo g_bootInfo;
BootInfo * const g_pBootInfo = &g_bootInfo;

void test_klibc()
{
#ifdef TEST_BUILD
    bool pass = run_tests();
    if (!pass) {
        panic("tests failed!");
    }
#endif
}

void print_memmap(void)
{
    const AcpiMemoryMapEntry *memMap = g_pBootInfo->pMemoryMap;
    if (!g_pBootInfo->pMemoryMap) {
        BootPrint("ACPI memory map not found!\n");
        return;
    }

    do {
        char buf[64];
        char *p = buf;
        if (memMap->length > 0) {
            int n = snprintf(buf, sizeof(buf), "BIOS-E820h: %08x-%08x ",
                (uint32_t) memMap->base,
                (uint32_t) memMap->base + memMap->length - 1);
            p = &buf[n];
            size_t s = sizeof(buf) - (p - buf);
            switch (memMap->type) {
                case ACPI_MMAP_TYPE_USABLE:     snprintf(p, s, "usable"); break;
                case ACPI_MMAP_TYPE_RESERVED:   snprintf(p, s, "reserved"); break;
                case ACPI_MMAP_TYPE_ACPI:       snprintf(p, s, "ACPI"); break;
                case ACPI_MMAP_TYPE_ACPI_NVS:   snprintf(p, s, "ACPI NV"); break;
                case ACPI_MMAP_TYPE_BAD:        snprintf(p, s, "bad"); break;
                default:                        snprintf(p, s, "reserved (%d)", memMap->type); break;
            }
            BootPrint("%s\n", buf);
        }
    } while ((memMap++)->type != ACPI_MMAP_TYPE_INVALID);
}

__fastcall
void kmain(const BootInfo * const pBootInfo)
{
    memcpy(g_pBootInfo, pBootInfo, sizeof(BootInfo));

    con_init();     // get the vga console working first
    init_cpu();     // then finish setting up the CPU.

    test_klibc();   // run some tests on the kernel runtime library

    print_memmap();

    printf("INT8_MIN = %d\n", INT8_MIN);
    printf("INT_LEAST8_MIN = %d\n", INT_LEAST8_MIN);
    printf("INT8_MIN = %d\n", INT8_MAX);
    printf("INT_LEAST8_MIN = %d\n", INT_LEAST8_MAX);

    volatile int x = 3;
    assert(x == 3);
    assert(x == 4);

    (void) x;

}
