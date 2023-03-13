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
 *    File: lib/errno.c                                                       *
 * Created: December 22, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <errno.h>
#include <stdio.h>

int errno;

static const char *errno_text[MAX_ERRNO+1] =
{
    NULL,
    "Invalid argument",
    "Function not implemented",
    "Device temporarily unavailable"
};

void perror(const char *msg)
{
    if (msg != NULL) {
        puts(msg);
        puts(": ");
    }

    const char *err;
    if (errno > 0 && errno <= MAX_ERRNO) {
        err = errno_text[errno];
        puts(err);
        puts("\n");
    }
}
