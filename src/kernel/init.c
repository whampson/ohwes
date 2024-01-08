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

#include <stdio.h>
#include <string.h>

#include <boot.h>
#include <ohwes.h>
#include <irq.h>
#include <interrupt.h>
#include <test.h>
#include <x86.h>

extern void con_init(void);
extern void init_cpu(const struct bootinfo * const info);
extern void init_irq(void);

static void run_tests(void)
{
#if TEST_BUILD
    test_libc();
#endif
}

// --- Crude Memory Map ---
// 0x00000-0x004FF: reserved for Real Mode IVT and BDA (do we still need this?)
// 0x00500-0x007FF: ACPI memory map table
// 0x00800-0x00FFF: CPU data area (GDT/IDT/LDT/TSS/etc.)
// 0x02400-0x0????: (<= 0xDC00 bytes of free space)
//  (0x07C00-0x07DFF: stage 1 boot loader)
//  (0x07E00-0x0????: stage 2 boot loader)
// 0x0????-0x0FFFF: kernel stack (grows towards 0)
// 0x10000-(EBDA ): kernel and system

__fastcall
void kmain(const struct bootinfo * const info)
{
    con_init();
    init_cpu(info);
    init_irq();
    run_tests();
}
