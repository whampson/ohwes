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
 *         File: include/unistd.h
 *      Created: December 16, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __UNISTD_H
#define __UNISTD_H

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *)0)
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef __SSIZE_T_DEFINED
#define __SSIZE_T_DEFINED
typedef signed long ssize_t;            // TODO: sys/types.h?
#endif

void _exit(int status);
int close(int fd);
int dup(int fd);
int dup2(int fd, int newfd);
int ioctl(int fd, unsigned int cmd, void *arg);     // TODO: sys/ioctl.h? also varargs?
int open(const char *path, int flags);              // TOOD: fcntl.h?
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);

#endif // __UNISTD_H
