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
 *         File: include/ohwes.h
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

//
// kernel config
//

#define MIN_KB_REQUIRED                 639     // let's see how long this lasts!
#define SHOW_MEMMAP                     1
#define PRINT_IOCTL                     0

#define KERNEL_BASE                     0x20000
#define KERNEL_ENTRY                    KERNEL_BASE
#define KERNEL_STACK_DEFAULT            0x9FC00

// GDT
#define _GDT_NULL                       0
#define _GDT_LOCALDESC                  (0x08|KERNEL_PL)
#define _GDT_KERNEL_CS                  (0x10|KERNEL_PL)
#define _GDT_KERNEL_DS                  (0x18|KERNEL_PL)
#define _GDT_USER_CS                    (0x20|USER_PL)
#define _GDT_USER_DS                    (0x28|USER_PL)
#define _GDT_TASKSTATE                  (0x30|KERNEL_PL)

#define KERNEL_CS                       _GDT_KERNEL_CS
#define KERNEL_DS                       _GDT_KERNEL_DS
#define KERNEL_SS                       KERNEL_DS
#define USER_CS                         _GDT_USER_CS
#define USER_DS                         _GDT_USER_DS
#define USER_SS                         USER_DS


#endif // __CONFIG_H
