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
 *         File: include/pic.h
 *      Created: January 22, 2024
 *       Author: Wes Hampson
 *
 * Intel 8259A Programmable Interrupt Controller interface.
 * =============================================================================
 */

#ifndef __PIC_H
#define __PIC_H

#include <stdint.h>

#define PIC_MASK_ALL            0xFFFF

void pic_eoi(uint8_t irq_num);
void pic_mask_irq(uint8_t irq_num);
void pic_unmask_irq(uint8_t irq_num);

uint16_t pic_get_mask(void);
void pic_set_mask(uint16_t mask);

uint16_t pic_read_isr(void);
uint16_t pic_read_irr(void);

#endif /* __PIC_H */
