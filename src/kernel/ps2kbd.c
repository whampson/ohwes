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
 *         File: kernel/ps2kbd.c
 *      Created: February 25, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

// http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Keyboard%20Protocol.htm
// http://www-ug.eecg.utoronto.ca/desl/manuals/ps2.pdf
// https://wiki.osdev.org/PS/2_Keyboard
// https://www.tayloredge.com/reference/Interface/atkeyboard.pdf
// https://stanislavs.org/helppc/8042.html

#include <ohwes.h>
#include <io.h>
#include <irq.h>
#include <ps2.h>
#include <string.h>

#define SCANCODE_SET    1
#define TYPEMATIC_BYTE  0x22    // auto-repeat rate and delay
#define TIMEOUT         25000   // register poll count before giving up
#define NUM_RETRIES     3       // command resends before giving up
#define KB_BUFFER_SIZE  128     // interrupt input buffer size

#define LED_SCRLOCK     (1<<0)
#define LED_NUMLOCK     (1<<1)
#define LED_CAPLOCK     (1<<2)

struct kb {

    uint8_t ident[2];   // identifier word
    int leds;           // LED state
    bool typematic;     // supports auto-repeat
    int typematic_byte; // auto-repeat config (see kb_typematic())
    int scancode_set;   // scancode set in use
    bool sc2_support;   // can do scancode set 2
    bool sc3_support;   // can do scancode set 3

    // bool ack;
    // bool resend;
    // bool error;

    uint8_t ring[KB_BUFFER_SIZE];   // scancode ring buffer fifo
    int rptr;                       // fifo head
    int wptr;                       // fifo tail
};
struct kb g_kb = {};

static bool kbring_empty(void);             // check if scancode fifo empty
static bool kbring_full(void);              // check if scancode fifo full
static uint8_t kbring_get(void);            // get scancode from software fifo
static void kbring_put(uint8_t data);       // put scancode in software fifo

static void kb_interrupt(void);         // handle keypress
// static void kb_head(uint8_t data);
// static void kb_tail(uint8_t data);

static uint8_t kb_rdport(void);
static void kb_wrport(uint8_t data);

static bool kb_ident(void);
static bool kb_scset(uint8_t set);
static bool kb_selftest(void);
static bool kb_setleds(uint8_t leds);
static bool kb_typematic(uint8_t typ);

#define RIF(x)  if (!(x)) { return; }
#define RIF_FALSE(x)  if (!(x)) { return false; }


void init_kb(void)
{
    memset(&g_kb, 0, sizeof(struct kb));

    // turn off the keyboard PS/2 port (will not receive key presses)
    ps2_cmd(PS2_CMD_P1OFF);

    kb_selftest();
    kb_ident();
    kb_setleds(LED_NUMLOCK | LED_SCRLOCK);
    kb_typematic(TYPEMATIC_BYTE);           // configure auto-repeat

    // detect supported scancode sets
    g_kb.sc3_support = kb_scset(3);
    g_kb.sc2_support = kb_scset(2);
    if (!kb_scset(SCANCODE_SET)) {
        kprint("ps2kbd: guessing scancode set 1...\n");
        g_kb.scancode_set = 1;
    }

    // re-enable keyboard PS/2 port
    ps2_cmd(PS2_CMD_P1ON);

    // register ISR and unmask IRQ1 on the PIC
    irq_register(IRQ_KEYBOARD, kb_interrupt);
    irq_unmask(IRQ_KEYBOARD);

    kprint("ps2kbd: 0x%X 0x%X\n",
        g_kb.ident[0], g_kb.ident[1]);
    kprint("ps2kbd: leds = 0x%X, typematic = %s, typematic_byte = 0x%X\n",
        g_kb.leds, YN(g_kb.typematic), g_kb.typematic_byte);
    kprint("ps2kbd: scancode_set = %d, sc2_support = %s, sc3_support = %s\n",
        g_kb.scancode_set,  YN(g_kb.sc2_support), YN(g_kb.sc3_support));
}


