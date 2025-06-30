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

// linker script symbols -- use operator& to get assigned value
extern uint32_t __kernel_start, __kernel_end, __kernel_size;
extern uint32_t __setup_start, __setup_end, __setup_size;
extern uint32_t __text_start, __text_end, __text_size;
extern uint32_t __data_start, __data_end, __data_size;
extern uint32_t __rodata_start, __rodata_end, __rodata_size;
extern uint32_t __bss_start, __bss_end, __bss_size;

#endif  // __MM_H
