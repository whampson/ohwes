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
 *         File: init/init.c
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <fs.h>
#include <ctype.h>
#include <syscall.h>

#if TEST_BUILD
void tmain_ring3(void);
#endif

extern void rtc_test(void);     // TODO: make exe

void init(void)
{
    assert(getpl() == USER_PL);

    printf("\e4\e[5;33mHello, world!\e[m\n");

#if TEST_BUILD
    printf("running user mode tests...\n");
    tmain_ring3();
#endif

    // TODO: beep and sleep need syscalls
    // beep(1000, 100);
    // sleep(100);
    // beep(1250, 100); // requires kernel for cli
    // TODO: make a test beep program!

    // rtc_test();

    char c;
    int count = 0;

    while (true) {
        c = 0;
        count = read(stdin_fd, &c, 1);
        if (count == 0) {
            continue;   // TODO: blocking I/O for console
        }

        assert(count == 1);
        if (iscntrl(c)) {
            printf("^%c", 0x40 ^ c);
        }
        else {
            printf("%c", c);
        }

        if (c == 3) {   // CTRL+C
            exit(1);
        }
    }
}
