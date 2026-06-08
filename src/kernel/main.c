/* =============================================================================
* Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <i386/bitops.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <i386/pic.h>
#include <i386/ps2.h>
#include <i386/syscall.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/io.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/pool.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/termios.h>
#include <sys/ioctl.h>

#include <ring.h>

extern __init void init_mm(void);
extern __init void init_io(void);
extern __init void init_fs(void);
extern __init void init_tty(void);

static __init void print_boot_info(void);
static __init void print_cpu_info(void);
static __init void print_kernel_image_sections(void);
static __init void print_page_mappings(void);

static __init __noreturn void go_to_ring3(void *entry, void *stack);

#if TEST_BUILD
extern void run_tests(void);
#endif

void init(void);

// init globals
__init struct boot_info *g_boot;

__fastcall __init __noreturn void kmain(struct boot_info **info)
{
    g_boot = *info; // copy boot info pointer kernel memory
                    // TODO: validate that this ptr isn't bogus

#if PRINT_LOGO
    pr_info(ANSI_BOLD "  ____  __ __   _      __________");
    pr_info(ANSI_BOLD " / __ \\/ // /__| | /| / / __/ __/");
    pr_info(ANSI_BOLD "/ /_/ / _  /___/ |/ |/ / _/_\\ \\  ");
    pr_info(ANSI_BOLD "\\____/_//_/    |__/|__/___/___/  ");
#endif
    pr_info(ANSI_BOLD "%s %s (gcc %s) %s %s\n", OS_NAME, OS_VERSION, __VERSION__, __DATE__, __TIME__);
    pr_info(ANSI_BOLD "%s\n", OS_COPYRIGHT);  // printing will lazy-initialize terminal
    print_boot_info();
    print_cpu_info();

    init_mm();
    init_io();
    init_fs();
    init_tty();

    panic("Ah shit, here we go again!");

// #if TEST_BUILD
//     run_tests(); // TODO lol
// #endif

    pr_info("switching to ring 3...\n");
    go_to_ring3(init, __ustack_end);    // TODO: declare user stack in high memory
    for (;;);

    // for future reference...
    // https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779
}

static __init __noreturn void go_to_ring3(void *entry, void *stack)
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
    regs.ebp = (uint32_t) stack;
    regs.esp = (uint32_t) stack;
    regs.eip = (uint32_t) entry;
    regs.eflags = eflags._value;

    // drop to ring 3
    switch_context(&regs);
    for (;;);
}

static __init void print_boot_info(void)
{
    #define FMT_PL(x,a)     x, PLURALIZE(x, a)
    #define FMT_PL2(x,a,b)  x, PLURALIZE2(x, a, b)

    pr_info("bios-boot: %d %s, %d serial %s, %d parallel %s\n",
        FMT_PL2(g_boot->hwflags.has_diskette_drive + g_boot->hwflags.num_other_diskette_drives, "floppy", "floppies"),
        FMT_PL(g_boot->hwflags.num_serial_ports, "port"),
        FMT_PL(g_boot->hwflags.num_parallel_ports, "port"));
    pr_info("bios-boot: A20 mode is %s\n",
        (g_boot->a20_method == A20_KEYBOARD) ? "A20_KEYBOARD" :
        (g_boot->a20_method == A20_PORT92) ? "A20_PORT92" :
        (g_boot->a20_method == A20_BIOS) ? "A20_BIOS" : "A20_NONE");
    pr_info("bios-boot: %s PS/2 mouse, %s game port\n",
        A_OR_B(g_boot->hwflags.has_ps2mouse, "has", "no"),
        A_OR_B(g_boot->hwflags.has_gameport, "has", "no"));
    pr_info("bios-boot: video mode is %02lXh\n",
        g_boot->vga_mode & 0x7F);
    if (g_boot->ebda_base) {
        pr_info("bios-boot: EBDA=%08lX,%lXh\n",
            g_boot->ebda_base, EBDA_TOP - g_boot->ebda_base);
    }

    #undef FMT_PL
    #undef FMT_PL2
}

static __init void print_cpu_info(void)
{
    struct cpuid cpuid;
    get_cpu_info(&cpuid);

    #define YN(cond)    A_OR_B(cond, "yes","no")

    pr_info("%s: family=%02Xh model=%02Xh stepping=%02Xh type=%02Xh\n",
        cpuid.vendor_id, cpuid.family, cpuid.model, cpuid.stepping, cpuid.type);
    pr_info("%s\n", cpuid.brand_name);
    pr_info("  on-chip FPU? %s\n", YN(cpuid.fpu_support));
    pr_info("  large pages? %s\n", YN(cpuid.pse_support));
    pr_info(" global pages? %s\n", YN(cpuid.pge_support));
    // pr_info("  PAT support? %s\n", YN(cpuid.pat_support));
    pr_info("  TSC support? %s\n", YN(cpuid.tsc_support));
    pr_info("  MSR support? %s\n", YN(cpuid.msr_support));

    #undef YN
}

static __init void print_kernel_image_sections(void)
{
    struct section {
        const char *name;
        void *start, *end;
    };

    // TODO: pack kernel.elf header into image and extract info from there

    // TODO: make this into a sorted list; collect regions at boot
    struct section sections[] = {
        // { "kernel image:",  __kernel_start,     __kernel_end },
        { ".setup",         __setup_start,      __setup_end },
        { ".text",          __text_start,       __text_end },
        { ".rodata",        __rodata_start,     __rodata_end },
        { ".data",          __data_start,       __data_end },
        { ".bss",           __bss_start,        __bss_end },
        { ".idt",           __idt_start,        __idt_end },
        { ".pgdir",         __pgdir_start,      __pgdir_end },
        { ".pgtbl",         __pgtbl_start,      __pgtbl_end },
        { ".klog",          __klog_start,       __klog_end },
        { ".kstack",        __kstack_start,     __kstack_end },
        { ".ustack",        __ustack_start,     __ustack_end },
        { ".estack",        __estack_start,     __estack_end },
    };

    pr_info("kernel image mappings:\n");
    for (int i = 0; i < countof(sections); i++) {
        struct section *sec = &sections[i];
        size_t sec_size = (sec->end - sec->start);
        pr_info("  [%p-%p] %6lu %s\n",
            _P(KERNEL_ADDR(sec->start)), _P(KERNEL_ADDR(sec->end)-1),
            sec_size, sec->name);
    }

    pr_info("kernel occupies %ldk (%ld pages) of static memory\n",
        align(__kernel_size, KB) >> KB_SHIFT,
        PAGE_ALIGN(__kernel_size) >> PAGE_SHIFT);
}

static __init void print_page_info(uint32_t va, const struct pginfo *page)
{
    uint32_t pa = page->pfn << PAGE_SHIFT;
    uint32_t plimit = pa + PAGE_SIZE - 1;
    uint32_t vlimit = va + PAGE_SIZE - 1;
    if (page->pde) {
        if (page->ps) {
            plimit = pa + PGDIR_SIZE - 1;
        }
        vlimit = va + PGDIR_SIZE - 1;
    }

    // v(va-vlimit) -> p(pa-plimit) k/M/T rw u/s a/d g wt nc
    pr_debug("v(%08lX-%08lX) -> p(%08lX-%08lX) %c %-2s %c %c %c %s%s\n",
        va, vlimit, pa, plimit,
        page->pde ? (page->ps ? 'M' : 'T') : 'k',   // (k) small page, (M) large page, (T) page table
        page->rw ? "rw" : "r",                      // read/write
        page->us ? 'u' : 's',                       // user/supervisor
        page->a ? (page->d ? 'd' : 'a') : ' ',      // accessed/dirty
        page->g ? 'g' : ' ',                        // global
        page->pwt ? "wt " : "  ",                   // write-through
        page->pcd ? "nc " : "  ");                  // no-cache
}

static __init void print_page_mappings(void)
{
    struct pginfo *pgdir = (struct pginfo *) get_pgdir();
    struct pginfo *pgtbl;
    struct pginfo *page;
    uint32_t va;

    for (int i = 0; i < PDE_COUNT; i++) {
        page = &pgdir[i];
        if (!page->p) {
            continue;
        }

        va = i << PGDIR_SHIFT;
        print_page_info(va, page);

        if (page->pde && page->ps) {
            continue;   // large
        }

        pgtbl = KERNEL_ADDR(page->pfn << PAGE_SHIFT);
        for (int j = 0; j < PTE_COUNT; j++) {
            page = &pgtbl[j];
            if (!page->p) {
                continue;
            }
            va = (i << PGDIR_SHIFT) | (j << PAGE_SHIFT);
            print_page_info(va, page);
        }
    }
}


// ----------------------------------------------------------------------------
// ----------------------------- Ring 3 ---------------------------------------
// ----------------------------------------------------------------------------

// Return if False/Failed
#define RIF(sys)                \
({                              \
    int __ret = (sys);          \
    if (__ret < 0) {            \
        int __errno = errno;    \
        perror(#sys);           \
        _exit(__errno);         \
    }                           \
    __ret;                      \
})


void init(void)
{
    // TODO: this should be /bin/init
    assert(getpl() == USER_PL);

    RIF(open("/dev/tty1", O_RDWR));   // stdin
    RIF(dup(0));                      // stdout
    RIF(dup(0));                      // stderr

    // TODO: exec("/bin/sh")
    // _exit(main());

    // dummy libc fns
    int main(void);
    extern int shell(void);

#ifdef TEST_LIBC
    extern int test_libc(void);
    test_libc();
#endif

    int ret = shell();
    (void) close(STDERR_FILENO);
    (void) close(STDOUT_FILENO);
    (void) close(STDIN_FILENO);
    _exit(ret);
}

int main(void)
{
    //
    // Runs in ring 3.
    //
    assert(getpl() == USER_PL);
    printf("\e[5;33mHello from user mode!\e[m\n");

    // open TTY serial port
    printf("Opening /dev/ttyS2...\n");
    int fd = RIF(open("/dev/ttyS2", O_RDWR | O_NONBLOCK));

    // set serial TTY termios flags
    //  disable local echo, enable flow control
    struct termios serial_tio;
    ioctl(fd, TCGETS, &serial_tio);
    serial_tio.c_iflag |= (ICRNL | IXON | IXOFF);
    serial_tio.c_oflag |= (OPOST | ONLCR);
    serial_tio.c_cflag |= (CRTSCTS);
    serial_tio.c_lflag &= ~(ECHO | ECHOCTL);
    ioctl(fd, TCSETS, &serial_tio);

    // set stdin termios flags to disable local echo
    struct termios stdin_tio, orig_tio;
    ioctl(STDIN_FILENO, TCGETS, &orig_tio);
    stdin_tio = orig_tio;
    stdin_tio.c_lflag &= ~(ECHO | ECHOCTL);
    ioctl(STDIN_FILENO, TCSETS, &stdin_tio);

    // set stdin to nonblocking
    int orig_cntl;
    orig_cntl = fcntl(STDIN_FILENO, F_GETFL, NULL);
    fcntl(STDIN_FILENO, F_SETFL, orig_cntl | O_NONBLOCK);

    ssize_t ret0, ret1;
    char c0, c1;

    // // TEST: add char to TTY input buffer
    // c0 = 'X';
    // ioctl(fd, TIOCSTI, &c0);
    // c0 = 3;
    // ioctl(fd, TIOCSTI, &c0);

    printf("Waiting for serial input... press CTRL+C to end.\n");
    do {
        // read serial TTY, nonblocking
        ret0 = read(fd, &c0, 1);
        if (ret0 < 0 && errno != EAGAIN) {
            perror("read(TTY)");
            break;
        }

        // read stdin, nonblocking
        ret1 = read(STDIN_FILENO, &c1, 1);
        if (ret1 < 0 && errno != EAGAIN) {
            perror("read(stdin)");
            break;
        }

        // write received chars from serial TTY to stdout
        if (ret0 > 0) {
            write(STDOUT_FILENO, &c0, 1);
        }

        // write received chars from stdin to serial TTY
        if (ret1 > 0) {
            write(fd, &c1, 1);
        }
    } while (c0 != 3 && c1 != 3);   // quit if CTRL+C pressed on either end

    ioctl(STDIN_FILENO, TCSETS, &orig_tio);     // restore stdin termios
    fcntl(STDIN_FILENO, F_SETFL, orig_cntl);    // restore stdin flags

    // show modem status
    int modem;
    ioctl(fd, TIOCMGET, &modem);
    printf("modem=%Xh\n", modem);
    if (modem & TIOCM_DTR) {
        puts("  TIOCM_DTR is set");
    }
    else {
        puts("  TIOCM_DTR is not set");
    }

    // show some stats
    struct serial_stats stats;
    ioctl(fd, TIOCGICOUNT, &stats);
    printf("serial stats:\n");
    printf("  tx:%ld rx:%ld xc:%ld or:%ld pr:%ld fr:%ld tm:%ld bk:%ld\n",
        stats.n_tx, stats.n_rx, stats.n_xchar, stats.n_overrun,
        stats.n_parity, stats.n_framing, stats.n_timeout, stats.n_break);
    printf("  cts:%ld dsr:%ld ri:%ld dcd:%ld\n",
        stats.n_cts, stats.n_dsr, stats.n_ring, stats.n_dcd);

    // close 'er out -- TODO: need to make this actually work
    close(fd);
    return 0;
}
