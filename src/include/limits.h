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
 *         File: include/limits.h
 *      Created: April 14, 2025
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/header/limits
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/limits.h.html
 * =============================================================================
 */

#ifndef __LIMITS_H
#define __LIMITS_H

#define CHAR_BIT        __CHAR_BIT__
#define CHAR_MAX        __CHAR_MAX__
#define CHAR_MIN        (-CHAR_MAX-1)
#define SCHAR_MAX       __SCHAR_MAX__
#define SCHAR_MIN       (-SCHAR_MAX-1)
// #define WCHAR_MAX       __WCHAR_MAX__
// #define WCHAR_MIN       (-WCHAR_MAX-1)
#define SHRT_MAX        __SHRT_MAX__
#define SHRT_MIN        (-SHRT_MAX-1)
#define INT_MAX         __INT_MAX__
#define INT_MIN         (-INT_MAX-1)
#define LONG_MAX        __LONG_MAX__
#define LONG_MIN        (-LONG_MAX-1)
#define LLONG_MAX       __LONG_LONG_MAX__
#define LLONG_MIN       (-LLONG_MAX-1)
// #define WINT_MAX        __WINT_MAX__
// #define WINT_MIN        (-WINT_MAX-1)
#define UCHAR_MAX       (CHAR_MIN+CHAR_MAX)
#define USHRT_MAX       (SHRT_MIN+SHRT_MAX)
#define UINT_MAX        (INT_MIN+INT_MAX)
#define ULONG_MAX       (LONG_MIN+LONG_MAX)
#define ULLONG_MAX      (LLONG_MIN+LLONG_MAX)

#endif // __LIMITS_H
