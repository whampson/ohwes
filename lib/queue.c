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

void char_queue_init(struct char_queue *q, char *buf, size_t length)
{
    memset(q, 0, sizeof(struct char_queue));
    q->ring = buf;
    q->length = length;
}

bool char_queue_empty(const struct char_queue *q)
{
    return q->count == 0;
}

bool char_queue_full(const struct char_queue *q)
{
    return q->count == q->length;
}

char char_queue_get(struct char_queue *q)
{
    if (char_queue_empty(q)) {
        return '\0';
    }

    char c = q->ring[q->head++];
    if (q->head >= q->length) {
        q->head = 0;
    }

    q->count--;
    return c;
}

bool char_queue_put(struct char_queue *q, char c)
{
    if (char_queue_full(q)) {
        return false;
    }

    q->ring[q->tail++] = c;
    if (q->tail >= q->length) {
        q->tail = 0;
    }

    q->count++;
    return true;
}

char char_queue_erase(struct char_queue *q)
{
    if (char_queue_empty(q)) {
        return '\0';
    }

    if (q->tail == 0) {
        q->tail = q->length;
    }

    q->count--;
    return q->ring[--q->tail];
}

bool char_queue_insert(struct char_queue *q, char c)
{
    if (char_queue_full(q)) {
        return false;
    }

    if (q->head == 0) {
        q->head = q->length;
    }
    q->ring[--q->head] = c;

    q->count++;
    return true;
}

size_t char_queue_length(struct char_queue *q)
{
    return q->length;
}

size_t char_queue_count(struct char_queue *q)
{
    return q->count;
}
