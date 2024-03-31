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
#include <interrupt.h>
#include <io.h>
#include <irq.h>
#include <pic.h>
#include <ps2.h>
#include <test.h>
#include <x86.h>
#include <syscall.h>

#define INIT_STACK          0x7C00

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct boot_info *info);
extern void init_memory(const struct boot_info *info);
extern void init_pic(void);
extern void init_irq(void);
extern void init_ps2(const struct boot_info *info);
extern void init_kb(void);
extern void init_timer(void);
extern void init_tasks(void);
#ifdef TEST_BUILD
extern void tmain(void);
#endif

static void enter_ring3(void (*entry), uint32_t stack);
static void print_info(const struct boot_info *info);

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

    cli();

    zeromem(&g_boot, sizeof(struct boot_info));
    memcpy(&g_boot, info, sizeof(struct boot_info));

    init_cpu(&g_boot);
    init_pic();
    init_irq();
    init_vga();
    init_console();

    // safe to print now
    kprint("\n" OS_NAME " " OS_VERSION " '" OS_MONIKER "'\n");
    kprint("Build: " OS_BUILDDATE "\n");
    kprint("\n");

    print_info(&g_boot);

#if TEST_BUILD
    kprint("boot: TEST BUILD\n");
    kprint("boot: running tests...\n");
    tmain();
#endif

    init_memory(&g_boot);
    init_ps2(&g_boot);
    init_kb();
    init_timer();
    init_tasks();

    enter_ring3(init, INIT_STACK);
}

static void enter_ring3(void (*entry), uint32_t stack)
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

    // intptr_t kernel_end = info->kernel+info->kernel_size-1;
    // intptr_t stage2_end = info->stage2+info->stage2_size-1;

    kprint("boot: boot info found at %08X\n", info);
    kprint("boot: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("boot: %s ps/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("boot: video mode is %02X\n", info->video_mode);
    kprint("boot: stage2 %08X,%X\n", info->stage2, info->stage2_size);
    kprint("boot: kernel %08X,%X\n", info->kernel, info->kernel_size);
    kprint("boot: stack %08X\n", info->stack);
    if (info->ebda) kprint("boot: EBDA %08X\n", info->ebda);
    if (info->mem_map) kprint("boot: ACPI memory map %08X\n", info->mem_map);
}
