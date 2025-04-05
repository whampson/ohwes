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
 *         File: src/include/kernel/pool.h
 *      Created: November 18, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __POOL_H
#define __POOL_H

#include <stddef.h>

typedef void * pool_t;

#define INVALID_POOL ((pool_t) NULL)

/**
 * Create a new pool.
 */
pool_t create_pool(void *addr, const char *name, size_t item_size, size_t capacity);

/**
 * Destroy an existing pool.
 */
void destroy_pool(pool_t pool);

/**
 * Get an existing pool by name.
 */
pool_t find_pool(const char *name);

/**
 * Allocate an item within a given pool.
*/
void * pool_alloc(pool_t pool);

/**
 * Free an item from a given pool.
 */
int pool_free(pool_t pool, void *item);

#endif // __POOL_H
