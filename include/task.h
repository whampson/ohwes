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
 *         File: include/task.h
 *      Created: March 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __TASK_H
#define __TASK_H

#include <fs.h>
#include <ohwes.h>

#define MAX_OPEN_FILES              8
#define MAX_TASKS                   64

struct task {
    int pid;
    int errno;
    struct file *open_files[MAX_OPEN_FILES];

    // TODO: put these somewhere else...?
    struct file _files[MAX_OPEN_FILES];
    struct file_ops _fops[MAX_OPEN_FILES];
};

extern struct task *g_task;

#endif // __TASK_H