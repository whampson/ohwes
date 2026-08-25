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

 // NOT threadsafe!

#ifndef __RING_H
#define __RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ring {
    void *buf;      // externally-allocated backing buffer
    size_t cap;     // maximum number of elements that can fit in the buffer
    size_t count;   // current number of elements in the buffer
    size_t head;    // current forward-read position
    size_t tail;    // current forward-write position
};

#define fifo_push   ring_push_back
#define fifo_pop    ring_pop_front
#define fifo_peek   ring_peek_front

#define lifo_push   ring_push_back
#define lifo_pop    ring_pop_back
#define lifo_peek   ring_peek_back

// initializer list
// NOTE: capacity must be >0 */
#define RING_INIT(buffer, capacity) { \
    .buf = (buffer), .cap = (capacity), \
    .count = 0, .head = 0, .tail = 0, \
}

// initialize / reinitialize
#define ring_init(r, buffer, capacity) \
({ \
    struct ring *_r = (r); \
    _r->buf = (buffer); \
    _r->cap = (capacity) < 1 ? 1 : (capacity); \
    _r->count = _r->head = _r->tail = 0; \
    (void)0; \
})

#define ring_capacity(r)    ((r)->cap)
#define ring_count(r)       ((r)->count)
#define ring_head(r)        ((r)->head)
#define ring_tail(r)        ((r)->tail)
#define ring_empty(r)       ((r)->count == 0)
#define ring_full(r)        ((r)->count == (r)->cap)

#define ring_clear(r) \
({ \
    struct ring *_r = (r); \
    _r->count = _r->head = _r->tail = 0; \
    (void)0; \
})

#define ring_peek_back(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_empty(_r) ? false : \
    ( \
        (item) = ((T*)_r->buf)[(_r->tail - 1 + _r->cap) % _r->cap], \
        true \
    ); \
})

#define ring_peek_front(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_empty(_r) ? false : \
    ( \
        (item) = ((T*)_r->buf)[_r->head], \
        true \
    ); \
})

#define ring_peek_at ring_get_at

#define ring_get_at(r, pos, item, T) \
({ \
    struct ring *_r = (r); \
    size_t _pos = (pos); \
    (ring_empty(_r) || _pos >= _r->count) ? false : \
    ( \
        (item) = ((T*)_r->buf)[(_r->head + _pos) % _r->cap], \
        true \
    ); \
})

#define ring_set_at(r, pos, item, T) \
({ \
    struct ring *_r = (r); \
    size_t _pos = (pos); \
    (ring_empty(_r) || _pos >= _r->count) ? false : \
    ( \
        ((T*)_r->buf)[(_r->head + _pos) % _r->cap] = (item), \
        true \
    ); \
})

#define ring_push_back(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_full(_r) ? false : \
    ( \
        ((T*)_r->buf)[_r->tail] = (item), \
        _r->tail = (_r->tail + 1) % _r->cap, \
        _r->count++, \
        true \
    ); \
})

#define ring_push_front(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_full(_r) ? false : \
    ( \
        _r->head = (_r->head - 1 + _r->cap) % _r->cap, \
        ((T*)_r->buf)[_r->head] = (item), \
        _r->count++, \
        true \
    ); \
})

#define ring_pop_back(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_empty(_r) ? false : \
    ( \
        _r->count--, \
        _r->tail = (_r->tail - 1 + _r->cap) % _r->cap, \
        (item) = ((T*)_r->buf)[_r->tail], \
        true \
    ); \
})

#define ring_pop_front(r, item, T) \
({ \
    struct ring *_r = (r); \
    ring_empty(_r) ? false : \
    ( \
        _r->count--, \
        (item) = ((T*)_r->buf)[_r->head], \
        _r->head = (_r->head + 1) % _r->cap, \
        true \
    ); \
})

#define ring_iterator(r, item, T, idx) \
    struct ring *_r = (r); \
    idx < _r->count && ((item) = ((T*)_r->buf)[(_r->head + idx) % _r->cap], true); \
    idx++

#define ring_reverse_iterator(r, item, T, idx) \
    struct ring *_r = (r); \
    idx < _r->count && ((item) = ((T*)_r->buf)[(_r->tail - idx - 1 + _r->cap) % _r->cap], true); \
    idx++

#endif // __RING_H