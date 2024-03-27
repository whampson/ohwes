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

#include <ohwes.h>
#include <ctype.h>
#include <console.h>
#include <syscall.h>
#include <errno.h>
#include <fs.h>
#include <debug.h>

SYSCALL_FN1_VOID(exit, int, status)
SYSCALL_FN3(int, read, int, fd, char*, buf, size_t, count)
SYSCALL_FN3(int, write, int, fd, const char *, buf, size_t, count)
SYSCALL_FN2(int, open, const char*, name, int, flags)
SYSCALL_FN1(int, close, int, fd)
SYSCALL_FN3(int, ioctl, int, fd, unsigned int, cmd, unsigned long, arg)

void init(void)
{
    assert(getpl() == USER_PL);

    // TODO: eventually this should load a program called 'init'
    // that forks itself and spawns the shell program
    // (it we're following the Unix model)

    printf("\e4\e[5;33mHello, world!\e[m\n");

    // beep(1000, 100);
    // sleep(100);
    // beep(1250, 100); // requires kernel for cli

    char buf[8];
    assert(read(stdin_fd, NULL, 0) == 0);
    assert(read(stdin_fd, NULL, 1) == -1 && errno == EINVAL);
    assert(read(stdout_fd, buf, 1) == -1 && errno == ENOSYS);
    assert(read(2, buf, 1) == -1 && errno == EBADF);
    assert(write(stdout_fd, NULL, 0) == 0);
    assert(write(stdout_fd, NULL, 1) == -1 && errno == EINVAL);
    assert(write(stdin_fd, buf, 1) == -1 && errno == ENOSYS);
    assert(write(2, buf, 1) == -1 && errno == EBADF);
    assert(open("dummy", 0) == -1 && errno == ENOSYS);
    assert(close(2) == -1 && errno == ENOSYS);
    assert(ioctl(2, 0, 0) == -1 && errno == ENOSYS);
    errno = 0;

    int retval;
    __asm__ volatile ("int $0x80" : "=a"(retval) : "a"(69));
    assert(retval == -ENOSYS);

    char c;
    int count;
    while (true) {
        count = read(stdin_fd, &c, 1);
        if (count) {
            assert(count == 1);
            if (iscntrl(c)) {
                printf("^%c", 0x40 ^ c);
            }
            else {
                printf("%c", c);
            }
        }
        if (c == 3) {   // CTRL+C
            exit(1);
        }
    }
}
