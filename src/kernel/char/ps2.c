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

// TODO: merge file with ps2kb.c

#include <i386/boot.h>
#include <i386/io.h>
#include <i386/ps2.h>
#include <kernel/kernel.h>

#define pr_fmt(fmt) "ps2-ctl: " fmt
#include <kernel/kprint.h>

#define IO_TIMEOUT          250000  // register poll count before giving up
#define IO_TIMEOUT_FAST     20000   // ditto, for use in ISRs and other speed-critical areas
#define DO_SELFTEST         1
#define RETRY_ATTEMPTS      2

static bool read_ready(void)
{
    // device output buffer must be full
    return ps2_read_status() & PS2_STATUS_OPF;
}

static bool write_ready(void)
{
    // device input buffer must be empty
    return !(ps2_read_status() & PS2_STATUS_IPF);
}

static bool wait_for_read_timeout(uint32_t timeout)
{
    for (uint32_t i = 0; i < timeout; i++) {
        if (read_ready()) {
            return true;
        }
    }
    return false;
}

static bool wait_for_write_timeout(uint32_t timeout)
{
    for (uint32_t i = 0; i < timeout; i++) {
        if (write_ready()) {
            return true;
        }
    }
    return false;
}

static bool wait_for_read(void)
{
    if (!wait_for_read_timeout(IO_TIMEOUT)) {
        pr_debug("poll read timeout");
        return false;
    }
    return true;
}

static bool wait_for_write(void)
{
    if (!wait_for_write_timeout(IO_TIMEOUT)) {
        pr_debug("poll write timeout");
        return false;
    }
    return true;
}

__init void init_ps2(void)
{
    static bool s_ps2_initialized = false;

    uint8_t cfg, result;
    bool port1, port2;

    if (s_ps2_initialized) {
        return;
    }

    //
    // disable ports and flush output buffer
    //
    ps2_write_cmd(PS2_CMD_P1OFF);
    ps2_write_cmd(PS2_CMD_P2OFF);
    ps2_flush();

    //
    // set initial port configuration
    //
    cfg = ps2_read_config();
    cfg &= ~(PS2_CFG_P1INTON|PS2_CFG_P2INTON);  // disable interrupts
    cfg &= ~PS2_CFG_P1CLKOFF;                   // enable port 1 clock
    cfg |=  PS2_CFG_P2CLKOFF;                   // disable port 2 clock (no mouse support)
    cfg &= ~PS2_CFG_TRANSLATE;                  // disable scancode translation
    ps2_write_config(cfg);

    //
    // perform controller self test
    //
#if DO_SELFTEST
    ps2_write_cmd(PS2_CMD_SELFTEST);
    for (int i = 0; i <= RETRY_ATTEMPTS; i++) {
        result = ps2_read();
        if (result == 0) {
            continue;   // timed out...
        }
        break;
    }
    if (result == 0) {
        pr_warn("self-test timed out!");
    }
    else if (result != PS2_RESP_PASS) {
        pr_error("self-test failed!");    // TODO: abort?
    }
    ps2_write_config(cfg);  // restore config
#endif

    //
    // detect port existence
    //
    cfg = ps2_read_config();
    port1 = !(cfg & PS2_CFG_P1CLKOFF);
    port2 = !(cfg & PS2_CFG_P2CLKOFF);
    if (port1) {
        pr_info("detected PS/2 keyboard port");
    }
    if (port2) {
        pr_info("detected PS/2 mouse port");
    }

    //
    // perform device port self tests
    //
#if DO_SELFTEST
    if (port1) {
        ps2_write_cmd(PS2_CMD_P1TEST);
        result = ps2_read();
        if (result != PS2_RESP_P1PASS) {
            pr_warn("port 1 self-test failed!");
        }
    }
    if (port2) {
        ps2_write_cmd(PS2_CMD_P2TEST);
        result = ps2_read();
        if (result != PS2_RESP_P2PASS) {
            pr_warn("port 2 self-test failed!");
        }
    }
#endif


    s_ps2_initialized = true;
    // TODO: we assume it's good, set some kind of status if bad
}

uint8_t ps2_read(void)
{
    if (!wait_for_read()) {
        return 0;
    }
    return inb_slow(PS2_PORT_DATA);
}

uint8_t ps2_read_fast(void)
{
    if (!wait_for_read_timeout(IO_TIMEOUT_FAST)) {
        return 0;
    }
    return inb_slow(PS2_PORT_DATA);
}

uint8_t ps2_read_status(void)
{
    return inb_slow(PS2_PORT_STATUS);
}

uint8_t ps2_read_config(void)
{
    ps2_write_cmd(PS2_CMD_RDCFG);
    return ps2_read();
}

void ps2_write(uint8_t data)
{
    if (wait_for_write()) {
        outb_slow(PS2_PORT_DATA, data);
    }
}

bool ps2_write_fast(uint8_t data)
{
    if (!wait_for_write_timeout(IO_TIMEOUT_FAST)) {
        return false;
    }

    outb_slow(PS2_PORT_DATA, data);
    return true;
}

void ps2_write_cmd(uint8_t cmd)
{
    if (wait_for_write()) {
        outb_slow(PS2_PORT_CMD, cmd);
    }
}

void ps2_write_config(uint8_t cfg)
{
    ps2_write_cmd(PS2_CMD_WRCFG);
    ps2_write(cfg);
}

void ps2_flush(void)
{
    do {
        inb_slow(PS2_PORT_DATA);
    } while (read_ready());
}
