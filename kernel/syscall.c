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
 *         File: kernel/syscall.c
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <console.h>
#include <errno.h>
#include <fs.h>
#include <task.h>
#include <syscall.h>

// !!!!!!!
// TODO: All of these need to safely access the current task struct, to prevent
// corruption in the event of a task switch.
// !!!!!!!

// indicate user pointer
#define _USER_

int __syscall __noreturn sys_exit(int status)
{
    assert(getpl() == KERNEL_PL);

    kprint("\nexit: returned %d\n", status);
    idle();     // TODO: switch task, handle signals, etc.

    die();
}

int __syscall sys_read(int fd, _USER_ char *buf, size_t count)
{
    struct file *f;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_task->open_files[fd])) {
        return -EBADF;
    }
    if (!f->fops || !f->fops->read) {
        return -ENOSYS;
    }
    if (count == 0) {
        return 0;
    }

    // TODO: verify user-space buffer address range
    return f->fops->read(f, buf, count);
}

int __syscall sys_write(int fd, const _USER_ char *buf, size_t count)
{
    struct file *f;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_task->open_files[fd])) {
        return -EBADF;
    }
    if (!f->fops || !f->fops->write) {
        return -ENOSYS;
    }
    if (count == 0) {
        return 0;
    }

    // TODO: verify user-space buffer address range
    return f->fops->write(f, buf, count);
}

int __syscall sys_close(int fd)
{
    struct file *f;
    int ret;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_task->open_files[fd])) {
        return -EBADF;
    }
    if (!f->fops) {
        return -ENOSYS;
    }

    ret = 0;
    if (f->fops->close) {
        ret = f->fops->close(f);
    }
    // TODO: if ret < 0, do we return and NOT close out the fd?

    g_task->open_files[fd] = NULL;
    return ret;
}

int __syscall sys_ioctl(int fd, unsigned int cmd, void *arg)
{
    struct file *f;
    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_task->open_files[fd])) {
        return -EBADF;
    }
    if (!f->fops || !f->fops->ioctl) {
        return -ENOSYS;
    }

    return f->fops->ioctl(f, cmd, arg);
}
