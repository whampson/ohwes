
/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/io.c                                                       *
 * Created: December 16, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <errno.h>
#include <types.h>
#include <ohwes/syscall.h>

ssize_t read(int fd, void *buf, size_t n)
{
    syscall_setup;
    syscall3(SYS_READ, fd, buf, n);
    syscall_ret;
}

ssize_t write(int fd, const void *buf, size_t n)
{
    syscall_setup;
    syscall3(SYS_WRITE, fd, buf, n);
    syscall_ret;
}
