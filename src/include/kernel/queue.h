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
 *         File: src/include/kernel/queue.h
 *      Created: June 15, 2024
 *       Author: Wes Hampson
 *
 * A double-ended fixed-length character queue.
 * =============================================================================
 */

#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ring {
    char *ring;         // character queue ring buffer pointer
    size_t length;      // total ring buffer size
    size_t count;       // number of characters in the queue
    uint32_t head;      // head pointer
    uint32_t tail;      // tail pointer
};

#define RING_INITIALIZER(buf)   { .ring = buf, .length = sizeof(buf) }
// TODO: get ring of ring_init()

/**
 * Initialize the character queue using the specified buffer.
 * TODO: allocate buffer internally so we don't need to supply our own.
 *
 * @param q     a pointer to the ring to to initialize
 * @param buf   a pre-allocated character buffer used to store the characters in
 *              the queue
 * @param len   the size of the character buffer
 */
void ring_init(struct ring *q, char *buf, size_t len);

/**
 * Check whether the character queue is empty.
 *
 * @param q     a pointer to the ring to check for emptiness
 * @return      `true` if the queue is empty
 */
bool ring_empty(const struct ring *q);

/**
 * Check whether the character queue is full.
 *
 * @param q     a pointer to the ring to check for fullness
 * @return      `true` if the queue is full
 */
bool ring_full(const struct ring *q);

/**
 * Pop a character from the front of the queue.
 *
 * NOTE: It is advised that you *always* check whether the queue is empty before
 * attempting to pop from the queue. If the queue is empty, a NUL character will
 * be returned, however, depending on how you use the queue, NUL characters may
 * be a valid characters within the queue, thus one should not rely on a NUL
 * being returned as surefire indicator that the queue is empty.
 *
 * @param q     a pointer to the ring to pop from
 * @return      the popped character, or `\0` if the queue is empty
 */
char ring_get(struct ring *q);

/**
 * Push a character into the back of the queue.
 *
 * @param q     a pointer to the ring to push to
 * @param c     the character to put into the queue
 * @return      `true` if the character was added (queue not full)
 */
bool ring_put(struct ring *q, char c);

/**
 * Pop a character from the back of the queue.
 *
 * NOTE: It is advised that you *always* check whether the queue is empty before
 * attempting to pop from the queue. If the queue is empty, a NUL character will
 * be returned, however, depending on how you use the queue, NUL characters may
 * be a valid characters within the queue, thus one should not rely on a NUL
 * being returned as surefire indicator that the queue is empty.
 *
 * @param q     a pointer to the ring to pop from
 * @return      the popped character, or `\0` if the queue is empty
 */
char ring_erase(struct ring *q);

/**
 * Push a character into the front of the queue.
 *
 * @param q     a pointer to the ring to push to
 * @param c     the character to put into the queue
 * @return      `true` if the character was added (queue not full)
*/
bool ring_insert(struct ring *q, char c);

/**
 * Get the total capacity of the queue, i.e. the size of the underlying
 * character buffer.
 *
 * @param q     a pointer to the ring
 * @return      the size of the underlying character buffer
 */
size_t ring_length(struct ring *q);

/**
 * Get the number of characters currently in the queue.
 *
 * @param q     a pointer to the ring
 * @return      the number of characters in the queue
 */
size_t ring_count(struct ring *q);

void ring_clear(struct ring *q);

#endif  // __QUEUE_H
