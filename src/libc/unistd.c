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

SYSCALL1_LINK_VOID(_exit, int,status)
SYSCALL1_LINK(int,close, int,fd)
SYSCALL1_LINK(int,dup, int,fd)
SYSCALL2_LINK(int,dup2, int,fd, int,newfd)
SYSCALL3_LINK(int,ioctl, int,fd, unsigned int,cmd, unsigned long,arg)
SYSCALL2_LINK(int,open, const char *,name, int,flags)
SYSCALL3_LINK(int,read, int,fd, void *,buf, size_t,count)
SYSCALL3_LINK(int,write, int,fd, const void *,buf, size_t,count)
