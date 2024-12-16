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
#include <config.h>
#include <errno.h>
#include <paging.h>
#include <ohwes.h>
#include <list.h>
#include <pool.h>

#define CHATTY_POOL         1

#define POOL_MAGIC          'loop'  // 'pool'
#define CHUNK_MAGIC         'knhc'  // 'chnk'

#define POOL_NAME_LENGTH    32

struct pool;
struct chunk {
    uint32_t magic;                 // unique ID for chunk type
    uint32_t index;                 // item slot index
    struct list_node chain;         // chunk chain
    struct pool *pool;              // pool chunk belongs to
    void *data;                     // chunk data
};

struct pool {
    uint32_t magic;                 // unique ID for pool type
    char name[POOL_NAME_LENGTH+1];  // pool name
    int index;                      // pool index
    void *base;                     // pool data base address
    size_t item_size;               // pool item size
    size_t capacity;                // pool item capacity
    list_t head;                    // chunk chain head
    struct chunk *chunk_base;       // chunk data base address
    struct chunk *alloc;            // current chunk allocation pointer
};

static struct pool _pools[MAX_NR_POOLS];
static struct chunk _chunks[MAX_NR_POOL_ITEMS];
static __data_segment bool _pools_initialized = false;

static char _pool_mask[MAX_NR_POOLS>>3];    // bit[n]==1 => n'th slot free
static char _chunk_mask[MAX_NR_POOL_ITEMS>>3];

// bitmap fns require bitstrings to be in DWORDs...
static_assert(MAX_NR_POOLS % 4 == 0,      "MAX_NR_POOLS must be a multiple of 4");
static_assert(MAX_NR_POOL_ITEMS % 4 == 0, "MAX_NR_POOL_ITEMS must be a multiple of 4");

#if DEBUG
static void print_chunk_chain(pool_t pool);

static void print_chunk_mask(void);
static void print_pool_mask(void);
#endif

static bool ensure_capacity(size_t capacity);

static struct pool * desc2pool(pool_t pool);
static pool_t pool2desc(struct pool *pool);

static unsigned int get_pool_index(pool_t pool);
static const char * get_pool_name(pool_t pool);
static uintptr_t get_base(pool_t pool);
static uintptr_t get_limit(pool_t pool);
static size_t get_item_size(pool_t pool);
static size_t get_capacity(pool_t pool);

static bool pool_valid(pool_t);                     // pool desc valid?
static bool addr_valid(pool_t pool, void *addr);    // item addr in pool range?

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

pool_t create_pool(void *addr, const char *name, size_t item_size, size_t capacity)
{
    int pool_index;
    struct pool *p;
    struct chunk *chunk;

    if (!_pools_initialized) {
        panic("pools not yet initialized!");
    }

    if (!name || !addr) {
        return NULL;
    }
    // TODO: verify address is in kernel space...

    // ensure we have space
    if (!ensure_capacity(capacity)) {
        panic("not enough pool memory to create pool");
        return NULL;
    }

    // find a free pool slot
    pool_index = bit_scan_forward(_pool_mask, sizeof(_pool_mask));
    if (pool_index == -1) {
        panic("max number of pools reached!");
        return NULL;
    }

    // setup pool descriptor
    clear_bit(_pool_mask, pool_index);
    p = &_pools[pool_index];
    p->magic = POOL_MAGIC;
    p->index = pool_index;
    p->base = addr;
    p->item_size = item_size;
    p->capacity = capacity;
    strncpy(p->name, name, POOL_NAME_LENGTH);
    list_init(&p->head);

    // create free list
    for (int i = 0; i < capacity; i++) {
        int chunk_index = bit_scan_forward(_chunk_mask, sizeof(_chunk_mask));
        clear_bit(_chunk_mask, chunk_index);
        chunk = &_chunks[chunk_index];
        chunk->magic = CHUNK_MAGIC;
        chunk->index = i;   // index in local pool, not global chunk pool
        chunk->pool = p;
        chunk->data = NULL;
        if (i == 0) {
            p->chunk_base = chunk;
            p->alloc = chunk;
        }
        list_add_tail(&p->head, &chunk->chain);
    }

#if CHATTY_POOL
    kprint("pool[%d]: create: %08X-%08X capacity=%d item_size=%d %s\n",
        get_pool_index(p), get_base(p), get_limit(p),
        get_capacity(p), get_item_size(p), get_pool_name(p));
#endif

    return pool2desc(p);
}

