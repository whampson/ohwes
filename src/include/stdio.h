/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: include/stdio.h
 *      Created: January 3, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __STDIO_H
#define __STDIO_H

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __typeof__(sizeof(int)) size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void*)0)
#endif

#ifndef __VA_LIST_DEFINED
#define __VA_LIST_DEFINED
typedef void *va_list;
#endif

int putchar(int c);
int puts(const char *str);

int printf(const char *format, ...);
int sprintf(char *buffer, const char *format, ...);
int snprintf(char *buffer, size_t bufsz, const char *format, ...);

int vprintf(const char *format, va_list args);

#endif // __STDIO_H
