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

#include <list.h>
#include <stdio.h>
#include <stdint.h>
#include <device.h>
#include <unistd.h>

#define DENTRY_NAME_LENGTH  32

#define MODE_CHRDEV     (1 << 0)
typedef uint32_t mode_t;

struct inode {
    mode_t mode;
    dev_t device;

    struct list_node inode_list;
    struct file_ops *fops;
};

struct dentry {
    char name[DENTRY_NAME_LENGTH];
    struct list_node dentry_list;
    struct inode *inode;
};

struct file;
struct file_ops {
    int     (*open)(struct inode *, struct file *);
    int     (*close)(struct file *);
    ssize_t (*read)(struct file *, char *, size_t);
    ssize_t (*write)(struct file *, const char *, size_t);
    int     (*ioctl)(struct file *, unsigned int, unsigned long);
};

struct file {
    struct file_ops *fops;
    void *private_data;     // TODO: needed?
};


int alloc_fd(struct file **file);
void free_fd(struct file *file);

struct inode * find_inode(struct file *file, const char *name);

#endif // __FS_H
