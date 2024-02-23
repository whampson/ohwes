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
extern void init_irq(void);
extern void init_pic(void);
extern void init_ps2(const struct bootinfo * const info);
extern void kbd_init(void);
extern void init_memory(const struct bootinfo * const info);

struct kb {
    bool req_ack;
    bool got_ack;
    bool resend;
    bool interrupt;
};

struct kb g_kb = {};

void recv_keypress(void)
{
    uint8_t scancode;

    g_kb.interrupt = true;

    scancode = inb(0x60);
    switch (scancode) {
        case 0xFA:
            if (!g_kb.req_ack) {
                kprint("kbd_interrupt: got unexpected ACK!\n");
            }
            else {
                kprint("kbd_interrupt: got ACK\n");
            }
            g_kb.got_ack = true;
            g_kb.req_ack = false;
            return;

        case 0xFE:
            kprint("kbd_interrupt: got resend request\n");
            g_kb.resend = true;
            return;

        default:
            if (g_kb.req_ack) {
                kprint("kbd_interrupt: expected ACK, got 0x%X\n", scancode);
            }
            break;
    }

    // idiotic testing code
    switch (scancode)
    {
        case 0x76:  // esc
            ps2_cmd(PS2_CMD_SYSRESET);  // reset
            break;
        case 0x05:  // f1
            kprint("\e3");  // blink off
            break;
        case 0x06:  // f2
            kprint("\e4");  // blink on
            break;
        case 0x07:  // f12
            __asm__ volatile ("int $69");   // crash
            break;
    }

    kprint("kbd_interrupt: scancode %02X\n", scancode);
}

#define SCANCODE_SET 2
#define NUM_RETRIES 10000

uint8_t kbd_read()
{
    uint8_t data;
    uint32_t flags;
    int count;

    count = 0;
    while (count++ < NUM_RETRIES) {
        data = inb_delay(0x64);
        if (data & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) {
            panic("ps2kbd: read failed with timeout or parity error");
        }
        if (data & (PS2_STATUS_OPF) ||
            g_kb.got_ack || g_kb.resend || g_kb.interrupt) {
            break;
        }
    }

    cli_save(flags);

    g_kb.got_ack = false;
    g_kb.req_ack = false;
    g_kb.resend = false;

    if (count >= NUM_RETRIES) {
        kprint("ps2kbd: timed out waiting for read\n");
        data = 0xFF;
        goto done;
    }

    data = inb_delay(0x60);
    if (data == 0x00 || data == 0xFF) {
        kprint("ps2kbd: returned error 0x%X\n", data);
    }
    kprint("ps2kbd: returned 0x%X\n", data);

done:
    restore_flags(flags);
    return data;
}

bool kbd_write(uint8_t data)
{
    uint8_t status;
    uint8_t resp;
    uint32_t flags;
    int count;

    cli_save(flags);

    // TODO: resend last command if requested?

    count = 0;
    while (count++ < NUM_RETRIES) {
        status = inb_delay(0x64);
        if (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) {
            panic("ps2kbd: write failed with timeout or parity error");
        }
        if (!(status & PS2_STATUS_IPF)) {
            break;
        }
    }

    if (count >= NUM_RETRIES) {
        kprint("ps2kbd: timed out waiting for write");
    }

    // tODO: need a queue/ring buffer

    kprint("ps2kbd: writing 0x%X...\n", data);

    g_kb.interrupt = false;
    g_kb.got_ack = false;
    g_kb.req_ack = true;
    g_kb.resend = false;

    outb_delay(0x60, data);

    restore_flags(flags);

    resp = kbd_read();
    return resp != 0x00 && resp != 0xFF;
}

void init_keyboard(void)
{
    uint8_t data;

    kprint("ps2kbd: enabling IRQ...\n", SCANCODE_SET);
    irq_register(IRQ_KEYBOARD, recv_keypress);
    pic_unmask(IRQ_KEYBOARD);

    sti();  // temp for testing keyboard state machine with interrupts on

    kbd_write(PS2KBD_CMD_SETLED);
    kbd_write(PS2KBD_LED_NUMLK | PS2KBD_LED_CAPLK);

    if (!kbd_write(PS2KBD_CMD_SCANCODE)) {
        kprint("ps2kbd: error setting scancode, defaulting to scancode set 1\n");
        data = 1;
    }
    else {
        kbd_write(0);
        data = kbd_read();
    }
    kprint("ps2kbd: using scancode set %d\n", data);
}

#define HASNO(cond)     ((cond)?"has":"no")
#define YN(cond)        ((cond)?"yes":"no")
#define PLURAL(n,a)     (((n)==1)?a:a "s")
#define PLURAL2(n,a,b)  (((n)==1)?a:b)

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

#if TEST_BUILD
    kprint("boot: TEST BUILD\n");
    kprint("boot: testing libc...\n");
    test_libc();
#endif

    struct bootinfo info;
    memcpy(&info, bootinfo, sizeof(struct bootinfo));
    print_info(&info);

    init_cpu(&info);
    init_irq();
    init_pic();
    init_ps2(&info);
    init_keyboard();
    init_memory(&info);

    kprint("\n" OS_NAME " " OS_VERSION " '" OS_MONIKER "'\n");
    kprint("Build: " OS_BUILDDATE "\n");
    kprint("\n");

    sti();

    go_to_ring3(); // "returns" to sys_exit via system call
}

int sys_exit(int status)
{
    kprint("Ring 3 returned %d\n", status);

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
