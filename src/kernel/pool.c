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
 *         File: kernel/pool.c
 *      Created: September 6, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <kernel/kernel.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

#define CHUNK_MAGIC     'nuhc'

#define pool_valid(p)   ((p) != INVALID_POOL && (p)->magic == POOL_MAGIC)

// memory 'chunk' info for keeping track of pool item
struct chunk {
    uint32_t magic;     // identifier for chunk type
    pool_t *pool;       // pool this item belongs to
    list_t list;        // item chain in pool
    void *data;         // item data
};

// global pool info
struct pool_info {
    int order;          // order of master allocation
    pool_t *alloc;      // pool of pools allocation
    list_t list;        // list of active pools
    list_t free_list;   // list of free pools
    int count;          // number of active pools
};
static struct pool_info _pools;
struct pool_info *g_poolinfo = &_pools;

void lazy_init_pools(void)
{
    size_t num_pools = MAX_NR_POOLS;
    size_t master_pool_size = align(num_pools * sizeof(struct pool), 4);
    g_poolinfo->order = get_order(master_pool_size);

    g_poolinfo->alloc = (struct pool *) alloc_pages(ALLOC_ZERO, g_poolinfo->order);
    if (g_poolinfo->alloc == NULL) {
        panic("not enough memory for pools!\n");
    }

    list_init(&g_poolinfo->list);
    list_init(&g_poolinfo->free_list);

    for (int i = 0; i < num_pools; i++) {
        struct pool *p = &g_poolinfo->alloc[i];
        p->magic = POOL_MAGIC;
        list_add(&g_poolinfo->free_list, &p->list);
    }

    kprint("pool: %d pages used to manage a max of %d pools\n",
        get_order_size(g_poolinfo->order) >> PAGE_SHIFT, MAX_NR_POOLS);
}

#if 0
void destroy_pools()
{
    if (g_poolinfo->pools) {
        free_pages(g_poolinfo->alloc, g_poolinfo->order);
        g_poolinfo->alloc = NULL;

        kprint("pool: destroyed master pool\n");
    }
}
#endif

pool_t * pool_create(const char *name, size_t capacity, size_t size, int flags)
{
    // TODO: flags for alignment, zeroing, etc.
    (void) flags;

    if (name == NULL || capacity <= 0 || size <= 0) {
        return INVALID_POOL;
    }

    // validate name
    int len = 0;
    for (int i = 0; i < POOL_MAX_NAME+1; i++) {
        if (name[i] == '\0') {
            len = i;
            break;
        }
        if (!isalnum(name[i]) && name[i] != '_') {
            return INVALID_POOL;
        }
    }
    if (len < 1 || len > POOL_MAX_NAME) {
        return INVALID_POOL;
    }

    // lazy alloc master pool data
    if (g_poolinfo->alloc == NULL) {
        lazy_init_pools();
    }
    assert(g_poolinfo->alloc != NULL);

    // ensure name is unique
    for (list_iterator(n, &g_poolinfo->list)) {
        struct pool *other = list_item(n, struct pool, list);
        if (strncmp(other->name, name, POOL_MAX_NAME) == 0) {
            return INVALID_POOL;
        }
    }

    // ensure we have space for a new pool
    if (list_empty(&g_poolinfo->free_list)) {
        panic("pool: create: max number of pools allocated!!");
        return INVALID_POOL;
    }

    // initialize pool
    size_t size_bytes = capacity * (size + sizeof(struct chunk));
    int order = get_order(size_bytes);
    if (order < 0) {
        return INVALID_POOL;
    }
    void *alloc = alloc_pages(ALLOC_ZERO, order);
    if (alloc == NULL) {
        warn("pool: create: not enough memory for pool size=%d capacity=%d!\n", size, capacity);
        return INVALID_POOL;
    }

    struct pool *p = list_item(g_poolinfo->free_list.next, struct pool, list);
    if (p->magic != POOL_MAGIC || p->alloc != NULL) {
        panic("pool: create: got corrupted pool data from master pool!");
        return INVALID_POOL;
    }

    list_remove(&p->list);                 // remove pool from global free list
    list_add(&g_poolinfo->list, &p->list); // add to used list

    g_poolinfo->count++;
    assert(g_poolinfo->count <= MAX_NR_POOLS);

    p->order = order;
    p->alloc = alloc;
    p->name = name;
    p->size = size;
    p->capacity = capacity;

    // now, we could put the chunk metadata before the allocation slot, or we
    // could stuff it all somewhere else. one is more prone to corruption, while
    // the other uses more data... hmmm, decisions decisions...

    // let's put it at the start of the allocation, before the data
    list_init(&p->free_list);
    for (int i = 0; i < p->capacity; i++) {
        struct chunk *chunk = ((struct chunk *) p->alloc) + i;
        chunk->magic = CHUNK_MAGIC;
        chunk->pool = INVALID_POOL;
        list_add(&p->free_list, &chunk->list);
    }

    kprint("pool: created '%s' size_pages=%d capacity=%d item_size=%d flags=%Xh\n",
        name, get_order_size(p->order) >> PAGE_SHIFT, capacity, size, flags);
    return p;
}

