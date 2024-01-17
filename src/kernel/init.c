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
 *         File: kernel/init.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdio.h>
#include <string.h>

#include <ohwes.h>
#include <boot.h>
#include <cpu.h>
#include <irq.h>
#include <interrupt.h>
#include <test.h>
#include <x86.h>

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct bootinfo * const info);
extern void init_irq(void);
__fastcall extern void switch_context(struct iregs *regs);

void divide_by_zero(void)
{
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    (void) c;
}

int ring3_start(void)
{
    printf("got to ring3!\n");  // note: requires IOPL=3 due to console_write
    divide_by_zero();
    return 8675309;
}

__noreturn
void ring3_exit(void)
{
    int status = 0;
    store_eax(status);

    _syscall1(SYS_EXIT, status);
    die();
}

__noreturn
void go_to_ring3(void *func)
{
    struct eflags eflags;
    cli_save(eflags);

    eflags.intf = 1;        // enable interrupts
    eflags.iopl = USER_PL;  // so printf works

    uint32_t *const ebp = (uint32_t * const) 0xC000;    // user stack
    uint32_t *esp = ebp;
    ebp[-1] = (uint32_t) ring3_exit;
    esp = &ebp[-1];

    struct iregs regs = {};
    regs.cs = USER_CS;
    regs.ds = USER_DS;
    regs.es = USER_DS;
    regs.ss = USER_SS;
    regs.ebp = (uint32_t) ebp;
    regs.esp = (uint32_t) esp;
    regs.eip = (uint32_t) func;
    regs.eflags = eflags._value;
    switch_context(&regs);
    die();
}

__fastcall
void kmain(const struct bootinfo * const bootinfo)
{
    // --- Crude Memory Map ---
    // 0x00000-0x004FF: reserved for Real Mode IVT and BDA (do we still need this?)
    // 0x00500-0x007FF: ACPI memory map table
    // 0x00800-0x00FFF: CPU data area (GDT/IDT/LDT/TSS/etc.)
    // 0x02400-0x0????: (<= 0xDC00 bytes of free space)
    // 0x07C00-0x07DFF: stage 1 boot loader (potentially free; contains BPB)
    // 0x07E00-0x0????: stage 2 boot loader (potentially free; contains bootinfo)
    // 0x0????-0x0FFFF: kernel stack (grows towards 0)
    // 0x10000-(EBDA ): kernel and system

    struct bootinfo info;
    memcpy(&info, bootinfo, sizeof(struct bootinfo));

    init_vga();
    init_console();
    init_cpu(&info);
    init_irq();
#if TEST_BUILD
    test_libc();
#endif

    irq_unmask(IRQ_KEYBOARD);
    go_to_ring3(ring3_start);
    // divide_by_zero();
}
