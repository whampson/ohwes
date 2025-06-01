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
#define DECLARE_SYSCALL(name)    .long sys_##name
#else
#define DECLARE_SYSCALL(name)    _SYS_##name,
enum _syscall_nr {
#endif


//
// System Call Numbers
//

/*  0 */ DECLARE_SYSCALL(_exit)
/*  1 */ DECLARE_SYSCALL(read)
/*  2 */ DECLARE_SYSCALL(write)
/*  3 */ DECLARE_SYSCALL(open)
/*  4 */ DECLARE_SYSCALL(close)
/*  5 */ DECLARE_SYSCALL(ioctl)
/*  6 */ DECLARE_SYSCALL(dup)
/*  7 */ DECLARE_SYSCALL(dup2)
/*  8 */ DECLARE_SYSCALL(fcntl)


#ifndef __ASSEMBLER__
NR_SYSCALLS
};
#endif

#undef DECLARE_SYSCALL
