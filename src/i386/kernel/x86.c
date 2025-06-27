/* =============================================================================
 * Copyright (C) 2023-2024 Wes Hampson. All Rights Reserved.
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
 *         File: src/i386/kernel/x86.c
 *      Created: May 13, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/x86.h>


void make_seg_desc(struct x86_desc *desc, int dpl, int base, int limit, int type)
{
    desc->_value = 0;
    desc->seg.type = type;
    desc->seg.dpl = dpl;
    desc->seg.s = 1;       // 1 = memory descriptor (code/data)
    desc->seg.db = 1;      // 1 = 32-bit
    desc->seg.baselo = ((base) & 0x00FFFFFF);
    desc->seg.basehi = ((base) & 0xFF000000) >> 24;
    desc->seg.limitlo = ((limit) & 0x0FFFF);
    desc->seg.limithi = ((limit) & 0xF0000) >> 16;
    desc->seg.g = 1;       // 1 = 4K page granularity
    desc->seg.p = 1;       // 1 = present in memory
}

void make_ldt_desc(struct x86_desc *desc, int dpl, int base, int limit)
{
    desc->_value = 0;
    desc->seg.type = DESCTYPE_LDT;
    desc->seg.dpl = dpl;
    desc->seg.s = 0;       // 0 = system descriptor
    desc->seg.db = 1;      // 1 = 32-bit
    desc->seg.baselo = ((base) & 0x00FFFFFF);
    desc->seg.basehi = ((base) & 0xFF000000) >> 24;
    desc->seg.limitlo = ((limit) & 0x0FFFF);
    desc->seg.limithi = ((limit) & 0xF0000) >> 16;
    desc->seg.g = 0;       // 0 = byte granularity
    desc->seg.p = 1;       // 1 = present in memory
}

void make_tss_desc(struct x86_desc *desc, int dpl, struct tss *base)
{
    desc->_value = 0;
    desc->tss.type = DESCTYPE_TSS32;
    desc->tss.dpl = dpl;
    desc->tss.baselo = (((uint32_t) base) & 0x00FFFFFF);
    desc->tss.basehi = (((uint32_t) base) & 0xFF000000) >> 24;
    desc->tss.limitlo = ((TSS_SIZE-1) & 0x0FFFF);
    desc->tss.limithi = ((TSS_SIZE-1) & 0xF0000) >> 16;
    desc->tss.g = 0;    // 0 = byte granularity
    desc->tss.p = 1;    // 1 = present in memory
}

void make_task_gate(struct x86_desc *desc, int tss_segsel, int dpl)
{
    desc->_value = 0;
    desc->task.type = DESCTYPE_TASK;
    desc->task.segsel = tss_segsel;
    desc->task.dpl = dpl;
    desc->task.p = 1;  // 1 = present in memory
}

void make_call_gate(struct x86_desc *desc, int segsel, int dpl, int num_params, void *handler)
{
    desc->_value = 0;
    desc->call.type = DESCTYPE_CALL32;
    desc->call.segsel = segsel;
    desc->call.dpl = dpl;
    desc->call.num_params = num_params;
    desc->call.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->call.offsethi = ((uint32_t) handler) >> 16;
    desc->call.p = (handler != NULL);
}

void make_intr_gate(struct x86_desc *desc, int segsel, int dpl, void *handler)
{
    desc->_value = 0;
    desc->intr.type = DESCTYPE_INTR32;
    desc->intr.segsel = segsel;
    desc->intr.dpl = dpl;
    desc->intr.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->intr.offsethi = ((uint32_t) handler) >> 16;
    desc->intr.p = (handler != NULL);
}

void make_trap_gate(struct x86_desc *desc, int segsel, int dpl, void *handler)
{
    desc->_value = 0;
    desc->trap.type = DESCTYPE_TRAP32;
    desc->trap.segsel = segsel;
    desc->trap.dpl = dpl;
    desc->trap.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->trap.offsethi = ((uint32_t) handler) >> 16;
    desc->trap.p = (handler != NULL);
}
