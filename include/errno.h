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
#define ENFILE      6   // Too many files open in system
#define ENODEV      7   // No such device
#define ENOENT      8   // No such file or directory
#define ENOMEM      9   // Not enough memory
#define ENOSYS      10  // Function not implemented
#define ENOTTY      11  // Invalid I/O control operation
#define ENXIO       12  // No such device or address


// TODO: consider this instead...? more descriptive
// #define E_INVALID_ARGUMENT
// #define E_INVALID_SYSTEM_CALL
// #define E_INVALID_FUNCTION
// #define E_ALREADY_IN_USE

#ifndef __ASSEMBLER__
extern int _errno;
#define errno _errno
#endif


#endif // __ERRNO_H
