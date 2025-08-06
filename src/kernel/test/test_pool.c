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
 *         File: kernel/test/pool_test.c
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <test.h>
#include <kernel/pool.h>

struct thing {
    uint32_t id;
    char name[8];
};

#define _MAX_SLOTS 4
static struct thing _pool0[_MAX_SLOTS];
static struct thing _pool1[_MAX_SLOTS];

void test_pool(void)
{
    DECLARE_TEST("object pool");

    pool_t p0, p1;
    struct thing *thing0, *thing1;
    struct thing *things[_MAX_SLOTS*2];
    char zeros[sizeof(struct thing)];
    memset(zeros, 0, sizeof(zeros));

    // create/destroy single pool
    p0 = pool_create(_pool0, "p0", sizeof(struct thing), _MAX_SLOTS);
    VERIFY_IS_NOT_ZERO(p0);
    pool_destroy(p0);

    // fill pool
    p0 = pool_create(_pool0, "p0", sizeof(struct thing), _MAX_SLOTS);
    VERIFY_IS_NOT_ZERO(p0);
    for (int i = 0; i < _MAX_SLOTS; i++) {
        things[i] = pool_alloc(p0);
        VERIFY_IS_NOT_NULL(things[i]);
        things[i]->id = i;
        snprintf(things[i]->name, 8, "thing%d", i);
    }
    // ensure nothing got overwritten
    for (int i = 0; i < _MAX_SLOTS; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "thing%d", i);
        VERIFY_ARE_EQUAL(i, things[i]->id);
        VERIFY_IS_ZERO(strcmp(buf, things[i]->name));
    }
    // ensure we can't overflow
    thing0 = pool_alloc(p0);
    VERIFY_IS_NULL(thing0);

    // now, free one item and attempt to alloc
    pool_free(p0, things[0]);
    thing0 = pool_alloc(p0);
    VERIFY_IS_NOT_NULL(thing0);

    // bonus! was the new allocation zeroed?
    VERIFY_IS_ZERO(memcmp(zeros, thing0, sizeof(struct thing)));

    // destroy pool
    pool_destroy(p0);

    // create multiple concurrent pools, ensure there is no crosstalk
    p0 = pool_create(_pool0, "p0", sizeof(struct thing), _MAX_SLOTS);
    p1 = pool_create(_pool1, "p1", sizeof(struct thing), _MAX_SLOTS);
    VERIFY_IS_NOT_ZERO(p0);
    VERIFY_IS_NOT_ZERO(p1);
    VERIFY_ARE_NOT_EQUAL(p1, p0);

    // fill both pools
    for (int i = 0; i < _MAX_SLOTS; i++) {
        thing0 = pool_alloc(p0);
        thing1 = pool_alloc(p1);
        snprintf(thing0->name, 8, "thingA%d", i);
        snprintf(thing1->name, 8, "thingB%d", i);
        thing0->id = i;
        thing1->id = i+_MAX_SLOTS;
        things[i] = thing0;
        things[i+_MAX_SLOTS] = thing1;
    }
    // verify no corruption
    for (int i = 0; i < _MAX_SLOTS; i++) {
        char buf0[8], buf1[8];
        snprintf(buf0, sizeof(buf0), "thingA%d", i);
        snprintf(buf1, sizeof(buf1), "thingB%d", i);
        thing0 = things[i];
        thing1 = things[i+_MAX_SLOTS];
        VERIFY_ARE_NOT_EQUAL(thing0, thing1);
        VERIFY_ARE_EQUAL(i, thing0->id);
        VERIFY_ARE_EQUAL(i+_MAX_SLOTS, thing1->id);
        VERIFY_IS_ZERO(strncmp(buf0, thing0->name, 8));
        VERIFY_IS_ZERO(strncmp(buf1, thing1->name, 8));
    }

    // destroy pools
    pool_destroy(p1);
    pool_destroy(p0);
}
