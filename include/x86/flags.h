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
 *    File: include/x86/flags.h                                               *
 * Created: December 22, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Flags register definitions for x86-family CPUs.                            *
 *============================================================================*/

#ifndef __X86_FLAGS_H
#define __X86_FLAGS_H

#define CF_SHIFT    0
#define PF_SHIFT    2
#define AF_SHIFT    4
#define ZF_SHIFT    6
#define SF_SHIFT    7
#define TF_SHIFT    8
#define IF_SHIFT    9
#define DF_SHIFT    10
#define OF_SHIFT    11
#define IOPL_SHIFT  12
#define NT_SHIFT    14
#define RF_SHIFT    16
#define VM_SHIFT    17
#define AC_SHIFT    18
#define VIF_SHIFT   19
#define VIP_SHIFT   20
#define ID_SHIFT    21

#define CF_MASK     (1 << CF_SHIFT)
#define PF_MASK     (1 << PF_SHIFT)
#define AF_MASK     (1 << AF_SHIFT)
#define ZF_MASK     (1 << ZF_SHIFT)
#define SF_MASK     (1 << SF_SHIFT)
#define TF_MASK     (1 << TF_SHIFT)
#define IF_MASK     (1 << IF_SHIFT)
#define DF_MASK     (1 << DF_SHIFT)
#define OF_MASK     (1 << OF_SHIFT)
#define IOPL_MASK   (3 << IOPL_SHIFT)
#define NT_MASK     (1 << NT_SHIFT)
#define RF_MASK     (1 << RF_SHIFT)
#define VM_MASK     (1 << VM_SHIFT)
#define AC_MASK     (1 << AC_SHIFT)
#define VIF_MASK    (1 << VIF_SHIFT)
#define VIP_MASK    (1 << VIP_SHIFT)
#define ID_MASK     (1 << ID_SHIFT)

#ifndef __ASSEMBLY__
#include <stdint.h>

struct eflags
{
    union {
        struct {
            uint32_t cf     : 1;    /* Carry Flag */
            uint32_t        : 1;    /* (reserved; set to 1) */
            uint32_t pf     : 1;    /* Parity Flag */
            uint32_t        : 1;    /* (reserved; set to 0) */
            uint32_t af     : 1;    /* Adjust Flag */
            uint32_t        : 1;    /* (reserved; set to 0) */
            uint32_t zf     : 1;    /* Zero Flag */
            uint32_t sf     : 1;    /* Sign Flag */
            uint32_t tf     : 1;    /* Trap Flag */
            uint32_t if_    : 1;    /* Interrupt Flag */
            uint32_t df     : 1;    /* Direction Flag */
            uint32_t of     : 1;    /* Overflow Flag */
            uint32_t iopl   : 2;    /* I/O Privilege Level */
            uint32_t nt     : 1;    /* Nested Task Flag */
            uint32_t        : 1;    /* (reserved; set to 0) */
            uint32_t rf     : 1;    /* Resume Flag */
            uint32_t vm     : 1;    /* Virtual-8086 Mode */
            uint32_t ac     : 1;    /* Alignment Check */
            uint32_t vif    : 1;    /* Virtual Interrupt Flag */
            uint32_t vip    : 1;    /* Virtual Interrupt Pending */
            uint32_t id     : 1;    /* Identification Flag */
            uint32_t        : 10;   /* (reserved; set to 0) */
        };
        uint32_t _value;
    };
};

#endif /* __ASSEMBLY__ */

#endif /* __X86_FLAGS_H */

