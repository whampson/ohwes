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

// syscall kernel-mode entrypoints
extern int sys_exit();
extern int sys_read();
extern int sys_write();
extern int sys_open();
extern int sys_close();
extern int sys_ioctl();

typedef int (*fn_ptr)();

static const fn_ptr syscall_table[] =
{
    sys_exit, sys_read, sys_write, sys_open, sys_close, sys_ioctl,
};
static_assert(countof(syscall_table) == NUM_SYSCALLS, "syscall_table size");


int sys_exit(int status)
{
    assert(getpl() == KERNEL_PL);

    kprint("init: returned %d\n", status);
    idle(); // TODO: switch task, handle signals, etc.

    return 0;   // does not actually return
}

int sys_read(int fd, char *buf, size_t count)
{
    struct file *f;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_currtask->fds[fd])) {
        return -EBADF;
    }

    if (!f->fops || !f->fops->read) {
        return -EINVAL; // TODO: more appropriate return value here?
    }

    if (count == 0) {
        return 0;
    }

    // TODO: verify user-space buffer address range
    return f->fops->read(buf, count);
}

int sys_write(int fd, const char *buf, size_t count)
{
    struct file *f;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !(f = g_currtask->fds[fd])) {
        return -EBADF;
    }

    if (!f->fops || !f->fops->write) {
        return -EINVAL; // TODO: more appropriate return value here?
    }

    if (count == 0) {
        return 0;
    }

    // TODO: verify user-space buffer address range
    return f->fops->write(buf, count);
}

int sys_open(const char *name, int flags)
{
    return -ENOSYS;
}

int sys_close(int fd)
{
    return -ENOSYS;
}

int sys_ioctl(int fd, int cmd, unsigned long arg)
{
    return -ENOSYS;
}

__fastcall
int recv_syscall(struct iregs *regs)
{
    assert(regs->vec_num == IVT_SYSCALL);

    if (regs->eax >= NUM_SYSCALLS)
    {
        return -ENOSYS;
    }
    return syscall_table[regs->eax](regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
}
