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
#include <queue.h>

#if TEST_BUILD
void tmain_ring3(void);
#endif

extern void rtc_test(void);     // TODO: make exe

static void basic_shell(void);
static size_t /*
    it is now my duty to completely
*/  drain_queue(struct char_queue *q, char *buf, size_t bufsiz);

//
// Runs in ring 3.
//

int main(void)
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

    basic_shell();
    return 0;
}

static void basic_shell(void)
{
#define INPUT_LEN 128

    char _lineq_buf[INPUT_LEN];   // TOOD: NEED AN ALLOCATOR
    struct char_queue _lineq;
    struct char_queue *lineq = &_lineq;

    char line[INPUT_LEN];

    char c;
    int count;
    const char *prompt = "&";

    q_init(lineq, _lineq_buf, sizeof(_lineq_buf));

    while (true) {
        // read a line
        printf(prompt);
        line[0] = '\0';

    read_char:
        // read a character
        count = read(stdin_fd, &c, 1);
        if (count == 0) {
            panic("read returned 0!");
        }
        assert(count == 1);

        //
        // TODO: all this line processing stuff needs to go in the terminal line discipline
        //

        // handle special characters and translations
        switch (c) {
            case '\b':      // ECHOE
                break;
            case '\r':
                c = '\n';   // ICRNL
                break;
            case 3:
                exit(1);    // CTRL+C
                break;
            case 4:
                exit(0);    // CTRL+D
                break;
        }

        bool full = (q_count(lineq) == q_length(lineq) - 1);    // allow one space for newline

        // put translated character into queue
        if (c == '\b') {
            if (q_empty(lineq)) {
                printf("\a");       // beep!
                goto read_char;
            }
            c = q_erase(lineq);
            if (iscntrl(c)) {
                printf("\b");
            }
            printf("\b");
            goto read_char;
        }
        else if (c == '\n' || !full) {
            q_put(lineq, c);
        }
        else if (full) {
            printf("\a");       // beep!
            goto read_char;
        }

        assert(!q_full(lineq));

        // echo char
        if (iscntrl(c) && c != '\t' && c != '\n') {
            printf("^%c", 0x40 ^ c);        // ECHOCTL
        }
        else {
            printf("%c", c);                // ECHO
        }

        // next char if not newline
        if (c != '\n') {
            goto read_char;
        }

        // get and print entire line
        size_t count = drain_queue(lineq, line, sizeof(line));
        if (strncmp(line, "\n", count) != 0) {
            printf("%.*s", count, line);
        }

        //
        // process command line
        //
        if (strncmp(line, "cls\n", count) == 0) {
            printf("\033[2J");
        }
        if (strncmp(line, "exit\n", count) == 0) {
            exit(0);
        }
    }
}

static size_t drain_queue(struct char_queue *q, char *buf, size_t bufsiz)
{
    size_t count = 0;
    while (!q_empty(q) && bufsiz--) {
        *buf++ = q_get(q);
        count++;
    }

    if (bufsiz > 0) {
        *buf = '\0';
    }
    return count;
}