void destroy_pool(pool_t pool)
{
    struct pool *p;

    if (!pool_valid(pool)) {
        return;
    }

    p = desc2pool(pool);
    for (int i = 0; i < p->capacity; i++) {
        int chunk_index = p->chunk_base - _chunks;
        set_bit(_chunk_mask, chunk_index + i);
        zeromem(&_chunks[chunk_index], sizeof(struct chunk));
    }

    struct pool copy = *p;
    set_bit(_pool_mask, get_pool_index(pool));
    zeromem(p, sizeof(struct pool));

#if CHATTY_POOL
    kprint("pool[%d]: destroyed: %s\n",
        get_pool_index(&copy), get_pool_name(&copy))
#endif
}

void * pool_alloc(pool_t pool)
{
    struct pool *p;
    struct chunk *chunk;

    // is this a valid pool?
    if (!pool_valid(pool)) {
        return NULL;
    }
    p = pool;

    // do we have any free chunks left?
    if (list_empty(&p->head)) {
        return NULL;
    }

    // get next free chunk, advance alloc ptr, then remove chunk from free list
    chunk = p->alloc;
    assert(chunk->data == NULL);
    p->alloc = list_item(chunk->chain.next, struct chunk, chain);
    list_remove(&chunk->chain);

    // calculate data ptr from pool base using chunk index
    chunk->data = p->base + (p->item_size * chunk->index);

    // zero the memory and call it a day
    zeromem(chunk->data, p->item_size);
    return chunk->data;
}

int pool_free(pool_t pool, void *item)
{
    struct pool *p;
    struct chunk *chunk;
    int index;

    // is this a valid pool?
    if (!pool_valid(pool) || !item) {
        return -EINVAL;
    }
    p = pool;

    // did the user pass a pointer that's actually in the pool?
    if (!addr_valid(pool, item)) {
        return -EPERM;  // access denied, address out of range
    }

    // locate the index of the item within the pool
    index = (((char *) item) - ((char *) p->base)) / p->item_size;
    assert(index >= 0 && index < p->capacity);

    // locate the chunk data within the chunk array
    chunk = p->chunk_base + index;
    assert(chunk->index == index);
    assert(chunk->data != NULL);

    // free chunk by marking its data ptr invalid and adding it to free list
    chunk->data = NULL;
    list_add(&p->head, &chunk->chain);

    // set the alloc ptr to the free chunk and call it a day
    p->alloc = chunk;
    return 0;
}

// ----------------------------------------------------------------------------

#if DEBUG
static void print_chunk_chain(pool_t pool)
{
    struct list_node *n;
    struct pool *p;

    p = desc2pool(pool);
    kprint("{ ");
    for (list_iterator(&p->head, n)) {
        struct chunk *c = list_item(n, struct chunk, chain);
        kprint("%d ", c->index);
    }
    kprint("}\n");
}

static void print_chunk_mask(void)
{
    for (int i = sizeof(_chunk_mask) - 1; i >= 0; i--) {
        kprint("%02X", _chunk_mask[i] & 0xFF);
    }
    kprint("\n");
}

static void print_pool_mask(void)
{
    for (int i = sizeof(_pool_mask) - 1; i >= 0; i--) {
        kprint("%02X", _pool_mask[i] & 0xFF);
    }
    kprint("\n");
}
#endif

static bool ensure_capacity(size_t capacity)
{
    const uint32_t *_chunk_mask_dwords = (const uint32_t *) _chunk_mask;

    size_t avail = 0;
    for (int i = 0; i < sizeof(_chunk_mask) >> 2; i++) {
        avail += __builtin_popcount(_chunk_mask_dwords[i]);
    }

    return avail >= capacity;
}

static struct pool * desc2pool(pool_t pool)
{
    return (struct pool *) pool;
}

static pool_t pool2desc(struct pool *pool)
{
    return (pool_t) pool;
}

static unsigned int get_pool_index(pool_t pool)
{
    return desc2pool(pool)->index;
}

static const char * get_pool_name(pool_t pool)
{
    return desc2pool(pool)->name;
}

static uintptr_t get_base(pool_t pool)
{
    return (uintptr_t) desc2pool(pool)->base;
}

static uintptr_t get_limit(pool_t pool)
{
    size_t pool_size = get_item_size(pool) * get_capacity(pool);
    return get_base(pool) + pool_size - 1;
}

static size_t get_item_size(pool_t pool)
{
    return desc2pool(pool)->item_size;
}

static size_t get_capacity(pool_t pool)
{
    return desc2pool(pool)->capacity;
}

static bool pool_valid(pool_t pool)
{
    return desc2pool(pool) >= &_pools[0] && desc2pool(pool) < &_pools[MAX_NR_POOLS]
        && get_pool_index(pool) == (desc2pool(pool) - &_pools[0])
        && ((struct pool *) pool)->magic == POOL_MAGIC
        && ((struct pool *) pool)->base != NULL;
}

static bool addr_valid(pool_t pool, void *addr)
{
    uintptr_t ptr = (uintptr_t) addr;
    return ptr >= get_base(pool) && ptr <= get_limit(pool);
}
