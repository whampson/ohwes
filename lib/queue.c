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
 *         File: lib/queue.c
 *      Created: June 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <queue.h>

void ring_init(struct ring *q, char *buf, size_t length)
{
    memset(q, 0, sizeof(struct ring));
    q->ring = buf;
    q->length = length;
}

bool ring_empty(const struct ring *q)
{
    return q->count == 0;
}

bool ring_full(const struct ring *q)
{
    return q->count == q->length;
}

char ring_get(struct ring *q)
{
    if (ring_empty(q)) {
        return '\0';
    }

    char c = q->ring[q->head++];
    if (q->head >= q->length) {
        q->head = 0;
    }

    q->count--;
    return c;
}

bool ring_put(struct ring *q, char c)
{
    if (ring_full(q)) {
        return false;
    }

    q->ring[q->tail++] = c;
    if (q->tail >= q->length) {
        q->tail = 0;
    }

    q->count++;
    return true;
}

char ring_erase(struct ring *q)
{
    if (ring_empty(q)) {
        return '\0';
    }

    if (q->tail == 0) {
        q->tail = q->length;
    }

    q->count--;
    return q->ring[--q->tail];
}

bool ring_insert(struct ring *q, char c)
{
    if (ring_full(q)) {
        return false;
    }

    if (q->head == 0) {
        q->head = q->length;
    }
    q->ring[--q->head] = c;

    q->count++;
    return true;
}

size_t ring_length(struct ring *q)
{
    return q->length;
}

size_t ring_count(struct ring *q)
{
    return q->count;
}

void ring_clear(struct ring *q)
{
    q->head = q->tail = q->count = 0;
}