void pool_destroy(pool_t *pool)
{
    struct pool *p;
    const char *name;

    if (!pool_valid(pool)) {
        return;
    }

    // ensure pool is real
    // TODO: could check if pool is in addr range instead of using a loop...
    p = NULL;
    for (list_iterator(n, &g_poolinfo->list)) {
        p = list_item(n, struct pool, list);
        if (p == pool) {
            break;
        }
    }
    if (p == NULL) {
        return; // invalid pool
    }

    name = p->name;
    p->name = NULL;

    free_pages(p->alloc, p->order);
    p->alloc = NULL;

    list_remove(&p->list);                      // remove from used list
    list_add(&g_poolinfo->free_list, &p->list); // add to free list

    g_poolinfo->count--;
    assert(g_poolinfo->count >= 0);

    kprint("pool: destroyed '%s'\n", name);
}

void * pool_alloc(pool_t *pool, int flags)
{
    struct chunk *chunk;
    int index;

    (void) flags;   // TODO: flags

    if (!pool_valid(pool)) {
        return NULL;
    }

    if (list_empty(&pool->free_list)) {
        warn("pool: %s: alloc failed: pool is full!\n", pool->name);
        return NULL;
    }

    chunk = list_item(pool->free_list.next, struct chunk, list);
    if (chunk->magic != CHUNK_MAGIC || chunk->pool != INVALID_POOL) {
        panic("pool: %s: alloc failed: got corrupted chunk data", pool->name);
        return NULL;
    }
    list_remove(&chunk->list);  // remove from free list

    pool->count++;
    assert(pool->count <= pool->capacity);

    index = (chunk - (struct chunk *) pool->alloc);
    assert(index >= 0);
    assert(index < pool->capacity);

    chunk->pool = pool;
    chunk->data = pool->alloc + (pool->capacity * sizeof(struct chunk)) + (index * pool->size);
    assert(chunk->data >= pool->alloc);
    assert(chunk->data < pool->alloc + pool->capacity * (sizeof(struct chunk) + pool->size));
    // TODO: consider flags for alignment and zeroing for chunk data

    return chunk->data;
}

void pool_free(pool_t *pool, const void *item)
{
    struct chunk *chunk;
    int index;

    if (!pool_valid(pool) || item == NULL) {
        return;
    }

    const size_t chunk_area_size = pool->size * pool->capacity;
    const uintptr_t chunk_base = (uintptr_t) pool->alloc + (pool->capacity * sizeof(struct chunk));
    const uintptr_t item_addr = (uintptr_t) item;

    if (item_addr < chunk_base || item_addr > chunk_base + chunk_area_size) {
        warn("pool: %s: free failed: address invalid\n", pool->name);
        return;
    }

    index = ((int) item - chunk_base) / pool->size;
    chunk = (struct chunk *) pool->alloc + index;
    assert(chunk->data == item);
    if (chunk->data != item) {
        panic("pool: %s: free failed: got corrupted chunk data", pool->name);
        return;
    }

    chunk->pool = INVALID_POOL;
    list_add(&pool->free_list, &chunk->list);

    pool->count--;
    assert(pool->count >= 0);
}
