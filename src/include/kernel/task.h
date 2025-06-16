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
 *         File: src/include/kernel/task.h
 *      Created: July 31, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __TASK_H
#define __TASK_H

#define MAX_OPEN                    8
#define MAX_TASK                    64

#define TASK_IREGS                  0x0C

#ifndef __ASSEMBLER__
#include <i386/interrupt.h>
#include <kernel/fs.h>
#include <kernel/tty.h>

struct task {
    int pid;
    int errno;
    struct tty *tty;
    struct iregs *regs;
    struct file *files[MAX_OPEN];
};
static_assert(offsetof(struct task, regs) == TASK_IREGS,
    "offsetof(task, regs) == TASK_IREGS");

struct task * current_task(void);
struct task * get_task(int pid);
int get_pid(void);

#endif // __ASSENMBLER
#endif // __TASK_H
