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
#include <kernel/config.h>
#include <kernel/debug.h>
#include <kernel/io.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/termios.h>
#include <sys/ioctl.h>

#define CHECK(sys)              \
({                              \
    int __ret = (sys);          \
    if (__ret < 0) {            \
        int __errno = errno;    \
        perror(#sys);           \
        _exit(__errno);         \
    }                           \
    __ret;                      \
})

void init_boot(struct boot_info **info);
extern void init_cpu(void);
extern void init_fs(void);
extern void init_io(void);
extern void init_mm(void);
extern void init_pic(void);
extern void init_rtc(void);
extern void init_timer(void);
extern void init_tty(void);

#if SERIAL_DEBUGGING
extern bool g_debug_port_available;
#endif

#if TEST_BUILD
extern void run_tests(void);
#endif

#ifdef DEBUG
extern void crash_key_irq(int irq, struct iregs *regs);
#endif

extern void print_boot_info(void);
extern void print_memory_info(void);
extern void print_kernel_sections(void);
extern void print_page_mappings(void);

static void go_to_ring3(void *entry, uint32_t stack);

void init(void);    // user mode portion of setup
int main(void);     // user mode program entry point

struct boot_info *g_boot;

__fastcall void kmain(struct boot_info **info)
{
    init_boot(info);

    // initialize static memory and setup memory manager,
    // do this as early as possible to ensure BSS is zeroed
    init_mm();

    // setup I/O stuff
    init_io();

    // initialize interrupts and timers
    init_pic();
    init_timer();
    init_rtc();

#if DEBUG && ENABLE_CRASH_KEY       // CTRL+ALT+FN to crash kernel
    irq_register(IRQ_TIMER, crash_key_irq);
#endif

    // setup the file system
    init_fs();

    // get the TTY subsystem working for real
    init_tty();

    kprint(
        "Let's print something large to see whether console printing to "
        "both the VGA display and COM port works properly! Some lorem "
        "ipsum for ya...\n\n");
    kprint(
        "Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque "
        "faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi "
        "pretium tellus duis convallis. Tempus leo eu aenean sed diam urna "
        "tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. "
        "Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut "
        "hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent "
        "per conubia nostra inceptos himenaeos.\n\n");
    kprint(
        "\e[1mDid it work? :p\e[0m\n\n");

    kprint("Press any key to continue... ");
    console_getc();
    kprint("\n");

#if TEST_BUILD
    run_tests();
#endif

    kprint("entering user mode...\n");
    go_to_ring3(init, KERNEL_ADDR(KERNEL_STACK));

    // for future reference...
    // https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779
}

void init_boot(struct boot_info **info)
{
    g_boot = *info;

    kprint("\n\e[0;1m%s %s\n", OS_NAME, OS_VERSION);
    kprint("%s\n", OS_COPYRIGHT);
    kprint("Compiled on %s at %s using GCC %s\e[0m\n\n",
            __DATE__, __TIME__, __VERSION__);

#if SERIAL_DEBUGGING
    if (!g_debug_port_available) {
        kprint("warning: remote debugging not available, "
            "no serial port found on I/O port %Xh!\n", SERIAL_DEBUG_PORT);
    }
#endif

    print_boot_info();
    print_memory_info();
    print_kernel_sections();
#if PRINT_PAGE_MAP
    print_page_mappings();
#endif
}

static void go_to_ring3(void *entry, uint32_t stack)
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
    regs.eip = (uint32_t) entry;
    regs.eflags = eflags._value;

    // drop to ring 3
    __ltr(USER_TSS);
    switch_context(&regs);
}

void init(void)
{
    // TODO: this should run in ring0 as a kernel task,
    // then call execve("/bin/init") or similar to drop to ring3
    // assert(getpl() == KERNEL_PL);

    putchar('a');

    CHECK(open("/dev/tty1", O_RDWR));   // stdin
    CHECK(dup(0));                      // stdout
    CHECK(dup(0));                      // stderr

    _exit(main());
}

int main(void)
{
    //
    // Runs in ring 3.
    //
    assert(getpl() == USER_PL);

    printf("\e[5;33mHello from user mode!\e[m\n");

    // volatile unsigned int *bad = (volatile unsigned int *) 0xbaadc0de;
    // *bad = 0xdeadbeef;

    // open TTY serial port
    printf("Opening /dev/ttyS1...\n");
    int fd = CHECK(open("/dev/ttyS1", O_RDWR | O_NONBLOCK));

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

    // restore stdin termios
    ioctl(STDIN_FILENO, TCSETS, &orig_tio);

    // restore stdin flags
    fcntl(STDIN_FILENO, F_SETFL, orig_cntl);

    int modem;
    ioctl(fd, TIOCMGET, &modem);
    printf("modem=%Xh\n", modem);
    if (modem & TIOCM_DTR) {
        puts("  TIOCM_DTR is set");
    }
    else {
        puts("  TIOCM_DTR is not set");
    }

    struct serial_stats stats;
    ioctl(fd, TIOCGICOUNT, &stats);
    printf("serial stats:\n");
    printf("  tx:%d rx:%d xc:%d or:%d pr:%d fr:%d tm:%d bk:%d\n",
        stats.n_tx, stats.n_rx, stats.n_xchar, stats.n_overrun,
        stats.n_parity, stats.n_framing, stats.n_timeout, stats.n_break);
    printf("  cts:%d dsr:%d ri:%d dcd:%d\n",
        stats.n_cts, stats.n_dsr, stats.n_ring, stats.n_dcd);

    close(fd);
    return 0;
}
