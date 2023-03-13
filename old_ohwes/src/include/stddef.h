/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/stddef.h                                                  *
 * Created: December 13, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Implementation of stddef.h from the C Standard Library.                    *
 *============================================================================*/

/* Completion Status: DONE */

#ifndef __STDDEF_H
#define __STDDEF_H

#include <stdint.h>

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef uint32_t size_t;
#endif

#ifndef __PTRDIFF_T_DEFINED
#define __PTRDIFF_T_DEFINED
typedef int32_t ptrdiff_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *) 0)
#endif

#define offsetof(type,member)   \
    ((size_t)((char *)&((type *)(0))->member-(char *)0))

typedef uint64_t max_align_t;

#endif /* __STDDEF_H */
