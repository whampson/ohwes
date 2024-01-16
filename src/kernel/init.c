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

#include <boot.h>
#include <ohwes.h>
#include <irq.h>
#include <interrupt.h>
#include <test.h>
#include <x86.h>

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct bootinfo * const info);
extern void init_irq(void);

static void run_tests(void)
{
#if TEST_BUILD
    test_libc();
#endif
}

__fastcall
void divide_by_zero(void)
{
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    (void) c;
}

__fastcall
void ring3_test(void)
{
    // TODO: something's wrong on Bochs/VBox:
    //   triple faults as soon as it executes the first instruction that accesses
    //   memory

    // divide_by_zero();
    // printf("got to ring3!");
    ((char *) 0xB8000)[0] = '@';
    die();  // TODO: need a way to exit back to the kernel
}

__noreturn
void go_to_ring3(void *func)
{
    struct eflags eflags;
    cli_save(eflags);

    eflags.intf = 1;
    eflags.iopl = 0;

#define TEMP_USER_STACK_SIZE 64

    intptr_t temp_user_stack[TEMP_USER_STACK_SIZE];
    intptr_t *esp = &temp_user_stack[TEMP_USER_STACK_SIZE-1];

    __asm__ volatile (
        "               \n\
        pushl   %0      \n\
        pushl   %1      \n\
        pushl   %2      \n\
        pushl   %3      \n\
        pushl   %4      \n\
        iret            \n\
        "
        :
        : "i"(USER_SS),
          "r"(esp),
          "r"(eflags),
          "i"(USER_CS),
          "r"(func)
    );

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
    run_tests();

    irq_unmask(IRQ_KEYBOARD);

    go_to_ring3(ring3_test);
    // divide_by_zero();

    // __asm__ volatile ("int $0x81");
}
