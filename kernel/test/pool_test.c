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
 *         File: kernel/test/pool_test.c
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <list.h>
#include <pool.h>

struct task {
    uintptr_t mem_start, mem_end;
    int pid;
};

#define NUM_TASKS 8
static struct task _task_pool[NUM_TASKS];
static pool_t task_pool;

void test_pool(void)
{
    // struct task *t;

    task_pool = create_pool("task_pool", _task_pool, sizeof(struct task), 2);

    // {
    //     t = pool_alloc(task_pool);
    //     t->pid = 1234;
    //     t->mem_start = 0x10000;
    //     t->mem_end = 0x1FFFF;
    // }
    // {
    //     t = pool_alloc(task_pool);
    //     t->pid = 5678;
    //     t->mem_start = 0x20000;
    //     t->mem_end = 0x2FFFF;
    // }
    // {
    //     t = pool_alloc(task_pool);
    // }

    destroy_pool(task_pool);
}
