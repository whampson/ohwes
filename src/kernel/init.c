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

#define INIT_STACK          0xC000
int _errno; // TODO: this needs to point to errno in the current task structure

_syscall1_void(exit, int, status)
_syscall3(int, read, int, fd, char *, buf, size_t, count)
_syscall3(int, write, int, fd, const char *, buf, size_t, count)

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
    assert(read(2, buf, 1) == -1 && errno == EBADF);
    errno = 0;

    char c;

    while (true) {
        int count;
        while ((count = read(stdin_fd, &c, 1)) > 0) {
            if (count) {
                assert(count == 1);
                if (iscntrl(c)) {
                    printf("^%c", 0x40 ^ c);
                }
                else {
                    printf("%c", c);
                }
            }
        }
    }

    kprint("going idle...\n");
    idle();
}

int init_sys(void)
{
    assert(getpl() == KERNEL_PL);

    // tweak flags
    struct eflags eflags;
    cli_save(eflags);
    eflags.intf = 1;                // enable interrupts

    // setup stack
    void *ebp = (void *) INIT_STACK;
    void *esp = ebp;

    // ring 3 initial register context
    struct iregs regs = {};
    regs.cs = USER_CS;
    regs.ds = USER_DS;
    regs.es = USER_DS;
    regs.ss = USER_SS;
    regs.ebp = (uint32_t) ebp;
    regs.esp = (uint32_t) esp;
    regs.eip = (uint32_t) init;
    regs.eflags = eflags._value;

    // zero globals
    _errno = 0;

    // drop to ring 3 and call init
    switch_context(&regs);
    return 0;   // does not actually return
}
