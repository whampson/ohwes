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
 *         File: include/kernel/ring.h
 *      Created: June 15, 2024
 *       Author: Wes Hampson
 *
 * A double-ended fixed-length character ring.
 * =============================================================================
 */

#ifndef __RING_H
#define __RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ring {
    char *buf;         // character ring buffer pointer
    size_t length;      // total ring buffer size
    size_t count;       // number of characters in the ring
    uint32_t head;      // head pointer
    uint32_t tail;      // tail pointer
};

/**
 * Initialize the character ring using the specified buffer.
 * TODO: allocate buffer internally so we don't need to supply our own.
 *
 * @param r     a pointer to the ring to to initialize
 * @param buf   a pre-allocated character buffer used to store the characters in
 *              the ring
 * @param len   the size of the character buffer
 */
void ring_init(struct ring* r, char *buf, size_t len);

/**
 * Check whether the character ring is empty.
 *
 * @param r     a pointer to the ring to check for emptiness
 * @return      `true` if the ring is empty
 */
bool ring_empty(const struct ring* r);

/**
 * Check whether the character ring is full.
 *
 * @param r     a pointer to the ring to check for fullness
 * @return      `true` if the ring is full
 */
bool ring_full(const struct ring* r);

/**
 * Pop a character from the front of the ring.
 *
 * NOTE: It is advised that you *always* check whether the ring is empty before
 * attempting to pop from the ring. If the ring is empty, a NUL character will
 * be returned, however, depending on how you use the ring, NUL characters may
 * be a valid characters within the ring, thus one should not rely on a NUL
 * being returned as surefire indicator that the ring is empty.
 *
 * @param r     a pointer to the ring to pop from
 * @return      the popped character, or `\0` if the ring is empty
 */
char ring_get(struct ring* r);

/**
 * Push a character into the back of the ring.
 *
 * @param r     a pointer to the ring to push to
 * @param c     the character to put into the ring
 * @return      `true` if the character was added (ring not full)
 */
bool ring_put(struct ring* r, char c);

/**
 * Pop a character from the back of the ring.
 *
 * NOTE: It is advised that you *always* check whether the ring is empty before
 * attempting to pop from the ring. If the ring is empty, a NUL character will
 * be returned, however, depending on how you use the ring, NUL characters may
 * be a valid characters within the ring, thus one should not rely on a NUL
 * being returned as surefire indicator that the ring is empty.
 *
 * @param r     a pointer to the ring to pop from
 * @return      the popped character, or `\0` if the ring is empty
 */
char ring_erase(struct ring* r);

/**
 * Push a character into the front of the ring.
 *
 * @param r     a pointer to the ring to push to
 * @param c     the character to put into the ring
 * @return      `true` if the character was added (ring not full)
*/
bool ring_insert(struct ring* r, char c);

/**
 * Get the total capacity of the ring, i.e. the size of the underlying
 * character buffer.
 *
 * @param r     a pointer to the ring
 * @return      the size of the underlying character buffer
 */
size_t ring_length(struct ring* r);

/**
 * Get the number of characters currently in the ring.
 *
 * @param r     a pointer to the ring
 * @return      the number of characters in the ring
 */
size_t ring_count(struct ring* r);

/**
 * Reset ring buffer; revert to default empty state.
 *
 * @param r     a pointer to the ring to reset
 */
void ring_reset(struct ring* r);


#endif  // __RING_H
