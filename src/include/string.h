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
 *         File: include/string.h
 *      Created: December 13, 2020
 *       Author: Wes Hampson
 *       Module: C Standard Library (C99)
 * =============================================================================
 */

/**
 * Status: INCOMPLETE
 * https://en.cppreference.com/w/c/string/byte
 */

#ifndef __STRING_H
#define __STRING_H

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __typeof__(sizeof(int)) size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void*)0)
#endif

void * memmove(void *dest, const void *src, size_t n);
void * memcpy(void *dest, const void *src, size_t n);
char * strcpy(char *dest, const char *src);
char * strncpy(char *dest, const char *src, size_t n);

// char * strcat(char *dst, const char *src);
// char * strncat(char *dst, const char *src, size_t n);

int memcmp(const void *ptr1, const void *ptr2, size_t n);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);

// int strcoll(const char *str1, const char *str2);
// int strxfrm(char *dest, const char *src, size_t n);

// void * memchr(const void *ptr, int value, size_t n);
// char * strchr(const char *str, int ch);
// char * strrchr(const char *str, int ch);
// size_t strspn(const char *str1, const char *str2);
// size_t strcspn(const char *str1, const char *str2);
// char * strpbrk(const char *str1, const char *str2);
// char * strstr(const char *str1, const char *str2);
// char * strtok(char *str, const char *delim);

void * memset(void *dest, int c, size_t n);

size_t strlen(const char *str);

// char * strerror(int errnum);

#endif /* __STRING_H */
