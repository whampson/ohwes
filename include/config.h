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
 *         File: include/config.h
 *      Created: July 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define MIN_KB_REQUIRED         639     // let's see how long this lasts!
#define PRINT_MEMORY_MAP        1
#define PRINT_IOCTL             0
#define PRINT_PAGE_MAP          1
#define E9_HACK                 1

#define BOOT_MEMMAP             0x1000
#define KERNEL_PGDIR            0x2000
#define KERNEL_PGTBL            0x3000
#define KERNEL_LMA              0x10000     // physical load address
#define KERNEL_INIT_STACK       KERNEL_LMA  // grows toward 0

#define PAGE_OFFSET             0xC0000000

#endif // __CONFIG_H