static bool kb_selftest(void)
{
    uint8_t data;
    bool supported;
    int retries;

    supported = kb_sendcmd(PS2KBD_CMD_SELFTEST);
    if (!supported) {
        return true;        // vacuous truth; can't fail if the test isn't supported! ;-)
    }

    retries = NUM_RETRIES;
    while (retries-- > 0) {
        data = kb_rdport();
        kb_rdport();     // may or may not transmit an ack after pass/fail byte
        if (data == 0xAA) {
            return true;    // pass!
        }
        else if (data == 0) {
            // 0 means we timed out reading... might be taking a while to complete test
            // let's try again...
            continue;
        }
        else if (data == 0xFC || data == 0xFD) {
            kprint("ps2kbd: self-test failed!\n");
            return false;
        }
        else {
            kprint("ps2kbd: self-test failed! (got 0x%X)\n", data);
            return false;
        }
    }

    if (data == 0) {
        // 0 means we timed out reading... on some machines, the command acks
        // but the result byte never comes... not sure why this is, let's
        // consider it a command support bug and thus vacuous
        kprint("ps2kbd: self-test did not respond!\n");
        return true;
    }

    return false;
}

static bool kb_ident(void)
{
    RIF_FALSE(kb_sendcmd(PS2KBD_CMD_IDENT));

    for (int i = 0; i < 2; i++) {
        g_kb.ident[i] = kb_rdport();
    }

    return true;
}

static bool kb_setleds(uint8_t leds)
{
    RIF_FALSE(kb_sendcmd(PS2KBD_CMD_SETLED));

    g_kb.leds = leds;
    kb_wrport(g_kb.leds);
    kb_rdport();     // ack

    return true;
}

static bool kb_scset(uint8_t set)
{
    uint8_t data;

    assert(set > 0 && set <= 3);
    RIF_FALSE(kb_sendcmd(PS2KBD_CMD_SCANCODE));

    kb_wrport(set); // write desired set
    kb_rdport();    // ack

    // readback
    kb_sendcmd(PS2KBD_CMD_SCANCODE);
    kb_wrport(0);   // request current set
    kb_rdport();    // ack
    data = kb_rdport();
    kb_rdport();    // may send additional ack

    if (data == set) {
        g_kb.scancode_set = set;
        return true;
    }

    return false;
}

static bool kb_typematic(uint8_t typ)
{
    // TYPEMATIC BYTE
    //  [0:4] repeat rate   (00000 = 30Hz, 11111 = 2Hz)
    //  [6:5] delay         (00 = 250ms, 11 = 1000ms)
    //    [7] must be zero

    assert(!(typ & 0x80));

    RIF_FALSE(kb_sendcmd(PS2KBD_CMD_TYPEMATIC));

    kb_wrport(typ);
    kb_rdport(); // ack

    g_kb.typematic = true;
    g_kb.typematic_byte = typ;

    return true;
}

bool kb_sendcmd(uint8_t cmd)
{
    uint32_t flags;
    uint8_t resp;
    int retries;
    bool ack;

    cli_save(flags);

    retries = NUM_RETRIES;
    ack = false;

    do {
        --retries;
        kb_wrport(cmd);
        resp = kb_rdport();
        if (resp == 0xFA) {
            ack = true;
            break;
        }
    } while (resp == 0xFE && retries);

    if (!retries) {
        kprint("ps2kbd: cmd 0x%X timed out after %d retries!\n", cmd, NUM_RETRIES);
    }
    else if (resp == 0) {
        kprint("ps2kbd: cmd 0x%X did not respond\n", cmd);
    }
    else if (!ack) {
        kprint("ps2kbd: cmd 0x%X returned 0x%X\n", cmd, resp);
    }

    restore_flags(flags);
    return ack;
}

