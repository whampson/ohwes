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
 *         File: include/stddef.h
 *      Created: December 29, 2023
 *       Author: Wes Hampson
 *
 * Common definitions.
 *
 * https://en.cppreference.com/w/c/types (C11)
 * =============================================================================
 */

#ifndef __STDDEF_H
#define __STDDEF_H

#ifndef __ASSEMBLER__

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void*)0)
#endif

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#define offsetof(type,member) ((size_t)((char*)&((type*)0)->member-(char*)0))

// OH-WES additions
#ifndef __SSIZE_T_DEFINED
#define __SSIZE_T_DEFINED
typedef signed int ssize_t;
#endif

#endif // __ASSEMBLER__

#endif // __STDDEF_H
