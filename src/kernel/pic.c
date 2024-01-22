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
 *         File: kernel/pic.c
 *      Created: January 22, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <interrupt.h>
#include <irq.h>
#include <pic.h>
#include <x86.h>

#define PARANOID        1

#define MASTER_PIC      0
#define SLAVE_PIC       1

#define SLAVE_MASK      (1<<(IRQ_SLAVE))

// Initialization Command Words (ICW)
#define ICW1            0x11                // edge-triggered, 8 byte vectors, cascade mode, ICW4 needed
#define ICW2_M          (IVT_DEVICEIRQ)     // master PIC base interrupt vector
#define ICW2_S          (IVT_DEVICEIRQ+8)   // slave PIC base interrupt vector
#define ICW3_M          (SLAVE_MASK)        // mask of slave IRQ line on master PIC
#define ICW3_S          (IRQ_SLAVE)         // slave IRQ number, to be sent to master
#define ICW4            0x01                // not special fully nested, not auto EOI, 8086 mode

// Operation Command Words (OCW)
#define OCW1_MASK_ALL   0xFF            // mask all interrupt lines
#define OCW2_EOI        0x60            // specific EOI; needs IRQ num in bits 2-0

static void pic_write_cmd(int pic, uint8_t cmd);
static void pic_write_data(int pic, uint8_t data);
static uint8_t pic_read_data(int pic);

void init_pic(void)
{
    // configure master PIC
    pic_write_cmd(MASTER_PIC, ICW1);
    pic_write_data(MASTER_PIC, ICW2_M);
    pic_write_data(MASTER_PIC, ICW3_M);
    pic_write_data(MASTER_PIC, ICW4);

    // configure slave PIC
    pic_write_cmd(SLAVE_PIC, ICW1);
    pic_write_data(SLAVE_PIC, ICW2_S);
    pic_write_data(SLAVE_PIC, ICW3_S);
    pic_write_data(SLAVE_PIC, ICW4);

    // mask all IRQs, except slave line on master PIC
    pic_write_data(MASTER_PIC, OCW1_MASK_ALL & ~SLAVE_MASK);
    pic_write_data(SLAVE_PIC, OCW1_MASK_ALL);
}

void pic_eoi(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    if (irq_num >= 8) {
        pic_write_cmd(SLAVE_PIC, OCW2_EOI | (irq_num & 0x7));
        pic_write_cmd(MASTER_PIC, OCW2_EOI | IRQ_SLAVE);
    }
    else {
        pic_write_cmd(MASTER_PIC, OCW2_EOI | irq_num);
    }

    restore_flags(flags);
}

void pic_mask(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t pic_num = (irq_num >= 8);
    uint8_t mask = (1 << (irq_num & 0x7));

    uint8_t ocw1 = pic_read_data(pic_num);
    ocw1 |= mask;
    pic_write_data(pic_num, ocw1);

#ifdef PARANOID
    uint8_t ocw1_readback = pic_read_data(pic_num);
    (void) ocw1_readback;
    assert(ocw1 == ocw1_readback);
#endif

    restore_flags(flags);
}

void pic_unmask(uint8_t irq_num)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t pic_num = (irq_num >= 8);
    uint8_t mask = (1 << (irq_num & 0x7));

    uint8_t ocw1 = pic_read_data(pic_num);
    ocw1 &= ~mask;
    pic_write_data(pic_num, ocw1);

#ifdef PARANOID
    uint8_t ocw1_readback = pic_read_data(pic_num);
    (void) ocw1_readback;
    assert(ocw1 == ocw1_readback);
#endif

    restore_flags(flags);
}

uint16_t pic_getmask(void)
{
    uint8_t ocw1_m = pic_read_data(MASTER_PIC);
    uint8_t ocw1_s = pic_read_data(SLAVE_PIC);

    return (ocw1_s << 8) | ocw1_m;
}

static void pic_write_cmd(int pic, uint8_t cmd)
{
    uint16_t port = (pic == MASTER_PIC)
        ? PIC_MASTER_CMD_PORT
        : PIC_SLAVE_CMD_PORT;

    outb_delay(port, cmd);
}

static void pic_write_data(int pic, uint8_t data)
{
    uint16_t port = (pic == MASTER_PIC)
        ? PIC_MASTER_DATA_PORT
        : PIC_SLAVE_DATA_PORT;

    outb_delay(port, data);
}

static uint8_t pic_read_data(int pic)
{
    uint16_t port = (pic == MASTER_PIC)
        ? PIC_MASTER_DATA_PORT
        : PIC_SLAVE_DATA_PORT;

    return inb_delay(port);
}
