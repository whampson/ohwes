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
#include <ohwes.h>
#include <boot.h>
#include <console.h>
#include <config.h>
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

#define CHATTY 1

extern void init_irq(void);
extern void init_tasks(void);
extern void init_console(const struct boot_info *info);

extern void init_mm(const struct boot_info *info);
extern void init_pic(void);

extern void init_timer(void);
extern void init_rtc(void);

extern void init_serial(void);

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
void debug_interrupt(int irq_num);
#endif

void verify_gdt(void)
{
    volatile struct table_desc gdt_desc = { };
    __sgdt(gdt_desc);

    struct x86_desc *kernel_cs = x86_get_desc(gdt_desc.base, KERNEL_CS);
    panic_assert(kernel_cs->seg.type == DESCTYPE_CODE_XR || kernel_cs->seg.type == DESCTYPE_CODE_XRA);
    panic_assert(kernel_cs->seg.dpl == KERNEL_PL);
    panic_assert(kernel_cs->seg.db == 1);
    panic_assert(kernel_cs->seg.s == 1);
    panic_assert(kernel_cs->seg.p == 1);

    struct x86_desc *kernel_ds = x86_get_desc(gdt_desc.base, KERNEL_DS);
    panic_assert(kernel_ds->seg.type == DESCTYPE_DATA_RW || kernel_ds->seg.type == DESCTYPE_DATA_RWA);
    panic_assert(kernel_ds->seg.dpl == KERNEL_PL);
    panic_assert(kernel_ds->seg.db == 1);
    panic_assert(kernel_ds->seg.s == 1);
    panic_assert(kernel_ds->seg.p == 1);

    struct x86_desc *user_cs = x86_get_desc(gdt_desc.base, USER_CS);
    panic_assert(user_cs->seg.type == DESCTYPE_CODE_XR);
    panic_assert(user_cs->seg.dpl == USER_PL);
    panic_assert(user_cs->seg.db == 1);
    panic_assert(user_cs->seg.s == 1);
    panic_assert(user_cs->seg.p == 1);

    struct x86_desc *user_ds = x86_get_desc(gdt_desc.base, USER_DS);
    panic_assert(user_ds->seg.type == DESCTYPE_DATA_RW);
    panic_assert(user_ds->seg.dpl == USER_PL);
    panic_assert(user_ds->seg.db == 1);
    panic_assert(user_ds->seg.s == 1);
    panic_assert(user_ds->seg.p == 1);
}

void init_idt(void)
{
    extern idt_thunk exception_thunks[NUM_EXCEPTIONS];
    extern idt_thunk irq_thunks[NUM_IRQS];
    extern idt_thunk _syscall;

    struct x86_desc *idt;
    volatile struct table_desc idt_desc = { };

    __sidt(idt_desc);
    idt = (struct x86_desc *) idt_desc.base;

    for (int i = 0; i < NUM_EXCEPTIONS; i++) {
        int vec = VEC_INTEL + i;
        make_trap_gate(&idt[vec], KERNEL_CS, KERNEL_PL, exception_thunks[i]);
    }

    for (int i = 0; i < NUM_IRQS; i++) {
        int vec = VEC_DEVICEIRQ + i;
        make_intr_gate(&idt[vec], KERNEL_CS, KERNEL_PL, irq_thunks[i]);
    }

    make_trap_gate(&idt[VEC_SYSCALL], KERNEL_CS, USER_PL, _syscall);
}

int dummy_read(struct file *file, char *buf, size_t count)
{
    return console_read(kernel_task()->cons, buf, count);
}

int dummy_write(struct file *file, const char *buf, size_t count)
{
    return console_write(kernel_task()->cons, buf, count);
}

struct file_ops console_fops =
{
    .read = dummy_read,
    .write = dummy_write,
    .open = NULL,
    .close = NULL,
    .ioctl = NULL   // TODO: ioctl
};

struct file console_file =
{
    .fops = &console_fops,
    .ioctl_code = _IOC_CONSOLE
};

struct boot_info _boot;
struct boot_info *g_boot = &_boot;

