/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/stdint.h                                                  *
 * Created: December 11, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __STDINT_H
#define __STDINT_H

#define INT8_MIN                (-0x7F-1)
#define INT16_MIN               (-0x7FFF-1)
#define INT32_MIN               (-0x7FFFFFFF-1)
#define INT64_MIN               (-0x7FFFFFFFFFFFFFFFLL-1)
#define INTPTR_MIN              INT32_MIN
#define INTMAX_MIN              INT64_MIN
#define PTRDIFF_MIN             INT32_MIN

#define INT8_MAX                0x7F
#define INT16_MAX               0x7FFF
#define INT32_MAX               0x7FFFFFFF
#define INT64_MAX               0x7FFFFFFFFFFFFFFFLL
#define INTPTR_MAX              INT32_MAX
#define INTMAX_MAX              INT64_MAX
#define PTRDIFF_MAX             INT32_MAX

#define UINT8_MAX               0xFF
#define UINT16_MAX              0xFFFF
#define UINT32_MAX              0xFFFFFFFFU
#define UINT64_MAX              0xFFFFFFFFFFFFFFFFULL
#define UINTPTR_MAX             UINT32_MAX
#define UINTMAX_MAX             UINT64_MAX
#define SIZE_MAX                UINT32_MAX

typedef signed char             int8_t;
typedef signed short int        int16_t;
typedef signed long int         int32_t;
typedef signed long long int    int64_t;
typedef int32_t                 intptr_t;
typedef int64_t                 intmax_t;

typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned long int       uint32_t;
typedef unsigned long long int  uint64_t;
typedef uint32_t                uintptr_t;
typedef uint64_t                uintmax_t;

#endif /* __STDINT_H */
