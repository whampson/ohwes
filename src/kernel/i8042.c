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

#include <ps2.h>
#include <io.h>
#include <ohwes.h>

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
    cfg = ps2_read();
    cfg &= ~PS2_CFG_P1INTON;
    cfg &= ~PS2_CFG_P2INTON;
    cfg &= ~PS2_CFG_XLATON;
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_write(cfg);
}

void ps2_flush(void)
{
    do {
        (void) inb_delay(PS2_PORT_DATA);
    } while (ps2_canread());
}

uint8_t ps2_status(void)
{
    return inb_delay(PS2_PORT_STS);
}

bool ps2_testctl(void)
{
    ps2_cmd(PS2_CMD_TEST);
    return ps2_read() == PS2_RES_PASS;
}

bool ps2_testp1(void)
{
    ps2_cmd(PS2_CMD_P1TEST);
    return ps2_read() == PS2_RES_P1PASS;
}

bool ps2_testp2(void)
{
    ps2_cmd(PS2_CMD_P2TEST);
    return ps2_read() == PS2_RES_P2PASS;
}

void ps2_cmd(uint8_t cmd)
{
    while (!ps2_canwrite()) { }
    outb_delay(PS2_PORT_CMD, cmd);
}

uint8_t ps2_read(void)
{
    while (!ps2_canread()) { }
    return inb_delay(PS2_PORT_DATA);
}

bool ps2_canread(void)
{
    volatile uint8_t sts;

    /* device output buffer must be full */
    return has_flag(sts = inb_delay(PS2_PORT_STS), PS2_STS_OUTPUT);
}

void ps2_write(uint8_t data)
{
    while (!ps2_canwrite()) { }
    outb_delay(PS2_PORT_DATA, data);
}

bool ps2_canwrite(void)
{
    volatile uint8_t sts;

    /* device input buffer must be empty */
    return !has_flag(sts = inb_delay(PS2_PORT_STS), PS2_STS_INPUT);
}
