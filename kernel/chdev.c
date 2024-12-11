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
    const char *name;
    struct file_ops *fops;
};

int chdev_open(struct inode *inode, struct file *file);

struct file_ops chdev_ops = {
    .open = chdev_open
};

extern struct file_ops tty_fops;

struct chdev g_chdevs[MAX_CHDEV] = { };

struct inode g_chdev_inodes[MAX_CHDEV_INODES] =
{
    { .device = __mkdev(TTY_MAJOR, 1) },   // tty1
    { .device = __mkdev(TTY_MAJOR, 2) },   // tty2
    { .device = __mkdev(TTY_MAJOR, 3) },   // tty3
    { .device = __mkdev(TTY_MAJOR, 4) },   // tty4
    { .device = __mkdev(TTY_MAJOR, 5) },   // tty5
    { .device = __mkdev(TTY_MAJOR, 6) },   // tty6
    { .device = __mkdev(TTY_MAJOR, 7) },   // tty7
    { .device = __mkdev(TTYS_MAJOR, 0) },  // ttyS0
    { .device = __mkdev(TTYS_MAJOR, 1) },  // ttyS1
    { .device = __mkdev(TTYS_MAJOR, 2) },  // ttyS2
    { .device = __mkdev(TTYS_MAJOR, 3) },  // ttyS3
    { .device = 0 },    // position-independent; end sentinel is device=0
};

int register_chdev(uint16_t major, const char *name, struct file_ops *fops)
{
    if (major == 0 || major >= MAX_CHDEV || !name || !fops) {
        return -EINVAL;
    }

    if (g_chdevs[major].fops && g_chdevs[major].fops != fops) {
        return -EBUSY;
    }

    g_chdevs[major].name = name;
    g_chdevs[major].fops = fops;
    return 0;
}

struct file_ops * get_chdev_fops(dev_t device)
{
    uint16_t major = _DEV_MAJ(device);
    if (major >= MAX_CHDEV) {
        return NULL;
    }

    struct chdev *chdev = &g_chdevs[major];
    return chdev->fops;
}

struct inode * get_chdev_inode(uint16_t major, uint16_t minor)
{
    if (major >= MAX_CHDEV) {
        return NULL;
    }

    struct inode *inode = &g_chdev_inodes[0];
    while (inode->device) {
        if (major == _DEV_MAJ(inode->device) &&
            minor == _DEV_MIN(inode->device)) {
            break;
        }
        inode++;
    }

    if (inode->device == 0) {
        return NULL;    // not found
    }

    return inode;
}

int chdev_open(struct inode *inode, struct file *file)
{
    uint16_t major;
    struct chdev *chdev;

    if (!inode) {
        return -EINVAL;
    }

    // TODO: verify dev_id is a char dev

    major = _DEV_MAJ(inode->device);
    if (major >= MAX_CHDEV) {
        return -ENODEV;     // not a char dev
    }

    chdev = &g_chdevs[major];
    if (!chdev->fops || !chdev->fops->open) {
        return -ENXIO;     // device not registered
    }

    return chdev->fops->open(inode, file);
}
