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
 *         File: include/chdev.h
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CHDEV_H
#define __CHDEV_H

#include <fs.h>
#include <stdint.h>


#define MAX_CHDEV       8

#define TTY_DEVICE      1
#define TTYS_DEVICE     2
// #define KBD_MAJOR       3
// #define RTC_MAJOR       4
// #define PCSPK_MAJOR     5

struct file_ops * get_chdev(uint16_t major);

int chdev_open(struct inode *inode, struct file *file);  // TEMP

int chdev_register(uint16_t major, const char *name, struct file_ops *fops);
int chdev_unregister(uint16_t major, const char *name);

#endif // __CHDEV_H