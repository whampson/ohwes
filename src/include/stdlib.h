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
 *         File: include/stdlib.h
 *      Created: April 11, 2025
 *       Author: Wes Hampson
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdlib.h.html
 * =============================================================================
 */

#ifndef __STDLIB_H
#define __STDLIB_H

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *)0)
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

long strtol(const char *restrict str, char **restrict str_end, int base);
long long strtoll(const char *restrict str, char **restrict str_end, int base);
unsigned long strtoul(const char *restrict str, char **restrict str_end, int base);
unsigned long long strtoull(const char *restrict str, char **restrict str_end, int base);

#endif // __STDLIB_H
