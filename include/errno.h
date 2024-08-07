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

#define EINVAL      1   // Invalid Argument
#define ENOSYS      2   // System Call Not Valid
#define EBADF       3   // Bad File Descriptor
#define ENOTTY      4   // Invalid IOCTL
#define EBADRQC     5   // Bad Request Code
#define ENOMEM      6   // No Memory


// TOOD: consider this instead...
// #define E_INVALID_ARGUMENT
// #define E_INVALID_SYSTEM_CALL
// #define E_INVALID_FUNCTION
// #define E_ALREADY_IN_USE

#ifndef __ASSEMBLER__
extern int _errno;
#define errno _errno
#endif


#endif // __ERRNO_H
