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
 *         File: kernel/main.c
 *      Created: January 22, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdio.h>
#include <string.h>

#include <ohwes.h>
#include <boot.h>
#include <cpu.h>
#include <interrupt.h>
#include <io.h>
#include <irq.h>
#include <pic.h>
#include <test.h>
#include <x86.h>

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct bootinfo * const info);
extern void init_pic(void);
extern void init_memory(const struct bootinfo * const info);


void recv_keypress(void)
{
    uint8_t scancode = inb(0x60);
    printf("got scancode %d\n", scancode);

    // idiotic testing code
    if (scancode == 69) {
        __asm__ volatile ("int $69");   // crash
    }
    if (scancode == 73) {
        printf("\e3");  // blink off
    }
    if (scancode == 74) {
        printf("\e4");  // blink on
    }
}

void init_kbd(void)
{
    irq_register(IRQ_KEYBOARD, recv_keypress);
    pic_unmask(IRQ_KEYBOARD);
}

int getpl()
{
    struct segsel cs;
    store_cs(cs);

    return cs.rpl;
}

void gpfault(void)
{
    __asm__ volatile ("int $69");
}

void divide_by_zero(void)
{
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    (void) c;
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


int main(void);

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

    cli();
    init_vga();
    init_console();
    printf("\ecOH-WES 0.1 'Ronnie Raven'\n");    // Ronald Reagan lol
#if TEST_BUILD
    printf("TEST BUILD\n");
    printf("testing libc\n");
    test_libc();
#endif
    printf("init CPU descriptors\n");
    init_cpu(&info);
    printf("init PIC\n");
    init_pic();
    printf("init devices\n");
    init_kbd();
    printf("init memory\n");
    init_memory(&info);
    sti();

    go_to_ring3(main);
    // "returns" to sys_exit
}

int sys_exit(int status)
{
    printf("back in the kernel... idling CPU\n");
    printf("user mode returned %d\n", status);
    // gpfault();

    halt(); // TODO: switch to next task
    return 0;
}

int main(void)
{
    printf("got to ring3!\n");  // note: requires IOPL=3 due to console_write
    printf("pl = %d\n", getpl());
    // gpfault();

    return 8675309;
}
