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
#define SCHAR_MAX       ((1<<(CHAR_BIT-1))-1)
#define SCHAR_MIN       (-SCHAR_MAX-1)
#define UCHAR_MAX       ((1<<CHAR_BIT)-1)

#ifndef __CHAR_UNSIGNED__
#define CHAR_MAX        SCHAR_MAX
#define CHAR_MIN        SCHAR_MIN
#else
#define CHAR_MAX        UCHAR_MAX
#define CHAR_MIN        0
#endif

#define SHRT_MAX        __SHRT_MAX__
#define SHRT_MIN        (-SHRT_MAX-1)
#define USHRT_MAX       0xffff

#define INT_MAX         __INT_MAX__
#define INT_MIN         (-INT_MAX-1)
#define UINT_MAX        0xffffffff

#define LONG_MAX        __LONG_MAX__
#define LONG_MIN        (-LONG_MAX-1)
#define ULONG_MAX       0xffffffffUL

#define LLONG_MAX       __LONG_LONG_MAX__
#define LLONG_MIN       (-LLONG_MAX-1)
#define ULLONG_MAX      0xffffffffffffffffULL

#endif // __LIMITS_H
