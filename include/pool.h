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
 *         File: include/pool.h
 *      Created: November 18, 2024
 *       Author: Wes Hampson
 *
 * Intel 8259A Programmable Interrupt Controller interface.
 * =============================================================================
 */

#ifndef __POOL_H
#define __POOL_H

struct pool;
typedef struct pool * pool_t;

pool_t create_pool(uint32_t tag, void *addr, size_t capacity, size_t item_size);
int destroy_pool(pool_t pool);

void * pool_alloc(pool_t pool);
int pool_free(pool_t pool, void *item);

#endif // __POOL_H
