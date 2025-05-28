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
extern uint32_t _kernel_start, _kernel_end, _kernel_size;
extern uint32_t _setup_start, _setup_end, _setup_size;
extern uint32_t _text_start, _text_end, _text_size;
extern uint32_t _data_start, _data_end, _data_size;
extern uint32_t _rodata_start, _rodata_end, _rodata_size;
extern uint32_t _bss_start, _bss_end, _bss_size;

#endif  // __MM_H
