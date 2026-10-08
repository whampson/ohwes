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
 *         File: src/i386/kernel/pic.c
 *      Created: January 22, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/irq.h>

#define PIC_M           0   // master
#define PIC_S           1   // slave

#define PIC1_CMD_PORT   0x20
#define PIC1_DATA_PORT  0x21
#define PIC2_CMD_PORT   0xA0
#define PIC2_DATA_PORT  0xA1

#define SLAVE_IRQ_MASK      (1<<(IRQ_SLAVE))

// Initialization Command Words (ICW)
#define ICW1            0x11                // edge-triggered, 8 byte vectors, cascade mode, ICW4 needed
#define ICW2_M          (IRQ_BASE_VECTOR)   // master PIC base interrupt vector
#define ICW2_S          (IRQ_BASE_VECTOR+8) // slave PIC base interrupt vector
#define ICW3_M          (SLAVE_IRQ_MASK)    // mask of slave IRQ line on master PIC
#define ICW3_S          (IRQ_SLAVE)         // slave PIC IRQ number to raise on master
#define ICW4            0x01                // not special fully nested, not auto EOI, 8086 mode

// Operation Command Words (OCW)
#define OCW1_MASK_ALL   0xFF            // mask all interrupt lines
#define OCW2_EOI        0x60            // specific EOI; needs IRQ num in bits[2:0]
#define OCW3_READ_IRR   0x0A            // read Interrupt Request Register
#define OCW3_READ_ISR   0x0B            // read In-Service Register


#define __PIC_CMD_PORT(p) ((p) == PIC_M) ? PIC1_CMD_PORT : PIC2_CMD_PORT
#define __PIC_DATA_PORT(p) ((p) == PIC_M) ? PIC1_DATA_PORT : PIC2_DATA_PORT

static inline void __pic_write_cmd(int pic, uint8_t cmd)
{
    outb(__PIC_CMD_PORT(pic), cmd);
}

static inline void __pic_write_cmd_slow(int pic, uint8_t cmd)
{
    outb_slow(__PIC_CMD_PORT(pic), cmd);
}

static inline void __pic_write_data(int pic, uint8_t data)
{
    outb(__PIC_DATA_PORT(pic), data);
}

static inline void __pic_write_data_slow(int pic, uint8_t data)
{
    outb_slow(__PIC_DATA_PORT(pic), data);
}

static inline uint8_t __pic_read_reg(int pic, uint8_t ocw3)
{
    outb(__PIC_CMD_PORT(pic), (ocw3 & 0xB) | 0x8);  // bit[3]: OCW3 ident
    return inb(__PIC_CMD_PORT(pic));                // intentionally reading CMD port
}

static inline uint8_t __pic_read_data(int pic)
{
    return inb(__PIC_DATA_PORT(pic));
}


void init_pic(void)
{
    uint32_t flags;
    cli_save(flags);

    // configure master PIC
    __pic_write_cmd_slow(PIC_M, ICW1);
    __pic_write_data_slow(PIC_M, ICW2_M);
    __pic_write_data_slow(PIC_M, ICW3_M);
    __pic_write_data_slow(PIC_M, ICW4);

    // configure slave PIC
    __pic_write_cmd_slow(PIC_S, ICW1);
    __pic_write_data_slow(PIC_S, ICW2_S);
    __pic_write_data_slow(PIC_S, ICW3_S);
    __pic_write_data_slow(PIC_S, ICW4);

    // mask all interrupts
    __pic_write_data(PIC_M, OCW1_MASK_ALL);
    __pic_write_data(PIC_S, OCW1_MASK_ALL);

    restore_flags(flags);
}

void pic_eoi(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    if (irq_num >= 8) {
        __pic_write_cmd(PIC_S, OCW2_EOI | (irq_num & 0x7));
        __pic_write_cmd(PIC_M, OCW2_EOI | IRQ_SLAVE);
    }
    else {
        __pic_write_cmd(PIC_M, OCW2_EOI | (irq_num & 0x7));
    }

    restore_flags(flags);
}

void pic_mask_irq(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t pic_num = (irq_num >= 8) ? PIC_S : PIC_M;
    uint8_t mask = (1 << (irq_num & 0x7));

    uint8_t ocw1 = __pic_read_data(pic_num);
    __pic_write_data(pic_num, ocw1 | mask);

    restore_flags(flags);
}

void pic_unmask_irq(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t pic_num = (irq_num >= 8) ? PIC_S : PIC_M;
    uint8_t mask = (1 << (irq_num & 0x7));

    uint8_t ocw1 = __pic_read_data(pic_num);
    __pic_write_data(pic_num, ocw1 & ~mask);

    if (pic_num == PIC_S) {
        __pic_write_data(PIC_M, __pic_read_data(PIC_M) & ~SLAVE_IRQ_MASK);
    }

    restore_flags(flags);
}

uint16_t pic_get_mask(void)
{
    uint32_t flags;
    cli_save(flags);

    uint16_t mask = (__pic_read_data(PIC_S) << 8) | __pic_read_data(PIC_M);

    restore_flags(flags);
    return mask;
}

void pic_set_mask(uint16_t mask)
{
    uint32_t flags;
    cli_save(flags);

    __pic_write_data(PIC_M, mask & 0xFF);
    __pic_write_data(PIC_S, (mask >> 8) & 0xFF);

    restore_flags(flags);
}

uint16_t pic_read_isr(void)
{
    uint32_t flags;
    cli_save(flags);

    uint16_t isr = (__pic_read_reg(PIC_S, OCW3_READ_ISR) << 8) |
                    __pic_read_reg(PIC_M, OCW3_READ_ISR);

    restore_flags(flags);
    return isr;
}

uint16_t pic_read_irr(void)
{
    uint32_t flags;
    cli_save(flags);

    uint16_t irr = (__pic_read_reg(PIC_S, OCW3_READ_IRR) << 8) |
                    __pic_read_reg(PIC_M, OCW3_READ_IRR);

    restore_flags(flags);
    return irr;
}
