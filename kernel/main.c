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
#include <kernel.h>
#include <paging.h>
#include <pic.h>
#include <ps2.h>
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
typedef int (*test_main)(void);
extern void tmain(void);
#endif

int init(void);     // usermode entry point

static void basic_shell(void);
static size_t /*
    it is now my duty to completely
*/  drain_queue(struct char_queue *q, char *buf, size_t bufsiz);

static void enter_ring3(uint32_t entry, uint32_t stack);
static void print_info(const struct boot_info *info);

#ifdef DEBUG
int g_test_crash_kernel;
static void debug_interrupt(void);
#endif

struct boot_info g_boot;

__fastcall void kmain(const struct boot_info *info)
{
    memcpy(&g_boot, info, sizeof(struct boot_info));

    init_cpu(&g_boot);
    init_pic();
    init_irq();
    init_vga();
    init_tasks();
    init_console();     // safe to print now

    kprint("\n" OS_NAME " " OS_VERSION ", build " OS_BUILDDATE "\n");
    kprint("\n");
#if CHATTY
    print_info(&g_boot);
#endif

    init_ps2(&g_boot);
    init_kb();
    init_memory(&g_boot);
    init_timer();
    init_rtc();

#ifdef DEBUG
    // CTRL+ALT+FN to crash kernel
    irq_register(IRQ_TIMER, debug_interrupt);
#endif

    kprint("boot: entering ring3...\n");
    uint32_t user_stack = 0xD000;
    enter_ring3((uint32_t) init, user_stack);
}

int init(void)
{

//
// Runs in ring 3.
//
    assert(getpl() == USER_PL);

    // TODO: load shell program from disk

    printf("\e4\e[5;33mHello, world!\e[m\n");
    basic_shell();

    return 0;
}

static void basic_shell(void)
{
#define INPUT_LEN 128

    char _lineq_buf[INPUT_LEN];   // TOOD: NEED AN ALLOCATOR
    struct char_queue _lineq;
    struct char_queue *lineq = &_lineq;

    char line[INPUT_LEN];

    char c;
    int count;
    const char *prompt = "&";

    char_queue_init(lineq, _lineq_buf, sizeof(_lineq_buf));

    while (true) {
        // read a line
        printf(prompt);
        line[0] = '\0';

    read_char:
        // read a character
        count = read(stdin_fd, &c, 1);
        if (count == 0) {
            panic("read returned 0!");
        }
        assert(count == 1);

        //
        // TODO: all this line processing stuff needs to go in the terminal line discipline
        //

        // handle special characters and translations
        switch (c) {
            case '\b':      // ECHOE
                break;
            case '\r':
                c = '\n';   // ICRNL
                break;
            case 3:
                exit(1);    // CTRL+C
                break;
            case 4:
                exit(0);    // CTRL+D
                break;
        }

        bool full = (char_queue_count(lineq) == char_queue_length(lineq) - 1);    // allow one space for newline

        // put translated character into queue
        if (c == '\b') {
            if (char_queue_empty(lineq)) {
                printf("\a");       // beep!
                goto read_char;
            }
            c = char_queue_erase(lineq);
            if (iscntrl(c)) {
                printf("\b");
            }
            printf("\b");
            goto read_char;
        }
        else if (c == '\n' || !full) {
            char_queue_put(lineq, c);
        }
        else if (full) {
            printf("\a");       // beep!
            goto read_char;
        }

        assert(!char_queue_full(lineq));

        // echo char
        if (iscntrl(c) && c != '\t' && c != '\n') {
            printf("^%c", 0x40 ^ c);        // ECHOCTL
        }
        else {
            printf("%c", c);                // ECHO
        }

        // next char if not newline
        if (c != '\n') {
            goto read_char;
        }

        // get and print entire line
        size_t count = drain_queue(lineq, line, sizeof(line));
        if (strncmp(line, "\n", count) != 0) {
            printf("%.*s", (int) count, line);
        }

        //
        // process command line
        //
        if (strncmp(line, "cls\n", count) == 0) {
            printf("\033[2J");
        }
        if (strncmp(line, "exit\n", count) == 0) {
            exit(0);
        }
    }
}

static size_t drain_queue(struct char_queue *q, char *buf, size_t bufsiz)
{
    size_t count = 0;
    while (!char_queue_empty(q) && bufsiz--) {
        *buf++ = char_queue_get(q);
        count++;
    }

    if (bufsiz > 0) {
        *buf = '\0';
    }
    return count;
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
    regs.ss = USER_DS;
    regs.ds = USER_DS;
    regs.es = USER_DS;
    regs.ebp = stack;
    regs.esp = stack;
    regs.eip = entry;
    regs.eflags = eflags._value;

    // drop to ring 3
    switch_context(&regs);
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
    uint32_t ebda_size = 0xA0000 - info->ebda_base;

    kprint("bios: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("bios: %s ps/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("bios: video mode is %02Xh\n", info->vga_mode & 0x7F);
    if (info->ebda_base) kprint("boot: EBDA:\t%08X,%Xh\n", info->ebda_base, ebda_size);
    kprint("boot: stage2:\t%08X,%Xh\n", info->stage2_base, info->stage2_size);
    kprint("boot: kernel:\t%08X,%Xh\n", info->kernel_base, info->kernel_size);
}

#ifdef DEBUG
static void debug_interrupt(void)
{
    switch (g_test_crash_kernel) {
        case 1:     // F1 - divide by zero
            divzero();
            break;
        case 2:     // F2 -nmi
            softnmi();
            break;
        case 3:     // F3 - debug break
            dbgbrk();
            break;
        case 4:     // F4 - panic()
            panic("you fucked up!!");
            break;
        case 5:     // F5 - assert()
            assert(true == false);
            break;
        case 6:     // F6 - invalid interrupt
            __asm__ volatile ("int $69");
            break;
        case 7:     // F7 - unexpected device interrupt vector
            __asm__ volatile ("int $0x2D");
            break;
        case 8: {   // F8 - nullptr read
            volatile uint32_t *badptr = NULL;
            const int bad  = *badptr;
            (void) bad;
            break;
        }
        case 9: {   // F9 - nullptr write
            volatile uint32_t *badptr = (uint32_t *) 0xCA55E77E;
            *badptr = 0xBADC0DE;
            break;
        }
        case 12: {  // F12 - triple fault
            struct pseudo_desc idt_desc = { .limit = 0, .base = 0 };
            __lidt(idt_desc);
        }
    }

    g_test_crash_kernel = 0;
}
#endif
