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
 *         File: src/include/kernel/pool.h
 *      Created: November 18, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __POOL2_H
#define __POOL2_H

#include <stddef.h>
#include <stdint.h>
#include <kernel/list.h>

#define POOL2_MAGIC          'lwep'
#define INVALID_POOL2        ((pool2_t *) NULL)

/**
 * Memory pool for allocating items of a fixed size.
 */
struct pool2 {
    uint32_t magic;     // identifier for pool type
    const char *name;   // pool name
    size_t size;        // item size bytes
    size_t capacity;    // max number of item slots
    size_t count;       // number of slots allocated
    list_t list;        // list of pools on system
    list_t free_list;   // free slots
    int order;          // allocation order
    void *alloc;        // item allocation
};
typedef struct pool2 pool2_t;

// pool_t *find_pool(const char *name);

// TODO: remove max capacity
pool2_t * pool2_create(const char *name, size_t size, size_t capacity, int flags);

void pool2_destroy(pool2_t *pool);

void * pool2_alloc(pool2_t *pool, int flags);

void pool2_free(pool2_t *pool, const void *item);

#endif // __POOL2_H
