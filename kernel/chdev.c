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
 *         File: kernel/chdev.c
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <chdev.h>
#include <errno.h>
#include <ohwes.h>

struct chdev {
    uint16_t major;
    const char *name;
    struct file_ops *fops;
};

struct chdev chdevs[MAX_CHDEV];

void init_chdev(void)
{
    zeromem(chdevs, sizeof(struct chdev) * MAX_CHDEV);
}

struct file_ops * get_chdev(uint16_t major)
{
    if (major >= MAX_CHDEV) {
        return NULL;
    }

    struct chdev *chdev = &chdevs[major];
    if (chdev->major != major) {
        return NULL;
    }

    return chdev->fops;
}

int chdev_open(struct inode *inode, struct file *file)
{
    struct chdev *chdev;

    if (!inode || !file) {
        return -EINVAL;
    }

    // TODO: verify dev_id is a char dev

    if (inode->dev_id >= MAX_CHDEV) {
        return -ENODEV;
    }

    chdev = &chdevs[inode->dev_id];
    if (!chdev->fops || !chdev->fops->open) {
        return -ENODEV;
    }

    return chdev->fops->open(inode, file);
}

int chdev_register(uint16_t major, const char *name, struct file_ops *fops)
{
    if (major >= MAX_CHDEV) {
        return -EINVAL;
    }
    if (chdevs[major].fops && chdevs[major].fops != fops) {
        return -EBUSY;
    }

    chdevs[major].major = major;
    chdevs[major].name = name;
    chdevs[major].fops = fops;
    return 0;
}

int chdev_unregister(uint16_t major, const char *name)
{
    if (major >= MAX_CHDEV) {
        return -EINVAL;
    }
    if (!chdevs[major].fops) {
        return -EINVAL;
    }
    if (strcmp(chdevs[major].name, name) != 0) {
        return -EINVAL;
    }

    chdevs[major].name = NULL;
    chdevs[major].fops = NULL;
    return 0;
}
