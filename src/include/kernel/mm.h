/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 *         File: src/include/kernel/mm.h
 *      Created: June 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __MM_H
#define __MM_H

#include <stdint.h>

// check whether reading or writing a virtual address would cause a page fault
bool virt_addr_valid(void *va);

// walk the page table and return the PTE pointed to by the virtual address
bool walk_page_table(uint32_t va, pte_t **pte);

// TODO: virt_to_phys

// linker script symbols
extern char __kernel_start[], __kernel_end[], __kernel_size[];
extern char __setup_start[], __setup_end[], __setup_size[];
extern char __text_start[], __text_end[], __text_size[];
extern char __data_start[], __data_end[], __data_size[];
extern char __rodata_start[], __rodata_end[], __rodata_size[];
extern char __bss_start[], __bss_end[], __bss_size[];

#endif  // __MM_H
