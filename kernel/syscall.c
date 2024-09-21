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
#include <ioctl.h>
#include <task.h>
#include <syscall.h>
#include <paging.h>
#include <kernel.h>

// !!!!!!!
// TODO: All of these need to safely access the current task struct, to prevent
// corruption in the event of a task switch.
// !!!!!!!


//
// User mode entry points for kernel functions.
//
__syscall __noreturn int sys_exit(int status)
{
    assert(getpl() == KERNEL_PL);

    kprint("\nexit: returned %d\n", status);

    die();
}

__syscall int sys_read(int fd, void *buf, size_t count)
{
    struct file *f;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN || !(f = current_task()->files[fd])) {
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

__syscall int sys_write(int fd, const void *buf, size_t count)
{
    struct file *f;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN || !(f = current_task()->files[fd])) {
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

__syscall int sys_ioctl(int fd, unsigned int num, unsigned long arg)
{
    uint32_t seq;
    uint32_t code;
    uint32_t size;
    uint32_t dir;
    struct file *f;

    assert(getpl() == KERNEL_PL);

    if (fd < 0 || fd >= MAX_OPEN || !(f = current_task()->files[fd])) {
        return -EBADF;
    }
    if (!f->fops || !f->fops->ioctl) {
        return -ENOSYS;
    }

    seq = (num & _IOCTL_SEQMASK) >> _IOCTL_SEQSHIFT;
    code = (num & _IOCTL_CODEMASK) >> _IOCTL_CODESHIFT;
    size = (num & _IOCTL_SIZEMASK) >> _IOCTL_SIZESHIFT;
    dir = (num & _IOCTL_DIRMASK) >> _IOCTL_DIRSHIFT;

#if PRINT_IOCTL
    kprint("ioctl: 0x%08X (seq=%d,code=%d,size=%d%s)\n",
        num, seq, code, size,
        ((dir & _IOCTL_READ) && (dir & _IOCTL_WRITE)) ? ",dir=rw," : // yeesh...
            (dir & _IOCTL_READ) ? ",dir=r" :
                (dir & _IOCTL_WRITE) ? ",dir=w" : "");
#else
    (void) seq;
    (void) code;
    (void) size;
    (void) dir;
#endif

    // bad IOCTL number, size must be nonzero if direction bits set!
    if (dir && size == 0) {
        return -EBADRQC;
    }

    // ensure buffer is not null if I/O direction bits set
    if (dir && !arg) {
        return -EINVAL;
    }

    // TODO: validate size encoded in num
    // TODO: validate buffer address and range
    // TODO: validate device number

    return f->fops->ioctl(f, num, arg);
}
