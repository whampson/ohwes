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
 *         File: kernel/pool.c
 *      Created: November 18, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <pool.h>
#include <errno.h>

// TODO: This pool design has a problem in that its very easy to overrun the
// pool buffer as it approaches capacity. The chunk data is stored in-band and
// the caller doesn't necessarily know how much space the chunk data takes up,
// and a user typically provides the item count of the fixed-size array as the 
// capacity parameter.
// We need to store the chunk data out-of-band, but that requires allocating
// memory somewhere to store the chunk data, and then we need a way to allocate
// chunk data chunks... I smell a chicken and egg problem here... D: It can be
// done but it requires some thinking, but I think its a worthwhile investment
// because I'd rather not accidentally overrun buffers. Once we have a proper
// kmalloc written this won't be much of a problem because we can just ask for
// memory.

#define CHATTY_POOL  1

struct chunk {
    struct chunk *next;     // next chunk in chain
};

struct pool {
    const char *name;       // identifier
    bool valid;             // in use?
    uintptr_t base;         // base address of pool
    struct chunk *alloc;    // current alloc pointer
    size_t capacity;        // number of item slots
    size_t item_size;       // size of each allocable item
};

#define NUM_POOLS 8
struct pool g_pools[NUM_POOLS]; // global pool... pool :D
                                // gee, I could use a pool to pool all the pools
                                // in one big pool pool!

static unsigned int get_pool_index(pool_t pool);
static const char * get_pool_name(pool_t pool);
static uintptr_t get_pool_base(pool_t pool);
static uintptr_t get_pool_limit(pool_t pool);
static size_t get_pool_capacity(pool_t pool);
static size_t get_chunk_size(pool_t pool);

static bool is_pool_valid(pool_t pool);
static bool is_item_in_pool(pool_t pool, void *item);

pool_t create_pool(const char *name, void *addr, size_t capacity, size_t item_size)
{
    struct pool *p = NULL;
    for (int i = 0; i < NUM_POOLS; i++) {
        p = &g_pools[i];
        if (!p->valid) {
            break;
        }
    }

    if (NULL == p) {
        panic("no more pools available!");
        return NULL;
    }

    p->name = name;
    p->base = (uintptr_t) addr;
    p->alloc = (struct chunk *) addr;
    p->capacity = capacity;
    p->item_size = item_size;

    // create initial chunk chain
    struct chunk *chunk = p->alloc;
    for (int i = 0; i < capacity - 1; i++) {
        chunk->next = (struct chunk *) ((char *) (chunk + 1) + item_size);
        chunk = chunk->next;
    }
    chunk->next = NULL;
    p->valid = true;

#if CHATTY_POOL
    kprint("pool: '%s' created: id=%d range=0x%x-0x%x capacity=%d chunk_size=0x%x\n",
        name, get_pool_index(p), get_pool_base(p), get_pool_limit(p), capacity,
        get_chunk_size(p));
#endif

    return p;
}

int destroy_pool(pool_t pool)
{
    if (!is_pool_valid(pool)) {
        kprint("pool: attempt to free invalid pool (handle=0x%x)\n", pool);
        return -EINVAL;
    }

    struct pool *p = pool;
    zeromem(p, sizeof(struct pool));
    p->valid = false;

#if CHATTY_POOL
    kprint("pool: '%s' destroyed\n", get_pool_name(pool), get_pool_index(pool))
#endif

    return 0;
}

void * pool_alloc(pool_t pool)
{
    if (!is_pool_valid(pool)) {
        kprint("pool: attempt to allocate on an invalid pool (handle=0x%x)\n", pool);
        return NULL;
    }

    struct pool *p = pool;
    struct chunk *free_chunk = p->alloc;
    if (NULL == p->alloc) {
        return NULL;        // no more space! D:
    }

#if CHATTY_POOL
    kprint("pool: %s: chunk allocated at 0x%x\n", get_pool_name(p), free_chunk);
#endif

    p->alloc = free_chunk->next;
    return free_chunk + 1;  // advance ptr past chunk data
}

int pool_free(pool_t pool, void *item)
{
    if (!is_pool_valid(pool)) {
        kprint("pool: attempt to free item on an invalid pool (handle=0x%x)\n", pool);
        return -EINVAL;
    }

    if (!is_item_in_pool(pool, item)) {
        kprint("pool: %s: attempt to free an item not in the pool (item=0x%x)\n", get_pool_name(pool), item)
        return -EINVAL;
    }

    struct chunk *free_chunk = item;
    free_chunk = free_chunk - 1;    // back the ptr up to the chunk data
    // TODO: could do some math here to ensure the ptr points to the actual data
    // and not the next chunk pointer... or some other chunk

#if CHATTY_POOL
    kprint("pool: %s: freed chunk at 0x%x\n", get_pool_name(pool), free_chunk);
#endif

    free_chunk->next = pool->alloc;
    pool->alloc = free_chunk;
    return 0;
}

static unsigned int get_pool_index(pool_t pool)
{
    return pool - g_pools;
}

static const char * get_pool_name(pool_t pool)
{
    return ((struct pool *) pool)->name;
}

static uintptr_t get_pool_base(pool_t pool)
{
    return ((struct pool *) pool)->base;
}

static uintptr_t get_pool_limit(pool_t pool)
{
    size_t pool_size = get_chunk_size(pool) * get_pool_capacity(pool);
    return get_pool_base(pool) + pool_size - 1;
}

static size_t get_pool_capacity(pool_t pool)
{
    return ((struct pool *) pool)->capacity;
}

static size_t get_chunk_size(pool_t pool)
{
    return ((struct pool *) pool)->item_size + sizeof(struct chunk);
}

static bool is_pool_valid(pool_t pool)
{
    unsigned int index = get_pool_index(pool);
    return index < NUM_POOLS && ((struct pool *) pool)->valid;
}

static bool is_item_in_pool(pool_t pool, void *item)
{
    uintptr_t base = get_pool_base(pool);
    uintptr_t limit = get_pool_limit(pool);
    uintptr_t item_ptr = (uintptr_t) item;

    return (item_ptr > base) && (item_ptr <= limit);
}
