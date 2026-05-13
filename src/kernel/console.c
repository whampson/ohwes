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
 *         File: kernel/console.c
 *      Created: April 28, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <kernel/kernel.h>
#include <kernel/console.h>
#include <kernel/fs.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/vga.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/queue.h>
#include <sys/ohwes.h>

struct console *g_console_list = NULL;  // linked list
static int num_consoles = 0;

extern void klog_flush_to_console(struct console *cons);    // kprint.c

bool register_console(struct console *cons)
{
    struct console *curr_cons;
    struct console *prev_cons;
    bool success = true;

    // sanity check
    if (!cons || !cons->name || !cons->device || !cons->init ||
        !cons->write || !cons->read_char) {
        return false;
    }

    // add console to tail
    if (num_consoles == 0) {
        assert(g_console_list == NULL);
        g_console_list = cons;
    }
    else {
        assert(g_console_list != NULL);
        curr_cons = &g_console_list[0];
        do {
            if (curr_cons == cons) {
                goto registered;
            }
            prev_cons = curr_cons;
            curr_cons = curr_cons->next;
        } while (curr_cons != NULL);
        prev_cons->next = cons;
    }

    // initialize
    success = cons->init(cons);
    cons->next = NULL;

    if (cons->flags & _CONSOLE_FLAG_PRINTBUF) {
        klog_flush_to_console(cons);
    }

    num_consoles++;

registered:
    return success;
}

bool unregister_console(struct console *cons)
{
    struct console *curr_cons = &g_console_list[0];
    struct console *prev_cons = NULL;

    do {
        if (curr_cons == cons) {
            break;
        }
        prev_cons = curr_cons;
        curr_cons = curr_cons->next;
    } while (curr_cons != NULL);

    if (curr_cons == NULL) {
        return false;   // not found
    }

    assert(num_consoles > 0);

    if (curr_cons == g_console_list) {
        assert(prev_cons == NULL);
        g_console_list = curr_cons->next;
    }
    else {
        assert(prev_cons != NULL);
        prev_cons->next = curr_cons->next;
    }

    curr_cons->next = NULL;
    num_consoles--;

    return true;
}

bool has_console(void)
{
    return num_consoles > 0;
}

#if 0
static void print_log(void)    // TODO: procfs for this
{
    for (int i = 0; i < _log_size; i++) {
        // printf("%c", _kernel_log[(_log_start + i) % _log_size]);
    }
}

static void print_consoles(void)   // TODO: procfs for this
{
    struct console *cons;

    cons = g_console_list;
    while (cons) {
        // printf("%s ", cons->name);
        cons = cons->next;
    }
    // printf("\n");
}
#endif
