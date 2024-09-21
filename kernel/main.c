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

extern void init_tty(void);
extern void init_fs(void);  // sys/open.c

extern void init_chdev(void);
extern void init_serial(void);

#ifdef TEST_BUILD
typedef int (*test_main)(void);
extern void tmain(void);
#endif

int init(void);     // usermode entry point

static void basic_shell(void);
static size_t /*
    it is now my duty to completely
*/  drain_queue(struct ring *q, char *buf, size_t bufsiz);

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
    extern idt_thunk exception_thunks[NR_EXCEPTIONS];
    extern idt_thunk irq_thunks[NR_IRQS];
    extern idt_thunk _syscall;

    struct x86_desc *idt;
    volatile struct table_desc idt_desc = { };

    __sidt(idt_desc);
    idt = (struct x86_desc *) idt_desc.base;

    for (int i = 0; i < NR_EXCEPTIONS; i++) {
        int vec = VEC_INTEL + i;
        make_trap_gate(&idt[vec], KERNEL_CS, KERNEL_PL, exception_thunks[i]);
    }

    for (int i = 0; i < NR_IRQS; i++) {
        int vec = VEC_DEVICEIRQ + i;
        make_intr_gate(&idt[vec], KERNEL_CS, KERNEL_PL, irq_thunks[i]);
    }

    make_trap_gate(&idt[VEC_SYSCALL], KERNEL_CS, USER_PL, &_syscall);
}

struct boot_info _boot;
struct boot_info *g_boot = &_boot;

extern __syscall int sys_read(int fd, void *buf, size_t count);
extern __syscall int sys_write(int fd, const void *buf, size_t count);
extern __syscall int sys_open(const char *name, int flags);
extern __syscall int sys_close(int fd);

static void test_tty(const char *dev, const char *msg);

__fastcall void start_kernel(const struct boot_info *info)
{
    // copy the boot info into kernel memory so we don't accidentally overwrite
    // it when setting up the rest of the system
    memcpy(g_boot, info, sizeof(struct boot_info));
    info = g_boot;  // reassign pointer for convenience ;)

    // initialize interrupts and timers
    init_idt();
    init_pic();
    init_irq();
    init_timer();
    init_rtc();

    // get the console working
    init_tty();
    init_console(info);
    init_serial();
    // safe to print now using kprint, keyboard also works

    print_info(info);

    // ensure the GDT wasn't mucked with
    verify_gdt();
    // TODO: basic tests

    init_fs();
    init_tasks();

#ifdef DEBUG
    irq_register(IRQ_TIMER, debug_interrupt);   // CTRL+ALT+FN to crash kernel
#endif

    // kprint("enabling interrupts...\n");
    // __sti();

    // // screw around

    // const char *devs[] = {
    //     "/dev/tty1",    // console1 (ALT+F1)
    //     "/dev/ttyS0",   // COM1
    //     "/dev/ttyS1",   // COM2
    //     "/dev/ttyS2",   // COM3
    //     "/dev/ttyS3",   // COM4
    // };

    // // serial port(s)
    // for (int i = 0; i < countof(devs); i++) {
    //     char msg[64];
    //     snprintf(msg, sizeof(msg), "OH-WES says hello on %s!\n", devs[i]);
    //     test_tty(devs[i], msg);
    // }

    enter_ring3((uint32_t) init, __phys_to_virt(KERNEL_INIT_STACK));

    kprint("\n\n\e5\e[1;5;31msystem halted.\e[0m");
    for (;;);
}

static void test_tty(const char *dev, const char *msg)
{
    kprint(">> TTY TEST %s\n", dev);

    int fd = sys_open(dev, 0);
    if (fd < 0) {
        kprint("    open(%s): failed with %d\n", dev, fd);
    }
    else {
        kprint("    open(%s): returned %d\n", dev, fd);
    }

    kprint("    Writing TTY: '%s'...\n", msg);
    ssize_t count = sys_write(fd, msg, strlen(msg));
    if (count < 0) {
        kprint("    write(%s): failed with %d\n", dev, count);
    }
    else {
        kprint("    write(%s): returned %d\n", dev, count);
    }

    int ret = sys_close(fd);
    kprint("    close(%s): returned %d\n", dev, ret);
}

int init(void)
{
    //
    // Runs in ring 3.
    //
    assert(getpl() == USER_PL);

    // printf("\e4\e[5;33mHello, world!\e[m\n");

    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    (void) c;

    // exit(0);


    for(;;);
    return 0;
}

static void enter_ring3(uint32_t entry, uint32_t stack)
{
    assert(getpl() == KERNEL_PL);

    // tweak flags
    struct eflags eflags;
    cli_save(eflags);
    eflags.intf = 0;        // enable interrupts

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
