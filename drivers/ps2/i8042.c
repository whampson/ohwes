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
 *    File: drivers/ps2/i8042.c                                               *
 * Created: December 24, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <drivers/ps2.h>
#include <ohwes/ohwes.h>
#include <ohwes/io.h>
#include <ohwes/interrupt.h>
#include <ohwes/kernel.h>

/* TOOD: watchdog timer */

void ps2_init(void)
{
    uint8_t cfg;

    /* disable ports and flush output buffer */
    ps2_cmd(PS2_CMD_P1OFF);
    ps2_cmd(PS2_CMD_P2OFF);
    ps2_flush();

    /* disable interrupts and scancode translaton */
    ps2_cmd(PS2_CMD_RDCFG);
    cfg = ps2_inb();
    cfg &= ~PS2_CFG_P1INTON;
    cfg &= ~PS2_CFG_P2INTON;
    cfg &= ~PS2_CFG_XLATON;
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_outb(cfg);

    /* test controller and ports */
    ps2_cmd(PS2_CMD_TEST);
    if (ps2_inb() != PS2_TEST_PASS) {
        panic("PS/2 controller self-test failed!");
    }
    ps2_cmd(PS2_CMD_P1TEST);
    if (ps2_inb() != PS2_P1TEST_PASS) {
        panic("PS/2 controller port 1 self-test failed!");
    }
    ps2_cmd(PS2_CMD_P2TEST);
    if (ps2_inb() != PS2_P2TEST_PASS) {
        panic("PS/2 controller port 2 self-test failed!");
    }
}

void ps2_flush(void)
{
    do {
        (void) inb(PS2_PORT_DATA);
    } while (has_flag(inb(PS2_PORT_STS), PS2_STS_OUTPUT));
}

uint8_t ps2_status(void)
{
    return inb(PS2_PORT_STS);
}

void ps2_cmd(uint8_t cmd)
{
    volatile uint8_t sts;

    while (has_flag(sts = inb(PS2_PORT_STS), PS2_STS_INPUT)) { }
    outb(PS2_PORT_CMD, cmd);
}

uint8_t ps2_inb(void)
{
    volatile uint8_t sts;

    while (!has_flag(sts = inb(PS2_PORT_STS), PS2_STS_OUTPUT)) { }
    return inb(PS2_PORT_DATA);
}

void ps2_outb(uint8_t data)
{
    volatile uint8_t sts;

    while (has_flag(sts = inb(PS2_PORT_STS), PS2_STS_INPUT)) { }
    outb(PS2_PORT_DATA, data);
}
