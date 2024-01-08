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
extern void init_cpu(void);
extern void init_irq(void);

BootInfo g_bootInfo;
BootInfo * const g_pBootInfo = &g_bootInfo;

static void run_tests(void)
{
#if TEST_BUILD
    test_libc();
#endif
}

__fastcall
static void ring3_test(void)
{
    volatile int a = 0;
    __asm__ volatile ("movl $1, %eax; int $0x80");
    __asm__ volatile ("movl $2, %eax; int $0x80");
    __asm__ volatile ("movl $3, %eax; int $0x80");
    while (true) { (void) a; }
}

#define KERNEL_CS  0x10
#define KERNEL_DS  0x18
#define USER_CS    0x23
#define USER_DS    0x2B

struct iframe {
    uint32_t eip;       // pushed by cpu
    uint32_t cs;        // pushed by cpu
    uint32_t eflags;    // pushed by cpu
    uint32_t esp;       // pushed by cpu, only present when privilege level changes
    uint32_t ss;        // pushed by cpu, only present when privilege level changes
};

__fastcall
void kmain(const BootInfo * const pBootInfo)
{
    memcpy(g_pBootInfo, pBootInfo, sizeof(BootInfo));

    uint32_t flags;
    cli_save(flags);

    con_init();     // get the vga console working first
    init_cpu();     // then finish setting up the CPU.
    init_irq();
    run_tests();

    __asm__ volatile ("movl $1, %eax; int $0x80");
    __asm__ volatile ("movl $2, %eax; int $0x80");
    __asm__ volatile ("movl $3, %eax; int $0x80");

    __asm__ volatile (          // TODO: crashes on Bochs and real hardware...
        "           \n\
        pushl %0    \n\
        pushl %1    \n\
        pushl %2    \n\
        pushl %3    \n\
        pushl %4    \n\
        iret        \n\
        " : :
        "r"(USER_DS),
        "r"(0x7C00),
        "r"(flags),
        "r"(USER_CS),
        "i"(ring3_test));
}
