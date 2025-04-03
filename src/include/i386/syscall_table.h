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
 *         File: include/syscall_table.h
 *      Created: December 18, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifdef __ASSEMBLER__
#define SYSCALL_DECLARE(name)    .long sys_##name
#else
#define SYSCALL_DECLARE(name)    _SYS_##name,
enum _syscall_nr {
#endif


//
// System Call Numbers
//

SYSCALL_DECLARE(_exit)
SYSCALL_DECLARE(read)
SYSCALL_DECLARE(write)
SYSCALL_DECLARE(open)
SYSCALL_DECLARE(close)
SYSCALL_DECLARE(ioctl)
SYSCALL_DECLARE(dup)
SYSCALL_DECLARE(dup2)
SYSCALL_DECLARE(fcntl)


#ifndef __ASSEMBLER__
NR_SYSCALLS
};
#endif

#undef SYSCALL_DECLARE
