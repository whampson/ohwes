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
 *         File: lib/libc/crt.c
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <syscall.h>

int _errno;

//
// Usermode system call function definitions
//
SYSCALL1(exit, int,status)
SYSCALL3(read, int,fd, void*,buf, size_t,count)
SYSCALL3(write, int,fd, const void*,buf, size_t,count)
SYSCALL2(open, const char*,name, int,flags)
SYSCALL1(close, int,fd)
SYSCALL3(ioctl, int,fd, unsigned int,cmd, void*,arg)
