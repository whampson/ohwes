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

#include <config.h>
#include <chdev.h>
#include <errno.h>
#include <pool.h>
#include <ohwes.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

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

__syscall int sys_dup(int fd)
{
    return dupfd(fd, 0);
}

__syscall int sys_dup2(int fd, int newfd)
{
    return dupfd(fd, newfd);
}
