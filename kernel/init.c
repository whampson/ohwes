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
 *         File: kernel/init.c
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#define __USER_MODE__ 1

#include <ohwes.h>
#include <ctype.h>
#include <console.h>
#include <syscall.h>
#include <errno.h>
#include <fs.h>
#include <debug.h>
#include <io.h>
#include <rtc.h>

#if TEST_BUILD
void tmain_ring3(void);
#endif

// syscall user-mode function definitions, TODO: move these elsewhere
SYSCALL_FN1_VOID(exit, int, status)
SYSCALL_FN3(int, read, int, fd, char*, buf, size_t, count)
SYSCALL_FN3(int, write, int, fd, const char *, buf, size_t, count)
SYSCALL_FN2(int, open, const char*, name, int, flags)
SYSCALL_FN1(int, close, int, fd)
SYSCALL_FN3(int, ioctl, int, fd, unsigned int, cmd, void *, arg)

void init(void)
{
    assert(getpl() == USER_PL);

    // TODO: eventually this should load a program called 'init'
    // that forks itself and spawns the shell program
    // (it we're following the Unix model)

    printf("\e4\e[5;33mHello, world!\e[m\n");

#if TEST_BUILD
    printf("boot: running ring3 tests...\n");
    tmain_ring3();
#endif

    // beep(1000, 100);
    // sleep(100);
    // beep(1250, 100); // requires kernel for cli


    errno = 0;
    int fd = open("/dev/rtc", 0);
    assert(fd > 0);
    int rate = ioctl(fd, IOCTL_RTC_GETRATE, NULL);
    printf("rtc freq = %d\n", rtc_rate2hz(rate));
    assert(rate == RTC_RATE_8192Hz);

    rate = RTC_RATE_2Hz;
    int ret = ioctl(fd, IOCTL_RTC_SETRATE, &rate);
    assert(ret == 0);
    rate = ioctl(fd, IOCTL_RTC_GETRATE, NULL);
    printf("rtc freq = %d\n", rtc_rate2hz(rate));
    ret = close(fd);
    assert(ret == 0);

    char c;
    int count;
    int csinum = 0;
    char csiterm = 0;
    int state = 0;
    enum {
        S_NORM,
        S_ESC,
        S_CSI,
    };

    while (true) {
        c = 0;
        count = read(stdin_fd, &c, 1);
        if (count == 0) {
            continue;   // TODO: blocking I/O
        }

        assert(count == 1);
        if (iscntrl(c)) {
            printf("^%c", 0x40 ^ c);
        }
        else {
            printf("%c", c);
        }

        // kinda nuts...
        switch (c) {
            case '\e':
                state = S_ESC;
                break;
            case '[':
                if (state == S_ESC) {
                    state = S_CSI;
                    csinum = 0;
                    csiterm = '\0';
                }
                break;
            default:
                if (state == S_CSI && !csiterm) {
                    if (isdigit(c)) {
                        csinum *= 10;
                        csinum += (c - '0');
                    }
                    else {
                        csiterm = c;
                    }
                }
                else {
                    state = S_NORM;
                }
        }

        if (state == S_CSI && csiterm == '~') {
            switch (csinum) {
                case 11:        // F1
                    divzero();
                    break;
                case 12:        // F2
                    __asm__ volatile ("int $2");    // NMI
                    break;
                case 13:        // F3
                    dbgbrk();
                    break;
                case 14:        // F4
                    assert(true == false);
                    break;
                case 15:        // F5
                    testint();
                    break;
            }
        }

        if (c == 3) {   // CTRL+C
            exit(1);
        }
    }
}
