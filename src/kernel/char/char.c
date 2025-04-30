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
 *         File: kernel/char/char.c
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <kernel/char.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>

struct chdev {
    const char *name;
    struct file_ops *fops;
};
static struct chdev chdevs[MAX_CHDEV] = { };

int register_chdev(uint16_t major, const char *name, struct file_ops *fops)
{
    if (major == 0 || major >= MAX_CHDEV || !name || !fops) {
        return -EINVAL;
    }

    if (chdevs[major].fops && chdevs[major].fops != fops) {
        return -EBUSY;
    }

    chdevs[major].name = name;
    chdevs[major].fops = fops;
    return 0;
}

int chdev_open(struct inode *inode, struct file *file)
{
    if (!inode) {
        return -EINVAL;
    }

    // TODO: verify dev_id is a char dev

    uint16_t major = _DEV_MAJ(inode->device);
    if (major == 0 || major >= MAX_CHDEV) {
        return -ENODEV;     // not a char dev
    }

    struct chdev *chdev = &chdevs[major];
    if (!chdev->fops || !chdev->fops->open) {
        return -ENXIO;     // device not registered
    }

    return chdev->fops->open(inode, file);
}

struct file_ops chdev_ops = {
    .open = chdev_open,
    .close = NULL,
    .read = NULL,
    .write = NULL,
    .ioctl = NULL
};
