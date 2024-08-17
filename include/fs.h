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
 *         File: include/fs.h
 *      Created: March 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __FS_H
#define __FS_H

#include <ohwes.h>
#include <stddef.h>

// TODO: remove these
#define stdin_fd                        0
#define stdout_fd                       1

struct file;
struct file_ops {
    int     (*open)(struct file **, int);   // TODO: rethink double ptr
    int     (*close)(struct file *);
    ssize_t (*read)(struct file *, char *, size_t);
    ssize_t (*write)(struct file *, const char *, size_t);
    int     (*ioctl)(struct file *, unsigned int, unsigned long);
};

struct file {
    int ioctl_code;
    struct file_ops *fops;
};

#endif // __FS_H
