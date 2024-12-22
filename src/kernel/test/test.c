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
 *         File: src/kernel/test/test.c
 *      Created: December 22, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <test.h>
#include <kernel/kernel.h>

extern void test_bsf(void);
extern void test_list(void);
extern void test_pool(void);
extern void test_printf(void);
extern void test_ring(void);
extern void test_string(void);

void run_tests(void)
{
    tprint(_YLW("running tests...\n"));

    test_string();
    test_printf();
    test_bsf();
    test_ring();
    test_list();
    test_pool();

    tprint(_GRN("all tests passed!\n"));
}
