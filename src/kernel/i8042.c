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

#include <boot.h>
#include <ps2.h>
#include <io.h>
#include <ohwes.h>

#define NUM_RETRIES 100000

static void wait_for_read(void)
{
    for (int i = 0; i < NUM_RETRIES; i++) {
        if (ps2_canread()) {
            return;
        }
    }
    panic("timed out waiting for PS/2 controller read! (%d tries)", NUM_RETRIES);
}

static void wait_for_write(void)
{
    for (int i = 0; i < NUM_RETRIES; i++) {
        if (ps2_canwrite()) {
            return;
        }
    }
    panic("timed out waiting for PS/2 controller write! (%d tries)", NUM_RETRIES);
}

void init_ps2(const struct bootinfo * const info)
{
    uint32_t flags;
    uint8_t cfg, resp;
    bool port2;

    cli_save(flags);

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
    port2 = !has_flag(cfg, PS2_CFG_P2CLKOFF) && info->hwflags.has_ps2mouse;
    if (!port2) {
        kprint("ps2ctl: no mouse port detected on PS/2 controller\n");
    }

    //
    // run self tests
    //
    ps2_cmd(PS2_CMD_TEST);
    resp = ps2_read();
    if (resp != PS2_RESP_PASS) {
        panic("PS/2 controller self-test failed!");
    }

    ps2_cmd(PS2_CMD_P1TEST);
    resp = ps2_read();
    if (resp != PS2_RESP_PASS && resp != PS2_RESP_P1PASS) {
        panic("PS/2 controller port 1 self-test failed!");
    }

    if (port2) {
        ps2_cmd(PS2_CMD_P2TEST);
        resp = ps2_read();
        if (resp != PS2_RESP_PASS && resp != PS2_RESP_P2PASS) {
            panic("PS/2 controller port 2 self-test failed!");
        }
        ps2_cmd(PS2_CMD_P2OFF);
    }

    //
    // enable PS/2 device interrupts and disable scancode translation
    //
    cfg |= PS2_CFG_P1INTON;
    if (port2) {
        cfg |= PS2_CFG_P2INTON;
    }
    cfg &= ~PS2_CFG_XLATON;
        ps2_cmd(PS2_CMD_WRCFG);
    ps2_write(cfg);

    //
    // enable PS/2 ports
    //
    ps2_cmd(PS2_CMD_P1ON);
    if (port2) {
        ps2_cmd(PS2_CMD_P2ON);
    }
    ps2_flush();

    restore_flags(flags);
}

void ps2_flush(void)
{
    do {
        inb_delay(PS2_PORT_DATA);
    } while (ps2_canread());
}

uint8_t ps2_status(void)
{
    return inb_delay(PS2_PORT_STATUS);
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

void ps2_cmd(uint8_t cmd)
{
    wait_for_write();
    // TODO: iodelay(xxx) ?
    outb_delay(PS2_PORT_CMD, cmd);
}

bool ps2_canread(void)
{
    // device output buffer must be full
    return has_flag(ps2_status(), PS2_STATUS_OPF);
}

bool ps2_canwrite(void)
{
    // device input buffer must be empty
    return !has_flag(ps2_status(), PS2_STATUS_IPF);
}
