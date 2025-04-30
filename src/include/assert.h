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
 *         File: src/include/assert.h
 *      Created: December 26, 2023
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/error (C11)
 * =============================================================================
 */

#ifndef __ASSERT_H
#define __ASSERT_H

#define static_assert _Static_assert

#define _ASSERT_STRING_FORMAT(x) \
    "%s(%d): assertion failed:\n    " #x, __FILE__, __LINE__

#ifdef DEBUG

#ifdef __KERNEL__
extern void __noreturn panic(const char *fmt, ...);

#define assert(x) \
do { \
    if (!(x)) { \
        panic(_ASSERT_STRING_FORMAT(x)); \
    } \
} while (0)

#else

#include <stdio.h>
#include <unistd.h>
#define assert(x) \
do { \
    if (!(x)) { \
        printf(_ASSERT_STRING_FORMAT(x)); \
        _exit(1); \
    } \
} while (0)

#endif  // __KERNEL__

#else   // DEBUG
#define assert(x) (void) (x)
#endif  // DEBUG

#endif  // __ASSERT_H
