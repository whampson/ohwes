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
 *         File: lib/syscall.c
 *      Created: June 19, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <syscall.h>

//
// system call linkage
//
_syscall3(read, int,fd, void*,buf, size_t,count)
_syscall3(write, int,fd, const void*,buf, size_t,count)
_syscall2(open, const char*,name, int,flags)
_syscall1(close, int,fd)
_syscall3(ioctl, int,fd, unsigned int,cmd, void*,arg)

void exit(int status)
{
    __asm__ volatile ("int $0x80" :: "a"(_sys_exit), "b"(status));
    for (;;);
}
