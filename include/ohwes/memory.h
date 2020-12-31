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
 *    File: include/ohwes/memory.h                                            *
 * Created: December 19, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __MEMORY_H
#define __MEMORY_H

/* Minimum memory required to run OHWES. */
#define MIN_KB          4096

#define KB_SHIFT        10
#define KB              (1<<KB_SHIFT)
#define MB_SHIFT        20
#define MB              (1<<MB_SHIFT)
#define GB_SHIFT        30
#define GB              (1<<GB_SHIFT)

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1<<PAGE_SHIFT)
#define LG_PAGE_SHIFT   22
#define LG_PAGE_SIZE    (1<<LG_PAGE_SHIFT)

#define PGDIR           0x2000
#define PGTBL0          0x3000

#endif /* __MEMROY_H */
