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
 *    File: include/nb/boot.h                                                 *
 * Created: December 11, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __BOOT_H
#define __BOOT_H

#define BOOT_SEG        0x0000      /* boot ldr begins at 0000:7C00 = 0x07C00 */
#define INIT_SEG        0x0900      /* INIT.SYS begins at 0900:0000 = 0x09000 */
#define KERN_SEG        0x1000      /* NBOS.SYS begins at 1000:0000 = 0x10000 */

#define BOOT_BASE       0x7C00
#define INIT_BASE       (INIT_SEG<<4)
#define KERN_BASE       (KERN_SEG<<4)

#define BOOT_STACK      (BOOT_BASE)
#define KERN_STACK      (KERN_BASE)

#define BOOT_ENTRY      (BOOT_BASE)
#define INIT_ENTRY      (INIT_BASE)
#define KERN_ENTRY      (KERN_BASE)

#endif /* __BOOT_H */
