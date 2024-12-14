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
 *         File: kernel/pool2.c
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <bitops.h>
#include <errno.h>
#include <paging.h>
#include <ohwes.h>
#include <list.h>
#include <pool.h>

#define CHATTY_POOL             1

// TODO: config.h
#define MAX_NR_POOLS            32
#define MAX_NR_POOL_ITEMS       256
static_assert(MAX_NR_POOLS % 4 == 0,      "MAX_NR_POOLS must be a multiple of 4");
static_assert(MAX_NR_POOL_ITEMS % 4 == 0, "MAX_NR_POOL_ITEMS must be a multiple of 4");

#define POOL_MAGIC              0x6C6F6F70   // 'pool'
#define CHUNK_MAGIC             0x6B6E6863   // 'chnk'

struct pool;
struct chunk {
    uint32_t magic;             // unique ID for chunk type
    struct list_node chain;     // free list chain
    struct pool *pool;          // pool chunk belongs to
    void *data;                 // chunk data
};

struct pool {
    uint32_t magic;             // unique ID for pool type
    const char *name;           // pool name
    uintptr_t base;             // pool data base address
    size_t item_size;           // pool item size
    size_t capacity;            // pool item capacity
    struct chunk *alloc;        // current allocation pointer
};

static struct pool _pools[MAX_NR_POOLS];
static struct chunk _chunks[MAX_NR_POOL_ITEMS];
static __data_segment bool _pools_initialized = false;

static char _pool_mask[MAX_NR_POOLS>>3];    // bit[n]==1 => n'th slot free
static char _chunk_mask[MAX_NR_POOL_ITEMS>>3];

static struct pool * get_pool(pool_t pool);
static unsigned int get_pool_index(pool_t pool);
static const char * get_pool_name(pool_t pool);
static uintptr_t get_pool_base(pool_t pool);
static uintptr_t get_pool_limit(pool_t pool);
static size_t get_pool_item_size(pool_t pool);
static size_t get_pool_capacity(pool_t pool);
static bool pool_valid(pool_t);
static bool ensure_capacity(size_t capacity);

void init_pools(void)
{
    zeromem(_pools, sizeof(_pools));
    zeromem(_chunks, sizeof(_chunks));
    memset(_pool_mask, 0xFF, sizeof(_pool_mask));
    memset(_chunk_mask, 0xFF, sizeof(_chunk_mask));
    _pools_initialized = true;

#if CHATTY_POOL
    size_t data_size = sizeof(_pools) + sizeof(_chunks) + sizeof(_pool_mask)
        + sizeof(_chunk_mask);
    kprint("pool data takes up %d bytes (%d pages)\n",
        data_size, PAGE_ALIGN(data_size) >> PAGE_SHIFT);
#endif
}

pool_t create_pool(const char *name, void *addr, size_t item_size, size_t capacity)
{
    int index, pool_index;
    struct pool *p;
    struct chunk *chunk;

    if (!_pools_initialized) {
        panic("pools not yet initialized!");
    }

    if (!name || !addr) {
        return NULL;
    }

    // ensure we have space
    if (!ensure_capacity(capacity)) {
        return NULL;
    }

    // find a free pool slot
    pool_index = bit_scan_forward(_pool_mask, sizeof(_pool_mask));
    if (pool_index == -1) {
        return NULL;
    }

    // setup pool descriptor
    p = &_pools[pool_index];
    clear_bit(_pool_mask, pool_index);
    p->magic = POOL_MAGIC;
    p->name = name;
    p->base = (uintptr_t) addr; // TODO: keep void* ?
    p->item_size = item_size;
    p->capacity = capacity;

    // create free list
    for (int i = 0; i < capacity; i++) {
        index = bit_scan_forward(_chunk_mask, sizeof(_chunk_mask));
        clear_bit(_chunk_mask, index);  // 0 means in-use
        chunk = &_chunks[index];
        chunk->magic = CHUNK_MAGIC;
        chunk->pool = p;
        chunk->data = NULL;
        if (i == 0) {
            list_init(&chunk->chain);
            p->alloc = chunk;
        }
        else {
            list_add_tail(&p->alloc->chain, &chunk->chain);
        }
    }

#if CHATTY_POOL
    kprint("pool: %08X-%08X: %s: id=%d capacity=%d item_size=%d\n",
        get_pool_base(p), get_pool_limit(p), get_pool_name(p),
        get_pool_index(p), get_pool_capacity(p), get_pool_item_size(p));
#endif

    return (pool_t) p;
}

int destroy_pool(pool_t pool)
{
    // TODO
    return 0;
}

void * pool_alloc(pool_t pool)
{
    // TODO
    return NULL;
}

int pool_free(pool_t pool, void *item)
{
    // TODO
    return 0;
}

// ----------------------------------------------------------------------------

static struct pool * get_pool(pool_t pool)
{
    return (struct pool *) pool;
}

static unsigned int get_pool_index(pool_t pool)
{
    return get_pool(pool) - _pools;
}

static const char * get_pool_name(pool_t pool)
{
    return get_pool(pool)->name;
}

static uintptr_t get_pool_base(pool_t pool)
{
    return get_pool(pool)->base;
}

static size_t get_pool_item_size(pool_t pool)
{
    return get_pool(pool)->item_size;
}

static size_t get_pool_capacity(pool_t pool)
{
    return get_pool(pool)->capacity;
}

static uintptr_t get_pool_limit(pool_t pool)
{
    size_t pool_size = get_pool_item_size(pool) * get_pool_capacity(pool);
    return get_pool_base(pool) + pool_size - 1;
}

static bool pool_valid(pool_t pool)
{
    unsigned int index = get_pool_index(pool);
    return index < MAX_NR_POOLS && ((struct pool *) pool)->base;
}

static bool ensure_capacity(size_t capacity)
{
    const uint32_t *_chunk_mask_dwords = (const uint32_t *) _chunk_mask;

    size_t avail = 0;
    for (int i = 0; i < sizeof(_chunk_mask) >> 2; i++) {
        avail += __builtin_popcount(_chunk_mask_dwords[i]);
    }

    return avail >= capacity;
}
