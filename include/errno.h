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
 *         File: include/errno.h
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __ERRNO_H
#define __ERRNO_H

//
// Selected POSIX error numbers:
//
#define EBADF       1   // Bad file descriptor
#define EBADRQC     2   // Invalid request descriptor
#define EBUSY       3   // Device or resource busy
#define EINVAL      4   // Invalid argument
#define EIO         5   // Input/output error
#define EMFILE      6   // Too many files open in process
#define ENFILE      7   // Too many files open in system
#define ENODEV      8   // No such device
#define ENOENT      9   // No such file or directory
#define ENOMEM      10  // Not enough memory
#define ENOSYS      11  // Function not implemented
#define ENOTTY      12  // Invalid I/O control operation
#define ENXIO       13  // No such device or address
#define EPERM       14  // Operation not permitted

#ifndef __ASSEMBLER__
extern int _errno;
#define errno _errno
#endif

#endif // __ERRNO_H
