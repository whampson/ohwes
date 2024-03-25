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
 *         File: kernel/task.c
 *      Created: March 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <task.h>

int console_read(char *buf, size_t count);
int console_write(const char *buf, size_t count);

static struct task task_list[MAX_TASKS];

// TODO: put these somewhere else...
static struct file t0_files[MAX_OPEN_FILES];
static struct file_ops t0_fops[MAX_OPEN_FILES];

void init_task(void)
{
    struct task *t0;
    zeromem(task_list, sizeof(task_list));

    t0 = &task_list[0];

    t0->pid = 0;
    t0->errno = 0;

    t0->fds[stdin_fd] = &t0_files[stdin_fd];
    t0->fds[stdin_fd]->fops = &t0_fops[stdin_fd];
    t0->fds[stdin_fd]->fops->read = console_read;

    t0->fds[stdout_fd] = &t0_files[stdout_fd];
    t0->fds[stdout_fd]->fops = &t0_fops[stdout_fd];
    t0->fds[stdout_fd]->fops->write = console_write;

    g_currtask = t0;
}
