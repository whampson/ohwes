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
 *         File: src/include/kernel/fs.h
 *      Created: March 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __FS_H
#define __FS_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <kernel/device.h>
#include <kernel/list.h>

#define DENTRY_NAME_LENGTH  32

struct file;
struct inode;

struct file_ops {
    int     (*open)(struct inode *, struct file *);
    int     (*close)(struct file *);
    ssize_t (*read)(struct file *, char *, size_t);
    ssize_t (*write)(struct file *, const char *, size_t);
    int     (*ioctl)(struct file *, int, void *);
};

// represents an open file descriptor
struct file {
    uint32_t f_oflag;
    struct file_ops *fops;
    struct inode *inode;
    void *private_data;     // TODO: needed?
};


// represents a node to a file
struct inode {
    list_t inodes;
    dev_t device;
    struct file_ops *fops;
    // TODO: functions...
};

// represents an item in a file system directory
struct dentry {
    list_t dentries;
    char name[DENTRY_NAME_LENGTH];
    struct inode *inode;
};

// represents a mounted fle system
struct super_block {
    list_t list;
    dev_t  device;
    struct file_system *fs_type;
    struct dentry *root;

    struct inode * (*create_inode)(struct super_block *);
    void (*destroy_inode)(struct super_block *, struct inode *);
    int (*write_inode)(struct super_block *, struct inode *);

    void (*flush)(struct super_block *);
    void (*unmount)(struct super_block *);
    // TODO: stat
};

// represents a file system implementation (FAT, ext2, etc.)
struct file_system {
    const char *name;
    int flags;

    struct dentry * (*mount)(struct file_system *, int, const char *, void *);

    list_t fs_list;
};

int alloc_fd(struct file **file);
void free_fd(struct file *file);

struct inode * find_inode(struct file *file, const char *name);

#endif // __FS_H
