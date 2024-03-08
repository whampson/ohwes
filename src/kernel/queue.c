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
 *         File: kernel/queue.c
 *      Created: March 6, 2024
 *       Author: Wes Hampson
 *
 * =============================================================================
 */

#include <ohwes.h>
#include <assert.h>
#include <queue.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void q_init(queue_t *q, char *buf, size_t len)
{
    memset(q, 0, sizeof(struct _queue));
    q->ring = buf;
    q->len = len;
}

bool q_empty(const queue_t *q)
{
    return q->count == 0;
}

bool q_full(const queue_t *q)
{
    return q->count == q->len;
}

char q_get(queue_t *q)
{
    assert(!q_empty(q));
    if (q_empty(q)) {
        panic("attempt to get from an empty queue!");
    }

    char c = q->ring[q->rptr++];
    if (q->rptr >= q->len) {
        q->rptr = 0;
    }

    q->count--;
    return c;
}

void q_put(queue_t *q, char c)
{
    assert(!q_full(q));
    if (q_full(q)) {
        panic("attempt to put into a full queue!");
    }

    q->ring[q->wptr++] = c;
    if (q->wptr >= q->len) {
        q->wptr = 0;
    }

    q->count++;
}
