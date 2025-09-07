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
    int order;          // order of master pool allocation
    pool_t *pools;      // pool of pools
    list_t list;        // list of active pools
    list_t free_list;   // list of free pools
};
static struct pool_info _pools;
struct pool_info *g_pools = &_pools;

void lazy_init_pools(void)
{
    size_t num_pools = MAX_NR_POOLS;
    size_t master_pool_size = align(num_pools * sizeof(struct pool), 4);
    g_pools->order = get_order(master_pool_size);

    g_pools->pools = (struct pool *) alloc_pages(ALLOC_ZERO, g_pools->order);
    if (g_pools->pools == NULL) {
        panic("not enough memory for pools!\n");
    }

    list_init(&g_pools->list);
    list_init(&g_pools->free_list);

    for (int i = 0; i < num_pools; i++) {
        list_add(&g_pools->free_list, &g_pools->pools[i].list);
    }
}

void destroy_pools()
{
    if (g_pools->pools) {
        free_pages(g_pools->pools, g_pools->order);
        g_pools->pools = NULL;
    }
}

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
    if (g_pools->pools == NULL) {
        lazy_init_pools();
    }
    assert(g_pools->pools != NULL);

    // ensure name is unique
    for (list_iterator(n, &g_pools->list)) {
        struct pool *other = list_item(n, struct pool, list);
        if (strncmp(other->name, name, POOL_MAX_NAME) == 0) {
            return INVALID_POOL;
        }
    }

    // ensure we have space for a new pool
    if (list_empty(&g_pools->free_list)) {
        panic("max number of pools allocated!!");
        return INVALID_POOL;
    }

    // initialize pool
    struct pool *p = list_item(g_pools->free_list.next, struct pool, list);
    assert(p->alloc == NULL);

    size_t size_bytes = capacity * (size + sizeof(struct chunk));
    p->order = get_order(size_bytes);
    p->alloc = alloc_pages(ALLOC_ZERO, p->order);
    if (p->alloc == NULL) {
        warn("pool: not enough memory for pool size=%d capacity=%d!\n", size, capacity);
        return NULL;
    }

    p->magic = POOL_MAGIC;
    p->name = name;
    p->size = size;
    p->capacity = capacity;
    p->count = 0;

    list_remove(&p->list);                      // remove from free list
    list_add_tail(&g_pools->list, &p->list);    // add to used list
    list_init(&p->free_list);                   // init chunk free list

    // now, we could put the chunk metadata before the allocation slot, or we
    // could stuff it all somewhere else. one is more prone to corruption, while
    // the other uses more data... hmmm, decisions decisions...

    // let's put it at the start of the allocation, before the data
    for (int i = 0; i < p->capacity; i++) {
        struct chunk *chunk = ((struct chunk *) p->alloc) + i;
        list_add(&p->free_list, &chunk->list);
    }

    kprint("pool: create %08X-%08X capacity=%d item_size=%d flags=%d %s\n",
        p->alloc, p->alloc+get_order_size(p->order)-1, capacity, size, flags, name);
    return p;
}

void pool_destroy(pool_t *pool)
{
    struct pool *p;
    const char *name;
    void *alloc;
    int order;

    if (!pool_valid(pool)) {
        return;
    }

    // ensure pool is real
    // TODO: could check if pool is in addr range instead of using a loop...
    p = NULL;
    for (list_iterator(n, &g_pools->list)) {
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

    alloc = p->alloc;
    order = p->order;
    free_pages(p->alloc, p->order);
    p->alloc = NULL;

    list_remove(&p->list);                      // remove from used list
    list_add(&g_pools->free_list, &p->list);    // add to free list

    if (list_empty(&g_pools->list)) {
        destroy_pools();
    }

    kprint("pool: destroy %08X-%08X %s\n", alloc, alloc+get_order_size(order)-1, name);
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
        warn("pool: alloc failed: pool '%s' is full!\n", pool->name);
        return NULL;
    }

    chunk = list_item(pool->free_list.next, struct chunk, list);
    assert(chunk->pool == INVALID_POOL);

    index = (chunk - (struct chunk *) pool->alloc);
    assert(index >= 0);
    assert(index < pool->capacity);

    chunk->magic = CHUNK_MAGIC;
    chunk->pool = pool;
    chunk->data = pool->alloc + (pool->capacity * sizeof(struct chunk)) + (index * pool->size);
    // TODO: consider alignment for chunk data

    list_remove(&chunk->list);                  // remove from free list
    list_add_tail(&pool->list, &chunk->list);   // add to used list
    pool->count++;

    assert(pool->count <= pool->capacity);
    return chunk->data;
}

void pool_free(pool_t *pool, const void *item)
{
    struct chunk *chunk;

    if (!pool_valid(pool) || item == NULL) {
        return;
    }

    const size_t pool_size = pool->size * pool->capacity;
    const uintptr_t pool_data = (uintptr_t) pool->alloc + (pool->capacity * sizeof(struct chunk));
    const uintptr_t item_addr = (uintptr_t) item;

    if (item_addr < pool_data || item_addr > pool_data + (pool_size)) {
        return;
    }

    chunk = NULL;
    for (list_iterator(n, &pool->list)) {
        struct chunk *c = list_item(n, struct chunk, list);
        if (c->data == item) {
            chunk = c;
            break;
        }
    }

    if (chunk != NULL) {
        chunk->pool = INVALID_POOL;
        list_remove(&chunk->list);
        list_add(&pool->free_list, &chunk->list);
        pool->count--;
    }

    assert(pool->count >= 0);
}
