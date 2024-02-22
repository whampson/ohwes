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

// https://stanislavs.org/helppc/8042.html
// https://stanislavs.org/helppc/keyboard_commands.html
// https://www.tayloredge.com/reference/Interface/atkeyboard.pdf

#include <ps2.h>
#include <io.h>
#include <ohwes.h>

#define NUM_RETRIES 10000000

static void wait_for_read(void)
{
    int count = 0;
    while (count++ < NUM_RETRIES && !ps2_canread()) { }
    if (count >= NUM_RETRIES) {
        panic("timed out waiting for PS/2 controller read");
    }
}

static void wait_for_write(void)
{
    int count = 0;
    while (count++ < NUM_RETRIES && !ps2_canwrite()) { }
    if (count >= NUM_RETRIES) {
        panic("timed out waiting for PS/2 controller write");
    }
}

void init_ps2(void)
{
    uint8_t cfg;
    bool dualchan;

    //
    // disable ports and flush output buffer
    //
    ps2_cmd(PS2_CMD_P1OFF);
    ps2_cmd(PS2_CMD_P2OFF);
    ps2_flush();

    //
    // test for the existence of port 2
    //
    ps2_cmd(PS2_CMD_P2ON);
    ps2_cmd(PS2_CMD_RDCFG);
    cfg = ps2_read();
    dualchan = !has_flag(cfg, PS2_CFG_P2CLKOFF);
    if (dualchan) {
        ps2_cmd(PS2_CMD_P2OFF);
    }
    else {
        kprint("ps2: no mouse port on controller\n");
    }

    //
    // run self tests
    //
    if (!ps2_test()) panic("PS/2 controller self-test failed!");
    if (!ps2_testp1()) panic("PS/2 controller port 1 self-test failed!");           // TODO: fails on real hardware
    if (dualchan) {
        if (!ps2_testp2()) panic("PS/2 controller port 2 self-test failed!");
    }

    //
    // enable PS/2 device interrupts and disable scancode translation
    //
    cfg |= PS2_CFG_P1INTON;
    if (dualchan) {
        cfg |= PS2_CFG_P2INTON;
    }
    cfg &= ~PS2_CFG_XLATON;
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_write(cfg);

    //
    // enable PS/2 ports
    //
    ps2_cmd(PS2_CMD_P1ON);
    if (dualchan) {
        ps2_cmd(PS2_CMD_P2ON);
    }
}

void ps2_flush(void)
{
    do {
        inb_delay(PS2_PORT_DATA);
    } while (ps2_canread());
}

uint8_t ps2_status(void)
{
    wait_for_read();
    return inb_delay(PS2_PORT_STATUS);
}

bool ps2_test(void)
{
    ps2_cmd(PS2_CMD_TEST);
    uint8_t resp = ps2_read();

    return resp == PS2_RESP_PASS;
}

bool ps2_testp1(void)
{
    ps2_cmd(PS2_CMD_P1TEST);
    uint8_t resp = ps2_read();

    return resp == PS2_RESP_PASS || resp == PS2_RESP_P1PASS;
}

bool ps2_testp2(void)
{
    ps2_cmd(PS2_CMD_P2TEST);
    uint8_t resp = ps2_read();

    return resp == PS2_RESP_PASS || resp == PS2_RESP_P2PASS;
}

void ps2_cmd(uint8_t cmd)
{
    wait_for_write();
    outb_delay(PS2_PORT_CMD, cmd);
}

uint8_t ps2_read(void)
{
    wait_for_read();
    return inb_delay(PS2_PORT_DATA);
}

void ps2_write(uint8_t data)
{
    wait_for_write();
    outb_delay(PS2_PORT_DATA, data);
}

bool ps2_canread(void)
{
    // device output buffer must be full
    return has_flag(inb_delay(PS2_PORT_STATUS), PS2_STATUS_OUTPUT);
}

bool ps2_canwrite(void)
{
    // device input buffer must be empty
    return !has_flag(inb_delay(PS2_PORT_STATUS), PS2_STATUS_INPUT);
}
