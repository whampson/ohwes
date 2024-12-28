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
#define EAGAIN      1   // Resource unavailable, try again
#define EBADF       2   // Bad file descriptor
#define EBADRQC     3   // Invalid request descriptor
#define EBUSY       4   // Device or resource busy
#define EINVAL      5   // Invalid argument
#define EIO         6   // Input/output error
#define EMFILE      7   // Too many files open in process
#define ENFILE      8   // Too many files open in system
#define ENODEV      9   // No such device
#define ENOENT      10   // No such file or directory
#define ENOMEM      11  // Not enough memory
#define ENOSYS      12  // Function not implemented
#define ENOTTY      13  // Invalid I/O control operation
#define ENXIO       14  // No such device or address
#define EPERM       15  // Operation not permitted

#ifndef __ASSEMBLER__
extern int _errno;
#define errno _errno
#endif

#endif // __ERRNO_H
