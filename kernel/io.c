
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
#include <ohwes/console.h>
#include <ohwes/io.h>
#include <ohwes/syscall.h>
#include <ohwes/ohwes.h>

__fastcall ssize_t sys_read(int fd, void *buf, size_t n)
{
    (void) fd;
    (void) buf;
    (void) n;

    return -ENOSYS;
}

__fastcall ssize_t sys_write(int fd, const void *buf, size_t n)
{
    (void) fd;
    (void) buf;
    (void) n;

    return -ENOSYS;
}

/*__fastcall*/ ssize_t /*sys_write*/write(int fd, const void *buf, size_t n)
{
    /* TODO: actually implement this */
    (void) fd;

    char *ptr = (char *) buf;
    for (size_t i = 0; i < n; i++) {
        con_write(ptr[i]);
    }

    return (ssize_t) n;
}
