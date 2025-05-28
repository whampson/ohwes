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
 *         File: src/kernel/mm/mm.c
 *      Created: July 3, 2024
 *       Author: Wes Hampson
 *
 * Physical Page Allocator, Buddy System:
 * https://www.kernel.org/doc/gorman/html/understand/understand009.html
 * =============================================================================
 */

#include <assert.h>
#include <errno.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/paging.h>
#include <kernel/config.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

static void init_bss(void);

extern void init_pools(void);   // see pool.c

struct free_area {
    struct list_node free_list;
    void *bitmap;   // buddy list pair state
};

#define MAX_ORDER   11
struct zone {
    uintptr_t zone_base;
    struct free_area free_area[MAX_ORDER];
    pool_t free_list_pool;
};

#define BASELIMIT(base, size)   (base), ((base)+(size)-1)

void init_mm(void)
{
    init_bss();
    init_pools();
}

static void init_bss(void)
{
    // zero the BSS region
    zeromem(&_bss_start, (size_t) &_bss_size);
}
