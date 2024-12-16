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
 *         File: kernel/fs.c
 *      Created: December 11, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <config.h>
#include <errno.h>
#include <kernel.h>
#include <fs.h>
#include <list.h>
#include <pool.h>
#include <string.h>

static struct list_node inodes;
static struct inode _inode_pool[MAX_NR_INODES];
static pool_t inode_pool;

static struct list_node dentries;
static struct dentry _dentry_pool[MAX_NR_DENTRIES];
static pool_t dentry_pool;

static struct file _file_pool[MAX_NR_TOTAL_OPEN];
static pool_t file_pool;

extern struct file_ops chdev_ops;

void init_fs(void)
{
    list_init(&inodes);
    inode_pool = create_pool(_inode_pool, "inodes", MAX_NR_INODES, sizeof(struct inode));
    if (!inode_pool) {
        panic("failed to create inode pool!");
    }

    list_init(&dentries);
    dentry_pool = create_pool(_dentry_pool, "dentries", MAX_NR_DENTRIES, sizeof(struct dentry));
    if (!inode_pool) {
        panic("failed to create dentry pool!");
    }

    file_pool = create_pool(_file_pool, "files", MAX_NR_TOTAL_OPEN, sizeof(struct file));
    if (!inode_pool) {
        panic("failed to create file pool!");
    }

    // create TTY dentries
    for (int i = 0; i < NR_TTY; i++) {
        char name[DENTRY_NAME_LENGTH];
        if (i <= NR_CONSOLE) {
            snprintf(name, DENTRY_NAME_LENGTH, "/dev/tty%d", i);
        }
        else {
            snprintf(name, DENTRY_NAME_LENGTH, "/dev/ttyS%d", i-NR_CONSOLE-1);
        }

        struct dentry *dentry = NULL;
        struct inode *inode = NULL;

        dentry = pool_alloc(dentry_pool);
        dentry->inode = NULL;
        strncpy(dentry->name, name, DENTRY_NAME_LENGTH);
        list_add_tail(&dentries, &dentry->dentry_list);

        if (i == 0) {
            // no inode for tty0
            continue;
        }

        inode = pool_alloc(inode_pool);
        inode->mode = MODE_CHRDEV;
        inode->device = __mkdev(TTY_MAJOR, i);
        inode->fops = &chdev_ops;
        list_add_tail(&inodes, &inode->inode_list);

        dentry->inode = inode;
    }
}

struct inode * find_inode(struct file *file, const char *name)
{
    struct dentry *dentry;
    struct inode *inode;
    struct list_node *n;

    dentry = NULL;
    for (list_iterator(&dentries, n)) {
        struct dentry *d = list_item(n, struct dentry, dentry_list);
        if (strncmp(name, d->name, DENTRY_NAME_LENGTH) == 0) {
            dentry = d;
            break;
        }
    }

    file->fops = NULL;
    if (!dentry) {
        return NULL;
    }

    inode = dentry->inode;
    file->fops = inode->fops;
    return inode;
}


int alloc_fd(struct file **file)
{
    if (!file) {
        return -EINVAL;
    }

    struct file *f = pool_alloc(file_pool);
    if (!f) {
        return -ENOMEM;
    }

    *file = f;
    return 0;
}

void free_fd(struct file *file)
{
    if (!file) {
        return;
    }

    pool_free(file_pool, file);
}
