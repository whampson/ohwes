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
 *    File: include/ohwes/boot.h                                              *
 * Created: December 11, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *
 * Real Mode boot loader macros.
 *============================================================================*/

#ifndef __BOOT_H
#define __BOOT_H

#define STAGE1_SEGMENT      0x0000
#define STAGE2_SEGMENT      0x0900
#define KERNEL_SEGMENT      0x1000

#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         (STAGE2_SEGMENT<<4)
#define KERNEL_BASE_EARLY   (KERNEL_SEGMENT<<4)

#define STAGE1_ENTRY        (STAGE1_BASE)
#define STAGE2_ENTRY        (STAGE2_BASE)

#define BOOT_STACK          0x7C00

#endif /* __BOOT_H */
