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
 *    File: include/ohwes/except.h                                            *
 * Created: December 21, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __EXCEPT_H
#define __EXCEPT_H

#define NUM_EXCEPT  32
#define EXCEPT_DE   0x00    /* Divide Error */
#define EXCEPT_DB   0x01    /* Debug */
#define EXCEPT_NMI  0x02    /* Non-Maskable Interrupt */
#define EXCEPT_BP   0x03    /* Breakpoint */
#define EXCEPT_OF   0x04    /* Overflow */
#define EXCEPT_BR   0x05    /* BOUND Range Exceeded */
#define EXCEPT_UD   0x06    /* Invalid Opcode */
#define EXCEPT_NM   0x07    /* Device Not Available */
#define EXCEPT_DF   0x08    /* Double Fault */
#define EXCEPT_TS   0x0A    /* Invalid TSS */
#define EXCEPT_NP   0x0B    /* Segment Not Present */
#define EXCEPT_SS   0x0C    /* Stack Fault */
#define EXCEPT_GP   0x0D    /* General Protection Fault */
#define EXCEPT_PF   0x0E    /* Page Fault */
#define EXCEPT_MF   0x10    /* x87 FPU Foating-Point Error */
#define EXCEPT_AC   0x11    /* Alignment Check Exception */
#define EXCEPT_MC   0x12    /* Machine Check Exception */
#define EXCEPT_XM   0x13    /* SIMD Floating-Point Exception */
#define EXCEPT_VE   0x14    /* Virtualization Exception */
#define EXCEPT_CP   0x15    /* Control Protection Exception */

#endif /* __EXCEPT_H */
