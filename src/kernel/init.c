/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdio.h>
#include <string.h>

#include <boot.h>
#include <os.h>
#include <test.h>

extern void con_init(void);
extern void init_cpu(void);

BootInfo g_bootInfo;
BootInfo * const g_pBootInfo = &g_bootInfo;

static void run_tests(void)
{
#if TEST_BUILD
    test_libc();
#endif
}

__fastcall
void kmain(const BootInfo * const pBootInfo)
{
    memcpy(g_pBootInfo, pBootInfo, sizeof(BootInfo));

    con_init();     // get the vga console working first
    init_cpu();     // then finish setting up the CPU.

    run_tests();
}
