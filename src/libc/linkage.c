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
 *         File: src/libc/syscall.c
 *      Created: June 19, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <unistd.h>
#include <i386/syscall.h>

//
// System Call Linkage
//

LINK_SYSCALL1_VOID(_exit, int,status)
LINK_SYSCALL1(int,close, int,fd)
LINK_SYSCALL1(int,dup, int,fd)
LINK_SYSCALL2(int,dup2, int,fd, int,newfd)
LINK_SYSCALL3(int,fcntl, int,fd, int,op, unsigned long,arg)
LINK_SYSCALL3(int,ioctl, int,fd, int,op, unsigned long,arg)
LINK_SYSCALL2(int,open, const char *,name, int,flags)
LINK_SYSCALL3(int,read, int,fd, void *,buf, size_t,count)
LINK_SYSCALL3(int,write, int,fd, const void *,buf, size_t,count)
