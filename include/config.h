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

//
// static memory
//

#define ZERO_PAGE                       0x0000
#define SYSTEM_CPU_PAGE                 0x1000
#define SYSTEM_PAGE_DIRECTORY           0x2000
#define SYSTEM_MEMORY_PAGE              0x3000
#define KERNEL_PAGE_TABLE               0x4000
#define KERNEL_STACK_PAGE               0xF000
#define SYSTEM_DMA_AREA                 0x10000 // 0x10000-0x1FFFF
#define KERNEL_BASE                     0x20000 // 0x20000-????
#define KERNEL_ENTRY                    KERNEL_BASE
#define USER_STACK_PAGE                 0x90000 // 0x90000-0x90FFF
#define SYSTEM_FRAME_BUFFER             0xB8000
// TODO: map zero page physical contents elsewhere (for reboot param, etc.)

//
// x86 descriptors
//

// GDT
#define _GDT_NULL                       0
#define _GDT_LOCALDESC                  (0x08|KERNEL_PL)
#define _GDT_KERNEL_CS                  (0x10|KERNEL_PL)
#define _GDT_KERNEL_DS                  (0x18|KERNEL_PL)
#define _GDT_USER_CS                    (0x20|USER_PL)
#define _GDT_USER_DS                    (0x28|USER_PL)
#define _GDT_TASKSTATE                  (0x30|KERNEL_PL)

// GDT segment selectors
#define KERNEL_CS                       _GDT_KERNEL_CS
#define KERNEL_DS                       _GDT_KERNEL_DS
#define KERNEL_SS                       KERNEL_DS
#define USER_CS                         _GDT_USER_CS
#define USER_DS                         _GDT_USER_DS
#define USER_SS                         USER_DS

#endif // __CONFIG_H
