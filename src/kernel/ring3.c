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
 *         File: kernel/ring3.c
 *      Created: January 26, 2024
 *       Author: Wes Hampson
 *
 * Ring 3 antics.
 * =============================================================================
 */

#include <ohwes.h>

int main(void)
{
    printf("Got to ring3!\n");
    // divzero();

    return 8675309;
}

__noreturn
void ring3_main(void)
{
    int status = main();
    store_eax(status);

    _syscall1(SYS_EXIT, status);
    die();
}

__noreturn
void go_to_ring3(void)
{
    struct eflags eflags;
    cli_save(eflags);

    eflags.intf = 1;        // enable interrupts
    eflags.iopl = USER_PL;  // printf requires outb

    uint32_t *const ebp = (uint32_t * const) 0xC000;    // user stack
    uint32_t *esp = ebp;

    struct iregs regs = {};
    regs.cs = USER_CS;
    regs.ds = USER_DS;
    regs.es = USER_DS;
    regs.ss = USER_SS;
    regs.ebp = (uint32_t) ebp;
    regs.esp = (uint32_t) esp;
    regs.eip = (uint32_t) ring3_main;
    regs.eflags = eflags._value;
    switch_context(&regs);
    die();
}

