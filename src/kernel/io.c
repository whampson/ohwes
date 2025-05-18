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
 *         File: kernel/io.c
 *      Created: April 5, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <kernel/config.h>
#include <kernel/io.h>
#include <kernel/list.h>
#include <kernel/kernel.h>
#include <kernel/pool.h>
#include <kernel/serial.h>

struct io_range {
    struct list_node chain;
    const char *name;
    uint16_t base;
    uint8_t count;
};

list_t io_ranges_list = LIST_INITIALIZER(io_ranges_list);
struct io_range _io_ranges_pool[MAX_NR_IO_RANGES];
pool_t io_ranges_pool = NULL;

void init_io(void)
{
    io_ranges_pool = create_pool(
        _io_ranges_pool,
        "io_ranges",
        MAX_NR_IO_RANGES,
        sizeof(struct io_range));
    if (!io_ranges_pool) {
        panic("failed to create io_ranges_pool!");
    }

#if SERIAL_DEBUGGING
    if (reserve_io_range(SERIAL_DEBUG_PORT, 8, "serial_debug") < 0) {
        panic("unable to reserve I/O ports for serial debugging!");
    }
#endif
}

int check_io_range(uint16_t base, uint8_t count)
{
    return -EBUSY;  // TODO
}

int reserve_io_range(uint16_t base, uint8_t count, const char *name)
{
    struct io_range *new_range;
    struct io_range *curr;
    struct list_node *e;

    if (!io_ranges_pool) {
        panic("I/O ranges not initialized!");
    }

    new_range = pool_alloc(io_ranges_pool);
    if (!new_range) {
        kprint("warning: I/O range reservation list is full!\n");
        return -ENOMEM;
    }

    new_range->base = base;
    new_range->count = count;
    new_range->name = name;

    if (list_empty(&io_ranges_list)) {
        list_add(&io_ranges_list, &new_range->chain);
        return 0;
    }

    for (list_iterator(&io_ranges_list, e)) {
        curr = list_item(e, struct io_range, chain);

        // list is ordered; if the new range is below the current range,
        // we found a gap and can reserve it
        if (base < curr->base && base + count <= curr->base) {
            list_add(&curr->chain, &new_range->chain);
            return 0;
        }
        else if (base >= curr->base && base < curr->base + curr->count) {
            return -EBUSY;  // overlap
        }
    }

    // we've wrapped to the beginning of the circular list,
    // so add the new item /before/ the head
    list_add(&io_ranges_list, &new_range->chain);
    return 0;
}

#if 0
void print_io_range_list(void)  // TODO: adapt for procfs
{
    struct io_range *curr;
    struct list_node *e;

    for (list_iterator(&io_ranges_list, e)) {
        curr = list_item(e, struct io_range, chain);
        kprint("%04x-%04x: %s\n", curr->base, curr->base+curr->count-1, curr->name);
    }
}
#endif

void release_io_range(uint16_t base, uint8_t count)
{
    struct io_range *curr;
    struct list_node *e;

    for (list_iterator(&io_ranges_list, e)) {
        curr = list_item(e, struct io_range, chain);
        if (curr->base == base && curr->count == count) {
            list_remove(&curr->chain);
            break;
        }
    }
}
