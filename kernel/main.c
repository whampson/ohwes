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

extern void init_cpu(const struct boot_info *info);
extern void init_irq(void);
extern void init_tasks(void);
extern void init_console(const struct boot_info *info);

extern void init_mm(const struct boot_info *info);
extern void init_pic(void);

extern void init_timer(void);
extern void init_rtc(void);

extern void init_tty(void);
extern void init_fs(void);  // sys/open.c

extern void init_chdev(void);
extern void init_serial(void);

// #ifdef TEST_BUILD
// typedef int (*test_main)(void);
// extern void tmain(void);
// #endif

extern void test_list(void);    // test/list_test.c

void _start(void);  // usermode runtime entry point
int main(void);     // usermode program entry point

static void usermode(uint32_t stack);

static void basic_shell(void);
static size_t /*
    it is now my duty to completely
*/  drain_queue(struct ring *q, char *buf, size_t bufsiz);

static void print_info(const struct boot_info *info);

#ifdef DEBUG
int g_test_crash_kernel;
void debug_interrupt(int irq_num);
#endif

struct boot_info _boot;
struct boot_info *g_boot = &_boot;

extern __syscall int sys_read(int fd, void *buf, size_t count);
extern __syscall int sys_write(int fd, const void *buf, size_t count);
extern __syscall int sys_open(const char *name, int flags);
extern __syscall int sys_close(int fd);

// linker script symbols -- use & to get assigned value
extern uintptr_t __kernel_text_vma;
extern uintptr_t __kernel_base;

__fastcall void start_kernel(const struct boot_info *info)
{
    // copy boot info into kernel memory so we don't accidentally overwrite it
    memcpy(g_boot, info, sizeof(struct boot_info));
    info = g_boot;  // reassign ptr for convenience ;)

    // finish setting up CPU descriptors
    init_cpu(info);

    // initialize interrupts and timers
    init_pic();
    init_irq();
    init_timer();
    init_rtc();

    // get the console working
    init_tty();
    init_console(info);
    // safe to print now using kprint, keyboard also works

    kprint("boot: kernel space mapped at 0x%x\n", &__kernel_base);
    kprint("boot: kernel .text mapped at 0x%x\n", &__kernel_text_vma);

    // TODO: basic tests

    init_serial();
    init_mm(info);
    init_fs();
    init_tasks();

#ifdef DEBUG
    // CTRL+ALT+FN to crash kernel
    irq_register(IRQ_TIMER, debug_interrupt);
#endif

    kprint("boot: entering user mode...\n");
    usermode(__phys_to_virt(USER_STACK));

    kprint("\n\n\e5\e[1;5;31msystem halted.\e[0m");
    for (;;);
}

static void usermode(uint32_t stack)
{
    assert(getpl() == KERNEL_PL);

    // tweak flags
    struct eflags eflags;
    cli_save(eflags);
    eflags.intf = 1;        // enable interrupts

    uint32_t entry = (uint32_t) _start;

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

#define SYS_CHECK(sys)                                          \
({                                                              \
    int __sysret = (sys);                                       \
    if (__sysret < 0) {                                         \
        panic(#sys ": failed with 0x%X (%d)\n", errno, errno);  \
    }                                                           \
    __sysret;                                                   \
})

void _start(void)
{
    int fd0 = SYS_CHECK(open("/dev/keyboard", 0));      /// TODO: open /dev/console, then call dup() to duplicate file desc
    int fd1 = SYS_CHECK(open("/dev/console", 0));
    int ret = main();
    close(fd1);
    close(fd0);
    exit(ret);
}

int main(void)
{
    //
    // Runs in ring 3.
    //
    assert(getpl() == USER_PL);

    // TODO: load shell program from disk


    printf("\e4\e[5;33mHello, world!\e[m\n");
    // basic_shell();

    return 69;

}

// static void basic_shell(void)
// {
// #define INPUT_LEN 128

//     char _lineq_buf[INPUT_LEN];   // TOOD: NEED AN ALLOCATOR
//     struct ring _lineq;
//     struct ring *lineq = &_lineq;

//     char line[INPUT_LEN];

//     char c;
//     int count;
//     const char *prompt = "&";

//     ring_init(lineq, _lineq_buf, sizeof(_lineq_buf));

//     while (true) {
//         // read a line
//         printf(prompt);
//         line[0] = '\0';

//     read_char:
//         // read a character
//         count = read(stdin_fd, &c, 1);
//         if (count == 0) {
//             panic("read returned 0!");
//         }
//         assert(count == 1);

//         //
//         // TODO: all this line processing stuff needs to go in the terminal line discipline
//         //

//         // handle special characters and translations
//         switch (c) {
//             case '\b':      // ECHOE
//                 break;
//             case '\r':
//                 c = '\n';   // ICRNL
//                 break;
//             case 3:
//                 exit(1);    // CTRL+C
//                 break;
//             case 4:
//                 exit(0);    // CTRL+D
//                 break;
//         }

//         bool full = (ring_count(lineq) == ring_length(lineq) - 1);    // allow one space for newline

//         // put translated character into queue
//         if (c == '\b') {
//             if (ring_empty(lineq)) {
//                 printf("\a");       // beep!
//                 goto read_char;
//             }
//             c = ring_erase(lineq);
//             if (iscntrl(c)) {
//                 printf("\b");
//             }
//             printf("\b");
//             goto read_char;
//         }
//         else if (c == '\n' || !full) {
//             ring_put(lineq, c);
//         }
//         else if (full) {
//             printf("\a");       // beep!
//             goto read_char;
//         }

//         assert(!ring_full(lineq));

//         // echo char
//         if (iscntrl(c) && c != '\t' && c != '\n') {
//             printf("^%c", 0x40 ^ c);        // ECHOCTL
//         }
//         else {
//             printf("%c", c);                // ECHO
//         }

//         // next char if not newline
//         if (c != '\n') {
//             goto read_char;
//         }

//         // get and print entire line
//         size_t count = drain_queue(lineq, line, sizeof(line));
//         if (strncmp(line, "\n", count) != 0) {
//             printf("%.*s", (int) count, line);
//         }

//         //
//         // process command line
//         //
//         if (strncmp(line, "cls\n", count) == 0) {
//             printf("\033[2J");
//         }
//         if (strncmp(line, "exit\n", count) == 0) {
//             exit(0);
//         }
//     }
// }

static size_t drain_queue(struct ring *q, char *buf, size_t bufsiz)
{
    size_t count = 0;
    while (!ring_empty(q) && bufsiz--) {
        *buf++ = ring_get(q);
        count++;
    }

    if (bufsiz > 0) {
        *buf = '\0';
    }
    return count;
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
