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
 *         File: src/include/string.h
 *      Created: January 3, 2024
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/string/byte
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/string.h.html
 * =============================================================================
 */

#ifndef __STRING_H
#define __STRING_H

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *) 0)
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

void * memcpy(void *restrict dst, const void *restrict src, size_t count);
void * mempcpy(void *restrict dst, const void *restrict src, size_t count);     // nonstandard

void * memmove(void *dst, const void *src, size_t count);
void * memset(void *dst, int c, size_t count);

int memcmp(const void *lhs, const void *rhs, size_t count);

char * strcpy(char *restrict dst, const char *restrict src);
char * stpcpy(char *restrict dst, const char *restrict src);                    // nonstandard

char * strncpy(char *restrict dst, const char *restrict src, size_t count);
char * stpncpy(char *restrict dst, const char *restrict src, size_t count);     // nonstandard

char * strcat(char *restrict dst, const char *restrict src);
char * strncat(char *restrict dst, const char *restrict src, size_t count);

size_t strlen(const char *str);
size_t strnlen(const char *str, size_t maxlen);

int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t count);

// char * strchr(const char *str, const char *substr);

char * strerror(int errnum);

#endif // __STRING_H
