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
 *         File: kernel/char/ps2.c
 *      Created: February 21, 2024
 *       Author: Wes Hampson
 *
 * Intel 8042 PS/2 Controller driver.
 * =============================================================================
 */

// https://stanislavs.org/helppc/8042.html
// https://stanislavs.org/helppc/keyboard_commands.html
// https://www.tayloredge.com/reference/Interface/atkeyboard.pdf

#include <i386/boot.h>
#include <i386/io.h>
#include <i386/ps2.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>

static void wait_for_read(void);
static void wait_for_write(void);

__init void init_ps2(void)
{
    uint8_t cfg, resp;
    bool port1, port2;

    //
    // disable ports and flush output buffer
    //
    ps2_cmd(PS2_CMD_P1OFF);
    ps2_cmd(PS2_CMD_P2OFF);
    ps2_flush();

    //
    // test for the existence of port 1
    //
    ps2_cmd(PS2_CMD_P1ON);
    ps2_cmd(PS2_CMD_RDCFG);
    cfg = ps2_read();
    port1 = !(cfg & PS2_CFG_P1CLKOFF);
    if (port1) {
        pr_info("detected PS/2 keyboard\n");
    }

    //
    // test for the existence of port 2
    //
    ps2_cmd(PS2_CMD_P2ON);
    ps2_cmd(PS2_CMD_RDCFG);
    cfg = ps2_read();
    port2 = !(cfg & PS2_CFG_P2CLKOFF);
    if (port2) {
        pr_info("detected PS/2 mouse\n");
    }

    //
    // run self tests
    //
    ps2_cmd(PS2_CMD_TEST);
    resp = ps2_read();
    if (resp != PS2_RESP_PASS) {
        pr_error("PS/2 controller self-test failed!");
    }

    ps2_cmd(PS2_CMD_P1TEST);
    resp = ps2_read();
    if (resp != PS2_RESP_PASS && resp != PS2_RESP_P1PASS) {
        pr_error("PS/2 controller port 1 self-test failed!");
    }

    if (port2) {
        ps2_cmd(PS2_CMD_P2TEST);
        resp = ps2_read();
        if (resp != PS2_RESP_PASS && resp != PS2_RESP_P2PASS) {
            pr_error("PS/2 controller port 2 self-test failed!");
        }
        ps2_cmd(PS2_CMD_P2OFF);
    }

    //
    // enable PS/2 device interrupts
    //
    cfg |= PS2_CFG_P1INTON;
    if (port2) {
        cfg |= PS2_CFG_P2INTON;
    }
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_write(cfg);

    //
    // enable PS/2 ports
    //
    ps2_cmd(port1 ? PS2_CMD_P1ON : PS2_CMD_P1OFF);
    ps2_cmd(port2 ? PS2_CMD_P2ON : PS2_CMD_P2OFF);
    ps2_flush();
}

bool ps2_canread(void)
{
    // device output buffer must be full
    return ps2_status() & PS2_STATUS_OPF;
}

bool ps2_canwrite(void)
{
    // device input buffer must be empty
    return !(ps2_status() & PS2_STATUS_IPF);
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

void ps2_cmd(uint8_t cmd)
{
    wait_for_write();
    outb_delay(PS2_PORT_CMD, cmd);
}

static void wait_for_read(void)
{
    for (int i = 0; i < PS2_IO_TIMEOUT; i++) {
        if (ps2_canread()) {
            return;
        }
    }
    panic("timed out waiting for PS/2 controller read! (%d tries)", PS2_IO_TIMEOUT);
}

static void wait_for_write(void)
{
    for (int i = 0; i < PS2_IO_TIMEOUT; i++) {
        if (ps2_canwrite()) {
            return;
        }
    }
    panic("timed out waiting for PS/2 controller write! (%d tries)", PS2_IO_TIMEOUT);
}
