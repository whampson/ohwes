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
 *         File: kernel/ring.c
 *      Created: June 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <kernel/ring.h>

// TODO: support generic type

void ring_init(struct ring *r, char *buf, size_t length)
{
    memset(r, 0, sizeof(struct ring));
    r->buf = buf;
    r->length = length;
}

bool ring_empty(const struct ring *r)
{
    return r->count == 0;
}

bool ring_full(const struct ring *r)
{
    return r->count == r->length;
}

char ring_get(struct ring *r)
{
    if (ring_empty(r)) {
        return '\0';
    }

    char c = r->buf[r->head++];
    if (r->head >= r->length) {
        r->head = 0;
    }

    r->count--;
    return c;
}

bool ring_put(struct ring *r, char c)
{
    if (ring_full(r)) {
        return false;
    }

    r->buf[r->tail++] = c;
    if (r->tail >= r->length) {
        r->tail = 0;
    }

    r->count++;
    return true;
}

char ring_erase(struct ring *r)
{
    if (ring_empty(r)) {
        return '\0';
    }

    if (r->tail == 0) {
        r->tail = r->length;
    }

    r->count--;
    return r->buf[--r->tail];
}

bool ring_insert(struct ring *r, char c)
{
    if (ring_full(r)) {
        return false;
    }

    if (r->head == 0) {
        r->head = r->length;
    }
    r->buf[--r->head] = c;

    r->count++;
    return true;
}

size_t ring_length(struct ring *r)
{
    return r->length;
}

size_t ring_count(struct ring *r)
{
    return r->count;
}

void ring_reset(struct ring *r)
{
    r->head = r->tail = r->count = 0;
}
