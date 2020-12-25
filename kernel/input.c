/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/input.c                                                    *
 * Created: December 24, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <ohwes/kernel.h>
#include <ohwes/input.h>
#include <ohwes/irq.h>
#include <drivers/ps2.h>

#define KBD_CMD_SETLED      0xED
#define KBD_CMD_SCANCODE    0xF0
#define KBD_CMD_SELFTEST    0xFF

#define KBD_RES_ACK         0xFA
#define KBD_RES_RESEND      0xFE

static void kbd_on(void);
static void kbd_sc3(void);
static void kbd_irq(void);
static void kbd_cmd(uint8_t cmd, uint8_t *data, size_t n);

void kbd_init(void)
{
    ps2_init();
    kbd_on();
    kbd_sc3();

    irq_register_handler(IRQ_KEYBOARD, kbd_irq);
    irq_unmask(IRQ_KEYBOARD);
}

static void kbd_on(void)
{
    uint8_t ps2cfg;

    ps2_cmd(PS2_CMD_RDCFG);
    ps2cfg = ps2_inb();
    ps2cfg |= PS2_CFG_P1INTON;
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_outb(ps2cfg);
    ps2_cmd(PS2_CMD_P1ON);
}

static void kbd_sc3(void)
{
    uint8_t sc = 3;
    kbd_cmd(KBD_CMD_SCANCODE, &sc, 1);

    sc = 0;
    kbd_cmd(KBD_CMD_SCANCODE, &sc, 1);
    sc = ps2_inb();
    if (sc != 3) {
        panic("failed to set scancode 3!");
    }

    kbd_cmd(0xFA, NULL, 0); /* set all keys to typematic/autorepeat/make/release */
    kbd_cmd(0xF4, NULL, 0); /* enable scanning  */
}

static void kbd_irq(void)
{
    uint8_t sc;

    sc = ps2_inb();
    kprintf("%02X ", sc);
}

static void kbd_cmd(uint8_t cmd, uint8_t *data, size_t n)
{
    const int NUM_RETRIES = 3;
    uint8_t res;
    int i, r;

    /* TODO: this is ugly... */

    i = 0; r = 0;
    while (r++ < NUM_RETRIES)
    {
        ps2_outb(cmd);
        goto check_resp;
    send_data:
        if (i >= (int) n) break;
        ps2_outb(data[i++]);
    check_resp:
        switch (res = ps2_inb()) {
            case KBD_RES_ACK:
                goto send_data;
            case KBD_RES_RESEND:
                continue;
            default:
                panic("PS/2 keyboard unknown response 0x%02X\n", res);
                break;
        }
    }

    if (i == NUM_RETRIES) {
        panic("PS/2 keyboard command failed 0x%02X\n", cmd);
    }
}

