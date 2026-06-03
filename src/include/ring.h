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
 *         File: include/ring.h
 *      Created: May 29, 2026
 *       Author: Wes Hampson
 *
 * A double-ended fixed-length ring buffer.
 * =============================================================================
 */

#ifndef __RING_H
#define __RING_H

#include <stddef.h>
#include <stdint.h>

struct ring {
    void *buf;      // externally-allocated backing buffer
    size_t cap;     // maximum number of elements that can fit in the buffer
    size_t count;   // current number of elements in the buffer
    size_t head;    // current forward-read position
    size_t tail;    // current forward-write position
};

#define ring_push   ring_push_back
#define ring_pop    ring_pop_front
#define ring_peek   ring_peek_front

#define RING_INIT(buffer, capacity) \
    { .buf = (buffer), .cap = (capacity) }

#define ring_init(r, buffer, capacity) \
    (r)->buf = (buffer); \
    (r)->cap = (capacity); \
    (r)->count = 0; \
    (r)->head = 0; \
    (r)->tail = 0

#define ring_count(r) \
    (r)->count

#define ring_capacity(r) \
    (r)->cap

#define ring_empty(r) \
    ((r)->count == 0)

#define ring_full(r) \
    ((r)->count == (r)->cap)

#define ring_clear(r) \
    (r)->head = 0; \
    (r)->tail = 0; \
    (r)->count = 0

#define ring_peek_back(r, T) \
    ((T*)(r)->buf)[((r)->tail - 1 + (r)->cap) % (r)->cap]

#define ring_peek_front(r, T) \
    ((T*)(r)->buf)[(r)->head]

#define ring_push_back(r, item, T) \
( \
    ring_full(r) ? 0 : \
    ( \
        ((T*)(r)->buf)[(r)->tail] = (item), \
        (r)->tail = ((r)->tail + 1) % (r)->cap, \
        (r)->count++, \
        1 \
    ) \
)

#define ring_push_front(r, item, T) \
( \
    ring_full(r) ? 0 : \
    ( \
        (r)->head = ((r)->head - 1 + (r)->cap) % (r)->cap, \
        ((T*)(r)->buf)[(r)->head] = (item), \
        (r)->count++, \
        1 \
    ) \
)

#define ring_pop_back(r, item, T) \
( \
    ring_empty(r) ? 0 : \
    ( \
        (r)->count--, \
        (r)->tail = ((r)->tail - 1 + (r)->cap) % (r)->cap, \
        (item) = ((T*)(r)->buf)[(r)->tail], \
        1 \
    ) \
)

#define ring_pop_front(r, item, T) \
( \
    ring_empty(r) ? 0 : \
    ( \
        (r)->count--, \
        (item) = ((T*)(r)->buf)[(r)->head], \
        (r)->head = ((r)->head + 1) % (r)->cap, \
        1 \
    ) \
)

#define ring_get_at(r, pos, item, T) \
( \
    (ring_empty(r) || (pos) > (r)->count - 1) ? 0 : \
    ( \
        (item) = ((T*)(r)->buf)[((r)->head + (pos)) % (r)->cap], \
        1 \
    ) \
)

#define ring_set_at(r, pos, item, T) \
( \
    (ring_empty(r) || (pos) > (r)->count - 1) ? 0 : \
    ( \
        ((T*)(r)->buf)[((r)->head + (pos)) % (r)->cap] = (item), \
        1 \
    ) \
)

#define ring_iterator(r, item, T) \
    size_t _idx = ({ (item) = ((T*)(r)->buf)[(r)->head]; 0; }); \
    _idx < (r)->count; \
    _idx++, (item) = ((T*)(r)->buf)[((r)->head + _idx) % (r)->cap]

#define ring_reverse_iterator(r, item, T) \
    size_t _idx = ({ (item) = ((T*)(r)->buf)[(r)->tail - 1]; 0; }); \
    _idx < (r)->count; \
    _idx++, (item) = ((T*)(r)->buf)[((r)->tail - _idx - 1 + (r)->cap) % (r)->cap]

#endif // __RING_H
