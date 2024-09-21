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
 *         File: kernel/sys/open.c
 *      Created: April 2, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <errno.h>
#include <syscall.h>
#include <task.h>
#include <chdev.h>
#include <string.h>

#define MAX_PATH    256

struct dev_file {
    dev_t dev_id;
    const char *name;
};

extern struct file_ops chdev_ops;

struct file fd_pool[64];    // TODO: dynamically alloc
uint64_t fd_mask;

void init_fs(void)
{
    fd_mask = 0;
    zeromem(fd_pool, sizeof(fd_pool));
}

static struct file * find_next_fd(void)
{
    struct file *file;
    int index = -1;

    if (fd_mask == 0) {
        index = 0;
    }
    else {
        // TODO: bit scan forward
        uint64_t mask = fd_mask;
        for (int i = 0; i < countof(fd_pool); i++, mask >>= 1) {
            if (mask & 1) {
                index = i;
                break;
            }
        }
    }

    if (index < 0 || index >= countof(fd_pool)) {
        return NULL;
    }

    file = &fd_pool[index];
    zeromem(file, sizeof(struct file));

    fd_mask |= (1ULL << index);
    return file;
}

static void free_fd(struct file *file)
{
    // lol this is the worst
    for (int i = 0; i < countof(fd_pool); i++) {
        if (file == &fd_pool[i]) {
            fd_mask &= (1ULL << i);
            break;
        }
    }
}

static struct inode * find_file(struct file *file, const char *name)
{
    struct inode *inode;
    struct dev_file *dev;

    // TODO: TEMP get rid of this lol
    struct dev_file devs[] = {
        { .name = "/dev/tty1",  .dev_id = __mkdev(TTY_MAJOR,  1) },
        { .name = "/dev/tty2",  .dev_id = __mkdev(TTY_MAJOR,  2) },
        { .name = "/dev/tty3",  .dev_id = __mkdev(TTY_MAJOR,  3) },
        { .name = "/dev/tty4",  .dev_id = __mkdev(TTY_MAJOR,  4) },
        { .name = "/dev/tty5",  .dev_id = __mkdev(TTY_MAJOR,  5) },
        { .name = "/dev/tty6",  .dev_id = __mkdev(TTY_MAJOR,  6) },
        { .name = "/dev/tty7",  .dev_id = __mkdev(TTY_MAJOR,  7) },
        { .name = "/dev/ttyS0", .dev_id = __mkdev(TTYS_MAJOR, 0) },
        { .name = "/dev/ttyS1", .dev_id = __mkdev(TTYS_MAJOR, 1) },
        { .name = "/dev/ttyS2", .dev_id = __mkdev(TTYS_MAJOR, 2) },
        { .name = "/dev/ttyS3", .dev_id = __mkdev(TTYS_MAJOR, 3) },
        { .name = NULL, .dev_id = 0 },
    };


    dev = &devs[0];
    do {
        if (strcmp(name, dev->name) == 0) {
            break;
        }
    } while ((++dev)->name);

    inode = NULL;
    file->fops = NULL;

    if (dev->name) {
        file->fops = get_chdev_fops(_DEV_MAJ(dev->dev_id));
        inode = get_chdev_inode(_DEV_MAJ(dev->dev_id), _DEV_MIN(dev->dev_id));
    }

    return inode;
}

__syscall int sys_open(const char *name, int flags)
{
    int fd;
    int ret;
    uint32_t cli_flags;
    struct task *task;
    struct file *file;
    struct inode *inode;

    assert(getpl() == KERNEL_PL);
    cli_save(cli_flags); // prevent task switch

    task = current_task();

    // find next available file descriptor slot in current task struct
    fd = -1;
    for (int i = 0; i < MAX_OPEN; i++) {
        if (task->files[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        ret = -ENFILE;  // Too many open files in process
        goto done;
    }

    file = find_next_fd();
    if (file == NULL) {
        ret = -EMFILE;  // Too many open files in system
        goto done;
    }

    inode = find_file(file, name);
    if (!inode) {
        ret = -ENOENT;  // File not found
        goto done;
    }

    if (!file->fops) {
        ret = -ENOENT;  // No such file or directory
        goto done;
    }

    if (!file->fops->open) {
        ret = -ENOSYS;  // Function not implemented
        goto done;
    }

    ret = file->fops->open(inode, file);
    if (ret < 0) {
        goto done;
    }

    ret = fd;
    task->files[fd] = file;

done:
    restore_flags(cli_flags);
    return ret;
}

__syscall int sys_close(int fd)
{
    struct file *file;
    int ret;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN) {
        return -EBADF;
    }

    file = current_task()->files[fd];
    if (!file) {
        return -EBADF;
    }

    if (!file->fops || !file->fops->close) {
        return -ENOSYS;
    }

    ret = file->fops->close(file);
    if (ret < 0) {
        return ret;
    }

    free_fd(file);
    current_task()->files[fd] = NULL;
    return ret;
}