void init_kernel_task(void)
{
    // initialize the kernel task
    // TODO: call open() on console to do this?
    kernel_task()->cons = get_console(1);
    kernel_task()->files[stdin_fd] = &console_file;
    kernel_task()->files[stdout_fd] = &console_file;
}

extern __syscall int sys_read(int fd, void *buf, size_t count);
extern __syscall int sys_write(int fd, const void *buf, size_t count);

__fastcall void start_kernel(const struct boot_info *info)
{
    // copy the boot info onto the stack so we don't accidentally overwrite it
    memcpy(g_boot, info, sizeof(struct boot_info));
    info = g_boot;  // for convenience ;)

    // initialize core system components
    init_idt();
    init_pic();
    init_irq();

    // initialize console
    init_console(info);
    // safe to print now

    print_info(info);

    // ensure GDT wasn't mucked with
    verify_gdt();

    // initialize tasks
    init_tasks();
    init_kernel_task();

    // initialize devices
    init_timer();
    init_rtc();

    init_serial();

#ifdef DEBUG
    // CTRL+ALT+FN to crash kernel
    irq_register(IRQ_TIMER, debug_interrupt);
#endif

    boot_kprint("enabling interrupts...\n");
    __sti();

    char c;
    do {
        sys_read(stdin_fd, &c, 1);
        sys_write(stdout_fd, &c, 1);
    } while (c != 3);   // CTRL+C

    // basic_shell();
    kprint("\n\n\e5\e[1;5;31msystem halted.\e[0m");
    for (;;);

//     memcpy(&g_boot, info, sizeof(struct boot_info));

//     init_cpu(&g_boot);
//     init_pic();
//     init_irq();
//     init_vga();
//     init_tasks();
//     init_console();     // safe to print now

//     kprint("\n" OS_NAME " " OS_VERSION ", build " OS_BUILDDATE "\n");
//     kprint("\n");
// #if CHATTY
//     print_info(&g_boot);
// #endif

//     init_ps2(&g_boot);
//     init_kb();
//     init_memory(&g_boot);
//     init_timer();
//     init_rtc();

// #ifdef DEBUG
//     // CTRL+ALT+FN to crash kernel
//     irq_register(IRQ_TIMER, debug_interrupt);
// #endif

//     kprint("boot: entering ring3...\n");
//     uint32_t user_stack = 0xD000;
//     enter_ring3((uint32_t) init, user_stack);
}

int init(void)
{
    //
    // Runs in ring 3.
    //
    assert(getpl() == USER_PL);

    // TODO: load shell program from disk

    // printf("\e4\e[5;33mHello, world!\e[m\n");
    // basic_shell();

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
    kprint("bios: A20 mode is %s\n",
        (info->a20_method == A20_KEYBOARD) ? "A20_KEYBOARD" :
        (info->a20_method == A20_PORT92) ? "A20_PORT92" :
        (info->a20_method == A20_BIOS) ? "A20_BIOS" :
        "A20_NONE");
    kprint("bios: %s PS/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("bios: video mode is %02Xh\n", info->vga_mode & 0x7F);
    if (info->ebda_base) kprint("boot: EBDA=%08X,%Xh\n", info->ebda_base, ebda_size);
    // kprint("boot: stage2:\t%08X,%Xh\n", info->stage2_base, info->stage2_size);
    // kprint("boot: kernel:\t%08X,%Xh\n", info->kernel_base, info->kernel_size);
}

#ifdef DEBUG
void debug_interrupt(int irq_num)
{
    switch (g_test_crash_kernel) {
        case 1:     // F1 - divide by zero
            __asm__ volatile ("idiv %0" :: "a"(0), "b"(0));
            break;
        case 2:     // F2 - simulate nmi (TODO: real NMI)
            __asm__ volatile ("int $2");
            break;
        case 3:     // F3 - debug break
            __asm__ volatile ("int $3");
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
            struct table_desc idt_desc = { .limit = 0, .base = 0 };
            __lidt(idt_desc);
        }
    }

    g_test_crash_kernel = 0;
}
#endif