static void kb_interrupt(void)
{
    uint8_t sc;

    ps2_cmd(PS2_CMD_P1OFF);

    sc = inb_delay(0x60);
    kbring_put(sc);         // put scancode into tail of ring buffer

    switch (sc) {
        // case 0xAA:
        //     kprint("kbint: self-test passed\n");
        //     break;

        // case 0xFA:
        //     g_kb.ack = true;
        //     break;

        case 0xFC: __fallthrough;
        case 0xFD:
            panic("kbint: inb 0x%X (self-test failed)\n", sc);
            break;

        case 0xFE:
            // g_kb.resend = true;
            panic("kbint: inb 0x%X (resend)\n", sc);
            break;

        case 0xFF:  __fallthrough;
        case 0x00:
            // g_kb.error = true;
            panic("kbint: inb 0x%X (error)\n", sc);
            break;
    }

    ps2_cmd(PS2_CMD_P1ON);
}

//
// pop data off front of ring buffer
//
uint8_t kb_read(void)
{
    uint8_t data;
    int count;

    // poll until data is available
    count = 0;
    while (kbring_empty() && count++ < TIMEOUT) { }

    if (count >= TIMEOUT) {
        // buffer empty
        return 0;
    }

    // grab data from the input buffer
    data = kbring_get();
    kprint("kb_read: got 0x%X\n", data);

    // // handle state machine changes
    // kb_tail(data);

    return data;
}

uint8_t kb_rdport(void)
{
    uint8_t status;
    uint32_t flags;
    uint8_t data;
    int count;

    cli_save(flags);

    // poll until write available
    count = 0;
    while (count++ < TIMEOUT) {
        status = ps2_status();
        if (status & PS2_STATUS_OPF) {
            break;
        }
    }

    if (status & PS2_STATUS_TIMEOUT) {
        kprint("ps2kbd: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kbd: parity error\n");
    }

    if (count >= TIMEOUT) {
        return 0;
    }

    data = inb_delay(0x60);
    switch (data) {
        case 0xFF: __fallthrough;
        case 0x00:
            kprint("ps2kbd: kb_rdport: inb 0x%X\n", data);
            break;
    }

    restore_flags(flags);
    return data;
}

void kb_wrport(uint8_t data)
{
    uint8_t status;
    uint32_t flags;
    int count;

    cli_save(flags);

    // poll until write available
    count = 0;
    while (count++ < TIMEOUT) {
        status = ps2_status();
        if (!(status & PS2_STATUS_IPF)) {
            break;
        }
    }

    if (status & PS2_STATUS_TIMEOUT) {
        kprint("ps2kbd: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kbd: parity error\n");
    }

    if (count >= TIMEOUT) {
        panic("ps2kbd: timed out waiting for write\n");
        goto done;
    }

    // write the port
    outb_delay(0x60, data);

done:
    restore_flags(flags);
}

// static void kb_head(uint8_t sc)
// {
// }

// static void kb_tail(uint8_t sc)
// {
// }

// ----------------------------------------------------------------------------
// kbring
// ----------------------------------------------------------------------------

static bool kbring_empty(void)
{
    uint32_t flags;
    cli_save(flags);

    bool empty = (g_kb.wptr == g_kb.rptr);

    restore_flags(flags);
    return empty;
}

static bool kbring_full(void)
{
    uint32_t flags;
    cli_save(flags);

    bool full =
        (g_kb.wptr == g_kb.rptr - 1) ||
        (g_kb.wptr - g_kb.rptr == KB_BUFFER_SIZE);

    restore_flags(flags);
    return full;
}

static uint8_t kbring_get(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t data;
    if (kbring_empty()) {
        panic("cannot get from an empty buffer\n");
        data = 0;
        goto done;
    }

    data = g_kb.ring[g_kb.rptr++];
    if (g_kb.rptr >= KB_BUFFER_SIZE) {
        g_kb.rptr = 0;
    }

done:
    restore_flags(flags);
    return data;
}

static void kbring_put(uint8_t data)
{
    uint32_t flags;
    cli_save(flags);

    if (kbring_full()) {
        panic("keyboard buffer is full!!! (size = %d)", KB_BUFFER_SIZE);
        goto done;
    }

    g_kb.ring[g_kb.wptr++] = data;
    if (g_kb.wptr >= KB_BUFFER_SIZE) {
        g_kb.wptr = 0;
    }

done:
    restore_flags(flags);
}
