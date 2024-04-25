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

    // beep(1000, 100);
    // sleep(100);
    // beep(1250, 100); // requires kernel for cli
    // TODO: make a test beep program!

    // rtc_test();

    char c;
    int count = 0;
    // int csinum = 0;
    // char csiterm = 0;
    // int state = 0;
    // enum {
    //     S_NORM,
    //     S_ESC,
    //     S_CSI,
    // };

    // nasty forever-input loop code follows, tread lightly
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

        // // escape sequence decoding...
        // switch (c) {
        //     case '\e':
        //         state = S_ESC;
        //         break;
        //     case '[':
        //         if (state == S_ESC) {
        //             state = S_CSI;
        //             csinum = 0;
        //             csiterm = '\0';
        //         }
        //         break;
        //     default:
        //         if (state == S_CSI && !csiterm) {
        //             if (isdigit(c)) {
        //                 csinum *= 10;
        //                 csinum += (c - '0');
        //             }
        //             else {
        //                 csiterm = c;
        //             }
        //         }
        //         else {
        //             state = S_NORM;
        //         }
        // }

        // if (state == S_CSI && csiterm == '~') {
        //     //
        //     // usermode crashes
        //     //
        //     switch (csinum) {
        //         case 11:        // F1
        //             divzero();
        //             break;
        //         // case 12:        // F2
        //         //     __asm__ volatile ("int $2");    // NMI
        //         //     break;
        //         case 13:        // F3
        //             dbgbrk();
        //             break;
        //         case 14:        // F4
        //             assert(true == false);
        //             break;
        //         case 15:        // F5
        //             testint();
        //             break;
        //         case 17:        // F6
        //             panic("you fucked up in userland!!");
        //             break;
        //         // case 18:        // F7
        //         //     __asm__ volatile ("int $0x2D");
        //         //     break;
        //     }
        // }

        if (c == 3) {   // CTRL+C
            exit(1);
        }
    }
}
