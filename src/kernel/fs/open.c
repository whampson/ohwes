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
 *         File: src/kernel/fs/open.c
 *      Created: April 2, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <i386/syscall.h>
#include <i386/x86.h>
#include <kernel/config.h>
#include <kernel/char.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>
#include <kernel/task.h>

static int dupfd(int fd, int newfd);
static int find_next_fd(struct task *task);

SYSCALL_DEFINE(dup, int fd)
{
    return dupfd(fd, 0);
}

SYSCALL_DEFINE(dup2, int fd, int newfd)
{
    return dupfd(fd, newfd);
}

SYSCALL_DEFINE(open, const char *name, int oflag)
{
    int fd;
    int ret;
    uint32_t cli_flags;
    struct task *task;
    struct file *file;
    struct inode *inode;

    const int req_oflag_mask = oflag & (O_RDONLY | O_WRONLY | O_RDWR);
    if ((req_oflag_mask & (req_oflag_mask - 1)) != 0) {
        return -EINVAL; // must specify exactly one of O_RDONLY, O_WRONLY, or O_RDWR
    }

    assert(getpl() == KERNEL_PL);
    cli_save(cli_flags); // prevent task switch

    task = current_task();

    // find next available file descriptor slot in current task struct
    fd = find_next_fd(task);
    if (fd < 0) {
        ret = -ENFILE;  // Too many open files in process
        goto done;
    }

    ret = alloc_fd(&file);      // TODO: should be alloc_file_struct or something
    if (ret == ENOMEM) {
        ret = -EMFILE;  // Too many open files in system
        goto done;
    }
    file->f_oflag = oflag;

    inode = find_inode(file, name);
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

SYSCALL_DEFINE(close, int fd)
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

static int dupfd(int fd, int newfd)
{
    int ret;
    struct task *task;
    struct file *newfile;
    struct file *file;

    if (fd < 0 || fd >= MAX_OPEN) {
        return -EBADF;
    }
    if (newfd < 0 || newfd >= MAX_OPEN) {
        return -EBADF;
    }
    task = current_task();

    // resolve file descriptor
    file = task->files[fd];
    if (!file) {
        return -EBADF;
    }

    if (newfd) {
        // close the existing file descriptor if we specified one
        sys_close(newfd);
    }
    else {
        // otherwise locate the next free descriptor
        newfd = find_next_fd(task);
        if (newfd < 0) {
            return -EMFILE;
        }
    }

    // allocate a new file struct
    ret = alloc_fd(&newfile);
    if (ret == ENOMEM) {
        return -EMFILE;  // Too many open files in system
    }

    // duplicate
    *newfile = *file;
    task->files[newfd] = newfile;
    return newfd;
}

static int find_next_fd(struct task *task)
{
    int fd = -1;
    for (int i = 0; i < MAX_OPEN; i++) {
        if (task->files[i] == NULL) {
            fd = i;
            break;
        }
    }

    return fd;
}
