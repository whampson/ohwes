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
 *         File: src/include/stdio.h
 *      Created: January 3, 2024
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/io
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdio.h.html
 * =============================================================================
 */

#ifndef __STDIO_H
#define __STDIO_H

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *)0)
#endif

#ifndef __VA_LIST_DEFINED
#define __VA_LIST_DEFINED
typedef __builtin_va_list va_list;
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#define EOF (-1)

int putchar(int c);
int puts(const char *str);

int printf(const char *format, ...);
int sprintf(char *buffer, const char *format, ...);
int snprintf(char *buffer, size_t bufsz, const char *format, ...);

int vprintf(const char *format, va_list args);
int vsprintf(char *buffer, const char *format, va_list args);
int vsnprintf(char *buffer, size_t bufsz, const char *format, va_list args);

void perror(const char *s);

#endif // __STDIO_H
