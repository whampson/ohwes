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
 *         File: kernel/fs.c
 *      Created: December 11, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <string.h>
#include <kernel/config.h>
#include <kernel/kernel.h>
#include <kernel/fs.h>
#include <kernel/list.h>
#include <kernel/pool.h>
#include <kernel/ohwes.h>


static list_t inodes;
static pool_t *inode_pool;

static list_t dentries;
static pool_t *dentry_pool;

static pool_t *file_pool;

extern struct file_ops chdev_ops;

static struct dentry * create_file(const char *name)
{
    struct dentry *dentry = NULL;
    struct inode *inode = NULL;

    dentry = pool_alloc(dentry_pool, 0);
    if (!dentry) {
        return NULL;
    }
    list_add_tail(&dentries, &dentry->dentries);
    strncpy(dentry->name, name, DENTRY_NAME_LENGTH);

    inode = pool_alloc(inode_pool, 0);
    if (!inode) {
        return NULL;
    }
    list_add_tail(&inodes, &inode->inodes);

    dentry->inode = inode;
    return dentry;
}

//
// ----------------------------------------------------------------------------
// HACK: file operations for a text file until we get some kind of file system
// driver written...
//

static size_t readme_fptr = 0;

int readme_open(struct inode *inode, struct file *file)
{
    (void) inode; (void) file;

    readme_fptr = 0;
    return 0;
}

int readme_close(struct file *file)
{
    (void) file;
    readme_fptr = 0;
    return 0;
}
ssize_t readme_read(struct file *file, char *buf, size_t count)
{
    (void) file; (void) buf; (void) count;

    static const char *ozy =
"I met a traveller from an antique land\n\
Who said: Two vast and trunkless legs of stone\n\
Stand in the desert.... Near them, on the sand,\n\
Half sunk, a shattered visage lies, whose frown,\n\
And wrinkled lip, and sneer of cold command,\n\
Tell that its sculptor well those passions read\n\
Which yet survive, stamped on these lifeless things,\n\
The hand that mocked them, and the heart that fed;\n\
And on the pedestal, these words appear:\n\
'My name is Ozymandias, King of Kings,\n\
Look on my Works, ye Mighty, and despair!'\n\
Nothing beside remains. Round the decay\n\
Of that colossal Wreck, boundless and bare\n\
The lone and level sands stretch far away.\n";

    size_t ozy_len = strlen(ozy);
    if (readme_fptr >= ozy_len) {
        return EOF;
    }

    int nread = min(count, ozy_len - readme_fptr);
    strncpy(buf, ozy + readme_fptr, nread);
    readme_fptr += nread;

    return nread;
}

struct file_ops readme_fops = {
    .open = readme_open,
    .close = readme_close,
    .read = readme_read,
    .write = NULL,
    .ioctl = NULL
};

//
// END HACK
// ----------------------------------------------------------------------------
//

void init_fs(void)
{
    list_init(&inodes);
    inode_pool = pool_create("inodes", MAX_NR_INODES, sizeof(struct inode), 0);
    if (inode_pool == INVALID_POOL) {
        panic("failed to create inode pool!");
    }

    list_init(&dentries);
    dentry_pool = pool_create("dentries", MAX_NR_DENTRIES, sizeof(struct dentry), 0);
    if (dentry_pool == INVALID_POOL) {
        panic("failed to create dentry pool!");
    }

    file_pool = pool_create("files", MAX_NR_TOTAL_OPEN, sizeof(struct file), 0);
    if (file_pool == INVALID_POOL) {
        panic("failed to create file pool!");
    }

    // create TTY dentries
    for (int i = 0; i < NR_TTY; i++) {
        char name[DENTRY_NAME_LENGTH];
        if (i <= TTY_MAX) {
            snprintf(name, DENTRY_NAME_LENGTH, "/dev/tty%d", i);
        }
        else {
            snprintf(name, DENTRY_NAME_LENGTH, "/dev/ttyS%d", i-TTYS_MIN+1);
        }

        if (i == 0) {
            // no inode for tty0, yet...
            continue;
        }

        struct dentry *dentry = create_file(name);
        if (!dentry) {
            panic("failed to create '%s'\n", name);
        }
        dentry->inode->device = __mkdev(TTY_MAJOR, i);
        dentry->inode->fops = &chdev_ops;
    }

    struct dentry *readme_dentry = create_file("readme.txt");
    readme_dentry->inode->fops = &readme_fops;  // HACK
}

struct inode * find_inode(struct file *file, const char *name)
{
    struct dentry *dentry;
    struct inode *inode;

    dentry = NULL;
    for (list_iterator(n, &dentries)) {
        struct dentry *d = list_item(n, struct dentry, dentries);
        if (strncmp(name, d->name, DENTRY_NAME_LENGTH) == 0) {
            dentry = d;
            break;
        }
    }

    if (!dentry) {
        return NULL;
    }
    if (!dentry->inode) {
        assert(dentry->inode);
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

    struct file *f = pool_alloc(file_pool, 0);
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
