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

#include <ohwes.h>
#include <boot.h>
#include <console.h>
#include <ctype.h>
#include <cpu.h>
#include <errno.h>
#include <interrupt.h>
#include <io.h>
#include <irq.h>
#include <paging.h>
#include <pic.h>
#include <ps2.h>
#include <test.h>
#include <x86.h>
#include <syscall.h>
#include <debug.h>

#define CHATTY 1

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct boot_info *info);
extern void init_memory(const struct boot_info *info);
extern void init_pic(void);
extern void init_irq(void);
extern void init_ps2(const struct boot_info *info);
extern void init_kb(void);
extern void init_timer(void);
extern void init_rtc(void);
extern void init_tasks(void);
#ifdef TEST_BUILD
extern void tmain(void);
#endif

extern void init(void);     // usermode entry point

static void enter_ring3(uint32_t entry, uint32_t stack);
static void print_info(const struct boot_info *info);

#ifdef DEBUG
int g_test_crash_kernel;
static void debug_interrupt(void);
#endif

struct boot_info g_boot;

__fastcall void kmain(const struct boot_info *info)
{
    // --- Crude Memory Map upon entry ---
    // 0x00000-0x004FF: reserved for Real Mode IVT and BDA (do we still need this?)
    // 0x00500-0x007FF: ACPI memory map table
    // 0x00800-0x00FFF: CPU data area (GDT/IDT/LDT/TSS/etc.)
    // 0x02400-0x0????: (<= 0xDC00 bytes of free space)
    // 0x07C00-0x07DFF: stage 1 boot loader (potentially free; contains BPB)
    // 0x07E00-0x0????: stage 2 boot loader (potentially free; contains boot_info)
    // 0x0????-0x0FFFF: kernel stack (grows towards 0)
    // 0x10000-(EBDA ): kernel and system

    __cli();
    memcpy(&g_boot, info, sizeof(struct boot_info));
    info = &g_boot;     // reassign ptr so we don't lose access to the data

    init_pic();
    init_irq();
    init_vga();
    init_console();     // safe to print now

#if TEST_BUILD
    kprint("boot: TEST BUILD\n");
    kprint("boot: running tests...\n");
    tmain();
#endif

    kprint("\n" OS_NAME " " OS_VERSION ", build " OS_BUILDDATE "\n");
    kprint("\n");
#if CHATTY
    print_info(&g_boot);
#endif

    init_cpu(&g_boot);
    init_ps2(&g_boot);
    init_kb();
    init_memory(&g_boot);
    init_timer();
    init_rtc();
    init_tasks();

#ifdef DEBUG
    g_test_crash_kernel = 0;
    irq_register(IRQ_RTC, debug_interrupt);
#endif

    // ----------------------------------------------------------------------
    // TODO: eventually this should load a program called 'init'
    // that forks itself and spawns the shell program
    // (if we're following the Unix model)

    assert(info->init_size > 0);
    assert(info->init_size <= 0x10000);

    kprint("boot: entering ring3...\n");
    const uint32_t init_base = INIT_BASE;
    const uint32_t user_stack = USER_STACK_PAGE + PAGE_SIZE;    // start at top
    enter_ring3(init_base, user_stack);
}

static void enter_ring3(uint32_t entry, uint32_t stack)
{
    assert(getpl() == KERNEL_PL);

    // tweak flags
    struct eflags eflags;
    cli_save(eflags);
    eflags.intf = 1;        // enable interrupts

    // ring 3 initial register context
    struct iregs regs = {};
    regs.cs = USER_CS;
    regs.ss = USER_SS;
    regs.ds = USER_DS;
    regs.es = USER_DS;
    regs.ebp = stack;
    regs.esp = stack;
    regs.eip = (uint32_t) entry;
    regs.eflags = eflags._value;

    // drop to ring 3
    switch_context(&regs);
    die();
}

static void print_info(const struct boot_info *info)
{
    int nfloppies = info->hwflags.has_diskette_drive;
    if (nfloppies) {
        nfloppies += info->hwflags.num_other_diskette_drives;
    }

    int nserial = info->hwflags.num_serial_ports;
    int nparallel = info->hwflags.num_parallel_ports;
    bool gameport = info->hwflags.has_gameport;
    bool mouse = info->hwflags.has_ps2mouse;

    kprint("boot: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("boot: %s ps/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("boot: video mode is %02Xh\n", info->video_mode);
    kprint("boot: stage2:\t\t%08X,%Xh\n", info->stage2, info->stage2_size);
    kprint("boot: kernel:\t\t%08X,%Xh\n", info->kernel, info->kernel_size);
    kprint("boot: init:\t\t%08X,%Xh\n", INIT_BASE, info->init_size);
    kprint("boot: framebuf:\t%08X,%Xh\n", info->framebuffer, info->framebuffer_pages << PAGE_SHIFT);
    kprint("boot: kernel stack:\t%08Xh\n", info->stack);
    kprint("boot: user stack:\t%08Xh\n", USER_STACK_PAGE + PAGE_SIZE);
    if (info->ebda) kprint("boot: EBDA\t\t%08Xh\n", info->ebda);
}

#ifdef DEBUG
static void debug_interrupt(void)
{
    switch (g_test_crash_kernel) {
        case 1:
            divzero();
            break;
        case 2:
            softnmi();
            break;
        case 3:
            dbgbrk();
            break;
        case 4:
            assert(true == false);
            break;
        case 5:
            testint();
            break;
        case 6:
            panic("you fucked up!!");
            break;
        case 7:
            __asm__ volatile ("int $0x2D");
            break;
        case 8: {
            volatile uint32_t *badptr = (uint32_t *) 0xCA55E77E;
            *badptr = 0xBADC0DE;
            break;
        }
        case 9: {
            volatile uint32_t *badptr = NULL;
            const int bad  = *badptr;
            (void) bad;
            break;
        }
    }

    g_test_crash_kernel = 0;
}
#endif
