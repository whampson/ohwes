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
 *         File: src/include/stdarg.h
 *      Created: December 29, 2023
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/variadic
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdarg.h.html
 * =============================================================================
 */

#ifndef __STDARG_H
#define __STDARG_H

#ifndef __VA_LIST_DEFINED
#define __VA_LIST_DEFINED
typedef __builtin_va_list va_list;
#endif

#define va_start(list, param)   __builtin_va_start(list, param)
#define va_end(list)            __builtin_va_end(list)
#define va_arg(list, type)      __builtin_va_arg(list, type)
#define va_copy(dest, src)      __builtin_va_copy(dest, src)

#endif // __STDARG_H
