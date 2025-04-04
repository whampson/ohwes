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
*         File: src/kernel/init/main.c
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
#include <kernel/console.h>
#include <kernel/config.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>
#include <kernel/serial.h>
#include <kernel/termios.h>
#include <sys/ioctl.h>

extern void init_cpu(void);
extern void init_fs(void);
extern void init_mm(void);
extern void init_pic(void);
extern void init_rtc(void);
extern void init_timer(void);
extern void init_tty(void);
extern void init_vga(void);

#if TEST_BUILD
extern void run_tests(void);
#endif

static void print_boot_info(void);

void init(void);
int main(void);     // usermode program entry point
static void usermode(uint32_t stack);

// static size_t
//  /* it is now my duty to completely */
//     drain_queue(struct ring *q, char *buf, size_t bufsiz);

__data_segment static struct boot_info _boot;
__data_segment struct boot_info *g_boot = &_boot;

__fastcall void start_kernel(struct boot_info *info)
{
    // copy boot info into kernel memory so we don't lose it
    memcpy(g_boot, info, sizeof(struct boot_info));
    info = g_boot;  // reassign local ptr so we don't use overwritten buffer

    // init the early terminal by printing something to it
    kprint("\n\e[0;1m%s %s '%s'\n", OS_NAME, OS_VERSION, OS_MONIKER);
    kprint("built %s %s using GCC %s by %s\e[0m\n",
        __DATE__, __TIME__, __VERSION__, OS_AUTHOR);
    print_boot_info();

    // initialize VGA
    init_vga();

    // finish setting up CPU descriptors
    init_cpu();

    // initialize static memory and setup memory manager,
    // do this as early as possible to ensure BSS is zeroed
    init_mm();

    // initialize interrupts and timers
    init_pic();
    init_timer();
    init_rtc();

#if TEST_BUILD
    run_tests();
#endif

    // setup the file system
    init_fs();

    // get the console and tty subsystem working for real
    init_tty();

    kprint("entering user mode...\n");
    usermode(__phys_to_virt(SETUP_STACK));

    // for future reference...
    // https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779
}

static void usermode(uint32_t stack)
{
    assert(getpl() == KERNEL_PL);

    // tweak flags
    struct eflags eflags;
    cli_save(eflags);
    eflags.intf = 1;        // enable interrupts

    uint32_t entry = (uint32_t) init;

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

#define CHECK(sys)                                              \
({                                                              \
    int __ret = (sys);                                          \
    if (__ret < 0) {                                            \
        int __errno = errno;                                    \
        perror(#sys);                                           \
        _exit(__errno);                                         \
    }                                                           \
    __ret;                                                      \
})

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

    printf("\e4\e[5;33mHello from user mode!\e[m\n");

    // open TTY serial port
    printf("Opening /dev/ttyS0...\n");
    int fd = CHECK(open("/dev/ttyS0", O_RDWR | O_NONBLOCK));

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
        if (ret0 < 0 && ret0 != -EAGAIN) {
            perror("read(TTY): ");
            break;
        }

        // read stdin, nonblocking
        ret1 = read(STDIN_FILENO, &c1, 1);
        if (ret1 <0 && ret1 != -EAGAIN) {
            perror("read(stdin): ");
            break;
        }

        // write received chars from serial TTY to stdout
        if (ret0 != -EAGAIN) {
            write(STDOUT_FILENO, &c0, 1);
        }

        // write received chars from stdin to serial TTY
        if (ret1 != -EAGAIN) {
            write(fd, &c1, 1);
        }
    } while (c0 != 3 && c1 != 3);   // quit if CTRL+C pressed on either end

    // restore stdin termios
    ioctl(STDIN_FILENO, TCSETS, &orig_tio);

    // restore stdin flags
    fcntl(STDIN_FILENO, F_SETFL, orig_cntl);

    int modem;
    ioctl(fd, TIOCMGET, &modem);
    printf("modem=%xh\n", modem);
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

static void print_boot_info(void)
{
    int nfloppies = g_boot->hwflags.has_diskette_drive;
    if (nfloppies) {
        nfloppies += g_boot->hwflags.num_other_diskette_drives;
    }

    int nserial = g_boot->hwflags.num_serial_ports;
    int nparallel = g_boot->hwflags.num_parallel_ports;
    bool gameport = g_boot->hwflags.has_gameport;
    bool mouse = g_boot->hwflags.has_ps2mouse;
    uint32_t ebda_size = 0xA0000 - g_boot->ebda_base;

    kprint("bios: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("bios: A20 mode is %s\n",
        (g_boot->a20_method == A20_KEYBOARD) ? "A20_KEYBOARD" :
        (g_boot->a20_method == A20_PORT92) ? "A20_PORT92" :
        (g_boot->a20_method == A20_BIOS) ? "A20_BIOS" :
        "A20_NONE");
    kprint("bios: %s PS/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("bios: video mode is %02Xh\n", g_boot->vga_mode & 0x7F);
    if (g_boot->ebda_base) kprint("bios: EBDA=%08X,%Xh\n", g_boot->ebda_base, ebda_size);
}
