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
 *         File: src/include/errno.h
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __ERRNO_H
#define __ERRNO_H

//
// Selected POSIX error numbers:
//
#define ENOMEM      1   // Not enough memory
#define ENFILE      2   // Too many files open in system
#define EBADF       3   // Bad file descriptor
#define ENOSYS      4   // Function not implemented
#define EMFILE      5   // Too many files open in process
#define ERANGE      6   // Result too large
#define ENODEV      7   // No such device
#define EPERM       8   // Operation not permitted
#define EFAULT      9   // Bad address
#define EINVAL      10  // Invalid argument
#define ENXIO       11  // No such device or address
#define ENOTTY      12  // Invalid I/O control operation
#define EAGAIN      13  // Resource unavailable, try again
#define ENOENT      14  // No such file or directory
#define EBUSY       15  // Device or resource busy
#define EBADRQC     16  // Invalid request descriptor
#define EIO         17  // Input/output error

#ifndef __ASSEMBLER__
extern int _errno;
#define errno _errno
#endif

#endif // __ERRNO_H
