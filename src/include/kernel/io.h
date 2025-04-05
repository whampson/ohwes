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
 *         File: include/kernel/io.h
 *      Created: April 5, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __IO_H
#define __IO_H

#include <stdint.h>

/**
 * Check whether a given I/O port range is in use.
 *
 * @param base  I/O port base address
 * @param count port count in range
 *
 * @return -EBUSY if range is reserved,
 *              0 if range is free
 */
int check_io_range(uint16_t base, uint8_t count);

/**
 * Reserve an I/O port range for use with a kernel-mode driver.
 *
 * @param base  I/O port base address
 * @param count port count in range
 *
 * @return -EBUSY  if range is already reserved,
 *         -ENOMEM if range list is full
 *               0 if range was free and is now reserved
 */
int reserve_io_range(uint16_t base, uint8_t count, const char *name);

/**
 * Release an I/O port range so it can be used by another driver.
 *
 * @param base  I/O port base address
 * @param count port count in range
 */
void release_io_range(uint16_t base, uint8_t count);

#endif // __IO_H
