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


#define YN(cond)    ((cond) ? "yes" : "no")

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

    intptr_t kernel_end = info->kernel+info->kernel_size-1;
    intptr_t stage2_end = info->stage2+info->stage2_size-1;

    printf("boot info found at %08x\n", info);
    printf("%12s: %02x\n",      "video mode",      info->video_mode);
    printf("%12s: %d\n",        "floppy",   nfloppies);
    printf("%12s: %d\n",        "serial",    nserial);
    printf("%12s: %d\n",        "parallel",  nparallel);
    printf("%12s: %s\n",        "game port",       YN(gameport));
    printf("%12s: %s\n",        "ps/2 mouse",      YN(mouse));
    printf("%12s: %08x-%08x\n", "kernel",          info->kernel, kernel_end);
    printf("%12s: %08x-%08x\n", "stage2",          info->stage2, stage2_end);
    printf("%12s: %08x\n",      "stack",           info->stack);
    printf("%12s: %08x\n",      "ebda",            info->ebda);
}

__noreturn void go_to_ring3(void);

__fastcall
void kmain(const struct bootinfo *const info)
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
    printf(OS_NAME " " OS_VERSION " '" OS_MONIKER "'\n");
    printf("build: " OS_BUILDDATE "\n");

#if TEST_BUILD
    printf("TEST BUILD\n");
    printf("testing libc...\n");
    test_libc();
#endif

    print_info(info);

    init_cpu(info);
    init_pic();
    init_kbd();
    init_memory(info);

    sti();

    go_to_ring3(); // "returns" to sys_exit via system call
}

int sys_exit(int status)
{
    printf("back in the kernel... idling CPU\n");
    printf("user mode returned %d\n", status);
    // gpfault();

    halt(); // TODO: switch to next task
    return 0;
}

__noreturn
void _dopanic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("panic: ");
    vprintf(fmt, args);

    va_end(args);
    die();
}