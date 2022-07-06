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
 *    File: include/ohwes/thunk.h                                             *
 * Created: December 21, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __THUNK_H
#define __THUNK_H

typedef void (*ivt_thunk)(void);

#define IVT_THUNK_PROTO(thunk_name) \
extern void thunk_name(void);

IVT_THUNK_PROTO(thunk_except_00)
IVT_THUNK_PROTO(thunk_except_01)
IVT_THUNK_PROTO(thunk_except_02)
IVT_THUNK_PROTO(thunk_except_03)
IVT_THUNK_PROTO(thunk_except_04)
IVT_THUNK_PROTO(thunk_except_05)
IVT_THUNK_PROTO(thunk_except_06)
IVT_THUNK_PROTO(thunk_except_07)
IVT_THUNK_PROTO(thunk_except_08)
IVT_THUNK_PROTO(thunk_except_09)
IVT_THUNK_PROTO(thunk_except_10)
IVT_THUNK_PROTO(thunk_except_11)
IVT_THUNK_PROTO(thunk_except_12)
IVT_THUNK_PROTO(thunk_except_13)
IVT_THUNK_PROTO(thunk_except_14)
IVT_THUNK_PROTO(thunk_except_15)
IVT_THUNK_PROTO(thunk_except_16)
IVT_THUNK_PROTO(thunk_except_17)
IVT_THUNK_PROTO(thunk_except_18)
IVT_THUNK_PROTO(thunk_except_19)
IVT_THUNK_PROTO(thunk_except_20)
IVT_THUNK_PROTO(thunk_except_21)
IVT_THUNK_PROTO(thunk_except_22)
IVT_THUNK_PROTO(thunk_except_23)
IVT_THUNK_PROTO(thunk_except_24)
IVT_THUNK_PROTO(thunk_except_25)
IVT_THUNK_PROTO(thunk_except_26)
IVT_THUNK_PROTO(thunk_except_27)
IVT_THUNK_PROTO(thunk_except_28)
IVT_THUNK_PROTO(thunk_except_29)
IVT_THUNK_PROTO(thunk_except_30)
IVT_THUNK_PROTO(thunk_except_31)
IVT_THUNK_PROTO(thunk_irq_00)
IVT_THUNK_PROTO(thunk_irq_01)
IVT_THUNK_PROTO(thunk_irq_02)
IVT_THUNK_PROTO(thunk_irq_03)
IVT_THUNK_PROTO(thunk_irq_04)
IVT_THUNK_PROTO(thunk_irq_05)
IVT_THUNK_PROTO(thunk_irq_06)
IVT_THUNK_PROTO(thunk_irq_07)
IVT_THUNK_PROTO(thunk_irq_08)
IVT_THUNK_PROTO(thunk_irq_09)
IVT_THUNK_PROTO(thunk_irq_10)
IVT_THUNK_PROTO(thunk_irq_11)
IVT_THUNK_PROTO(thunk_irq_12)
IVT_THUNK_PROTO(thunk_irq_13)
IVT_THUNK_PROTO(thunk_irq_14)
IVT_THUNK_PROTO(thunk_irq_15)
IVT_THUNK_PROTO(thunk_syscall)

#endif /* __THUNK_H */
