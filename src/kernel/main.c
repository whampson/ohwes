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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ohwes.h>
#include <boot.h>
#include <cpu.h>
#include <interrupt.h>
#include <io.h>
#include <irq.h>
#include <keyboard.h>
#include <pic.h>
#include <ps2.h>
#include <test.h>
#include <x86.h>

extern void init_vga(void);
extern void init_console(void);
extern void init_cpu(const struct bootinfo * const info);
extern void init_pic(void);
extern void init_irq(void);
extern void init_ps2(const struct bootinfo * const info);
extern void init_kb(void);
extern void init_memory(const struct bootinfo * const info);

static void print_info(const struct bootinfo *const info)
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
    kprint("boot: EBDA %08X\n", info->ebda);
}

__noreturn void go_to_ring3(void);

__fastcall
void kmain(const struct bootinfo *const bootinfo)
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

    cli();

    init_vga();
    init_console();
    // safe to print now

    kprint("\n" OS_NAME " " OS_VERSION " '" OS_MONIKER "'\n");
    kprint("Build: " OS_BUILDDATE "\n");
    kprint("\n");

#if TEST_BUILD
    kprint("boot: TEST BUILD\n");
    kprint("boot: testing libc...\n");
    test_libc();
#endif

    struct bootinfo info;
    memcpy(&info, bootinfo, sizeof(struct bootinfo));
    print_info(&info);

    init_cpu(&info);
    init_pic();
    init_irq();
    // safe to enable interrupts now

    sti();

    init_ps2(&info);
    init_kb();
    init_memory(&info);

    go_to_ring3(); // "returns" to sys_exit via system call
}

int sys_exit(int status)
{
    kprint("ring 3 returned %d\n", status);

    kprint("draining input buffer...\n");
    while (true) {
        uint8_t sc;
        while ((sc = kb_read())) {
            // kprint("got %02X\n", sc);
            // idiotic testing code
            switch (sc)
            {
                case 0x44:  // f10
                    ps2_cmd(PS2_CMD_SYSRESET);  // reset
                    break;
                case 0x3B:  // f1
                    kprint("\e3");  // blink off
                    break;
                case 0x3C:  // f2
                    kprint("\e4");  // blink on
                    break;
                // case 0xD8:  // f12
                //     __asm__ volatile ("int $69");   // crash
                //     break;
            }
        }
    }

    idle();
    return 0;
}

__noreturn
void _dopanic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    kprint("panic: ");
    vprintf(fmt, args);

    va_end(args);
    die();
}
