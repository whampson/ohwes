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
#include <input.h>
#include <io.h>
#include <irq.h>
#include <ps2.h>
#include <queue.h>
#include <string.h>

#define SCANCODE_SET    1
#define TYPEMATIC_BYTE  0x22    // repeat rate = 24cps, delay = 500ms
#define TIMEOUT         25000   // register poll count before giving up
#define NUM_RETRIES     3       // command resends before giving up
#define KB_BUFFER_SIZE  32      // interrupt input buffer size

struct kb {
    // keyboard configuration
    unsigned char ident[2];     // identifier word
    int leds;                   // LED state
    bool typematic;             // supports auto-repeat
    int typematic_byte;         // auto-repeat config
    int scancode_set;           // scancode set in use
    bool sc2_support;           // can do scancode set 2
    bool sc3_support;           // can do scancode set 3

    // scancode state
    bool e0;
    bool e1;

    // keyboard state
    bool ctrl;
    bool alt;
    bool shift;
    bool meta;
    bool numlk;
    bool capslk;
    bool scrlk;

    // input buffer
    struct _queue inputq;
    char _input_buffer[KB_BUFFER_SIZE];

    // spurious scancode tracking
    int ack_count;
    int resend_count;
    int error_count;
};
struct kb g_kb = {};

const uint8_t g_ScanCode1[128];
const uint8_t g_ScanCode1_E0[128];
const char * g_KeyNames[122];

static void kb_interrupt(void);

static bool kb_ident(void);
static bool kb_scset(uint8_t set);
static bool kb_selftest(void);
static bool kb_setleds(uint8_t leds);
static bool kb_typematic(uint8_t typ);

static bool kb_sendcmd(uint8_t cmd);
static uint8_t kb_rdport(void);
static void kb_wrport(uint8_t data);

#define RIF(x)  if (!(x)) { return; }
#define RIF_FALSE(x)  if (!(x)) { return false; }

void init_kb(void)
{
    uint8_t ps2cfg;
    uint32_t flags;
    cli_save(flags);

    zeromem(&g_kb, sizeof(struct kb));
    q_init(&g_kb.inputq, g_kb._input_buffer, KB_BUFFER_SIZE);

    // disable keyboard
    ps2_flush();
    ps2_cmd(PS2_CMD_P1OFF);
    kb_sendcmd(PS2KB_CMD_SCANOFF);
    ps2_flush();

    // disable scancode translation
    ps2_cmd(PS2_CMD_RDCFG);
    ps2cfg = ps2_read();
    ps2cfg &= ~PS2_CFG_TRANSLATE;
    ps2_cmd(PS2_CMD_WRCFG);
    ps2_write(ps2cfg);

    // initialize keyboard
    kb_selftest();
    kb_ident();
    kb_setleds(PS2KB_LED_NUMLK);    // TODO: sync with capslk numlk scrlk vars
    g_kb.numlk = 1;
    kb_typematic(TYPEMATIC_BYTE);

    // detect supported scancode sets
    g_kb.sc3_support = kb_scset(3);
    g_kb.sc2_support = kb_scset(2);

    // select our desired scancode set
    if (!kb_scset(SCANCODE_SET)) {
        // if we couldn't pick a set (command may be unsupported),
        // turn translation back on so we are guaranteed to be using set 1
        ps2cfg |= PS2_CFG_TRANSLATE;
        ps2_cmd(PS2_CMD_WRCFG);
        ps2_write(ps2cfg);
        g_kb.scancode_set = 1;
    }

    // re-enable keyboard
    ps2_cmd(PS2_CMD_P1ON);
    kb_sendcmd(PS2KB_CMD_SCANON);
    ps2_flush();

    // register ISR and unmask IRQ1 on the PIC
    irq_register(IRQ_KEYBOARD, kb_interrupt);
    irq_unmask(IRQ_KEYBOARD);

    kprint("ps2kb: ident = 0x%X 0x%X, translation = %s\n",
        g_kb.ident[0], g_kb.ident[1], ONOFF(ps2cfg & PS2_CFG_TRANSLATE));
    kprint("ps2kb: scancode_set = %d, sc2_support = %s, sc3_support = %s\n",
        g_kb.scancode_set,  YN(g_kb.sc2_support), YN(g_kb.sc3_support));
    kprint("ps2kb: leds = 0x%X, typematic = %s, typematic_byte = 0x%X\n",
        g_kb.leds, YN(g_kb.typematic), g_kb.typematic_byte);

    restore_flags(flags);
}

char kb_read(void)
{
    uint32_t flags;
    char c;

    cli_save(flags);

    // grab a scancode from the queue
    c = '\0';
    if (!q_empty(&g_kb.inputq)) {
        c = q_get(&g_kb.inputq);
    }

    restore_flags(flags);
    return c;
}

static void kb_interrupt(void)
{
    uint8_t sc;
    uint8_t orig_sc;
    uint8_t leds;
    bool release;

    // prevent keyboard from sending more interrupts
    ps2_cmd(PS2_CMD_P1OFF);

    // grab the scancode
    sc = inb_delay(0x60);

    // check for some special scancodes
    switch (sc) {
        case 0xFC: __fallthrough;
        case 0xFD:
            kprint("kbint: inb 0x%X (self-test failed)\n", sc);
            g_kb.ack_count++;
            goto kb_done;
        case 0xFE:
            kprint("kbint: inb 0x%X (resend)\n", sc);
            g_kb.resend_count++;
            goto kb_done;
        case 0xFF:  __fallthrough;
        case 0x00:
            kprint("kbint: inb 0x%X (error)\n", sc);
            g_kb.error_count++;
            goto kb_done;
    }

    // the following translation is for scancode set 1 only
    assert(g_kb.scancode_set == 1);

    // did we get an escape code?
    if (sc == 0xE0) {
        g_kb.e0 = true;
        goto kb_done;
    }
    if (sc == 0xE1) {
        g_kb.e1 = true;
        goto kb_done;
    }

    // record the unmodified scancode
    orig_sc = sc;

    // determine if it's a break code
    release = (sc & 0x80);
    if (release) {
        sc ^= 0x80;
    }

    // translate the scancode to a virtual key
    // from here on, sc contains a virtual key
    if (g_kb.e0) {
        sc = g_ScanCode1_E0[sc];
    }
    else {
        sc = g_ScanCode1[sc];
    }

    // special handling for the PAUSE key
    if (g_kb.e1) {
        if (sc == KEY_NUMLK) {
            sc = KEY_PAUSE;
        }
    }

    // update toggle keys
    if (!release && sc == KEY_CAPSLK) {
        g_kb.capslk ^= 1;
    }
    if (!release && sc == KEY_NUMLK) {
        g_kb.numlk ^= 1;
    }
    if (!release && sc == KEY_SCRLK) {
        g_kb.scrlk ^= 1;
    }

    // update toggle key LEDs
    leds = 0;
    if (g_kb.capslk) {
        leds |= PS2KB_LED_CAPLK;
    }
    if (g_kb.numlk) {
        leds |= PS2KB_LED_NUMLK;
    }
    if (g_kb.scrlk) {
        leds |= PS2KB_LED_SCRLK;
    }

    if (g_kb.leds != leds) {
        kb_setleds(leds);
    }

    kprint("%-8s ", (release) ? "release" : "press");
    kprint("%-16s ", g_KeyNames[sc]);
    if (g_kb.e1) {
        kprint("e1 ");
    }
    if (g_kb.e0) {
        kprint("e0 ");
    }
    kprint("%02x\n", orig_sc);

    // TODO: record virtual key event somewhere
    // TODO: translate virtual key into ASCII sequence

    // put the character in the queue
    if (!q_full(&g_kb.inputq)) {
        q_put(&g_kb.inputq, sc);
    }
    else {
        beep(750, 50);  // buffer full! beep for effect ;)
    }

    // reset scancode state
    g_kb.e0 = false;
    if (g_kb.e1 && sc == KEY_PAUSE) {
        g_kb.e1 = false;
    }

kb_done:
    // re-enable keyboard interrupts from controller
    ps2_cmd(PS2_CMD_P1ON);
}

static bool kb_selftest(void)
{
    uint8_t data;
    bool supported;
    int retries;

    supported = kb_sendcmd(PS2KB_CMD_SELFTEST);
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
            kprint("ps2kb: self-test failed!\n");
            return false;
        }
        else {
            kprint("ps2kb: self-test failed! (got 0x%X)\n", data);
            return false;
        }
    }

    if (data == 0) {
        // 0 means we timed out reading... on some machines, the command acks
        // but the result byte never comes... not sure why this is, let's
        // consider it a command support bug and thus vacuous
        kprint("ps2kb: self-test did not respond!\n");
        return true;
    }

    return false;
}

static bool kb_ident(void)
{
    RIF_FALSE(kb_sendcmd(PS2KB_CMD_IDENT));

    for (int i = 0; i < 2; i++) {
        g_kb.ident[i] = kb_rdport();
    }

    return true;
}

static bool kb_setleds(uint8_t leds)
{
    RIF_FALSE(kb_sendcmd(PS2KB_CMD_SETLED));

    g_kb.leds = leds;
    kb_wrport(g_kb.leds);
    kb_rdport();     // ack

    return true;
}

static bool kb_scset(uint8_t set)
{
    uint8_t data;

    assert(set > 0 && set <= 3);
    RIF_FALSE(kb_sendcmd(PS2KB_CMD_SCANCODE));

    kb_wrport(set);                 // write desired set
    kb_rdport();                    // ack

    // readback
    kb_sendcmd(PS2KB_CMD_SCANCODE);
    kb_wrport(0);                   // request current set
    kb_rdport();                    // ack
    data = kb_rdport();
    kb_rdport();                    // may send additional ack

    if (data == set) {
        g_kb.scancode_set = set;    // keep track of the current scancode set
        return true;
    }

    return false;
}

static bool kb_typematic(uint8_t typ)
{
    assert(!(typ & 0x80));

    RIF_FALSE(kb_sendcmd(PS2KB_CMD_TYPEMATIC));

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
        kprint("ps2kb: cmd 0x%X timed out after %d retries!\n", cmd, NUM_RETRIES);
    }
    else if (resp == 0) {
        kprint("ps2kb: cmd 0x%X did not respond\n", cmd);
    }
    else if (!ack) {
        kprint("ps2kb: cmd 0x%X returned 0x%X\n", cmd, resp);
    }

    restore_flags(flags);
    return ack;
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
        kprint("ps2kb: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kb: parity error\n");
    }

    if (count >= TIMEOUT) {
        return 0;
    }

    data = inb_delay(0x60);
    switch (data) {
        case 0xFF: __fallthrough;
        case 0x00:
            kprint("ps2kb: kb_rdport: inb 0x%X\n", data);
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
        kprint("ps2kb: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kb: parity error\n");
    }

    if (count >= TIMEOUT) {
        panic("ps2kb: timed out waiting for write\n");
    }
    else {
        // write the port
        outb_delay(0x60, data);
    }

    restore_flags(flags);
}

const uint8_t g_ScanCode1[128] =
{
/*00-07*/  0,KEY_ESCAPE,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,
/*08-0F*/  KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_BACKSPACE,KEY_TAB,
/*10-17*/  KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,
/*18-1F*/  KEY_O,KEY_P,KEY_LEFTBRACKET,KEY_RIGHTBRACKET,KEY_ENTER,KEY_LCTRL,KEY_A,KEY_S,
/*20-27*/  KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_SEMICOLON,
/*28-2F*/  KEY_APOSTROPHE,KEY_GRAVE,KEY_LSHIFT,KEY_BACKSLASH,KEY_Z,KEY_X,KEY_C,KEY_V,
/*30-37*/  KEY_B,KEY_N,KEY_M,KEY_COMMA,KEY_DOT,KEY_SLASH,KEY_RSHIFT,KEY_KPASTERISK,
/*38-3F*/  KEY_LALT,KEY_SPACE,KEY_CAPSLK,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,
/*40-47*/  KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_NUMLK,KEY_SCRLK,KEY_KP7,
/*48-4F*/  KEY_KP8,KEY_KP9,KEY_KPMINUS,KEY_KP4,KEY_KP5,KEY_KP6,KEY_KPPLUS,KEY_KP1,
/*50-57*/  KEY_KP2,KEY_KP3,KEY_KP0,KEY_KPDOT,KEY_SYSRQ,0,0,KEY_F11, // 0x55 = undefined, 0x56 = INT1
/*58-5F*/  KEY_F12,0,0,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0, // undefined
/*68-6F*/  0,0,0,0,0,0,0,0, // undefined
/*70-77*/  0,0,0,0,0,0,0,0, // international
/*78-7F*/  0,0,0,0,0,0,0,0,
};

const uint8_t g_ScanCode1_E0[128] =
{
/*00-07*/  0,0,0,0,0,0,0,0,
/*08-0F*/  0,0,0,0,0,0,0,0,
/*10-17*/  0,0,0,0,0,0,0,0,
/*18-1F*/  0,0,0,0,KEY_KPENTER,KEY_RCTRL,0,0,
/*20-27*/  0,0,0,0,0,0,0,0,
/*28-2F*/  0,0,KEY_LSHIFT,0,0,0,0,0,    // fake shift
/*30-37*/  0,0,0,0,0,KEY_KPSLASH,KEY_RSHIFT,KEY_PRTSC,  // fake shift
/*38-3F*/  KEY_RALT,0,0,0,0,0,0,0,
/*40-47*/  0,0,0,0,0,0,KEY_BREAK,KEY_HOME,
/*48-4F*/  KEY_UP,KEY_PGUP,0,KEY_LEFT,0,KEY_RIGHT,0,KEY_END,
/*50-57*/  KEY_DOWN,KEY_PGDOWN,KEY_INSERT,KEY_DELETE,0,0,0,0,
/*58-5F*/  0,0,0,KEY_LWIN,KEY_RWIN,KEY_MENU,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

const char * g_KeyNames[122] =
{
    "",
    "KEY_LCTRL",
    "KEY_RCTRL",
    "KEY_LSHIFT",
    "KEY_RSHIFT",
    "KEY_LALT",
    "KEY_RALT",
    "KEY_BREAK",
    "KEY_BACKSPACE",
    "KEY_TAB",
    "KEY_ENTER",
    "KEY_F1",
    "KEY_F2",
    "KEY_F3",
    "KEY_F4",
    "KEY_F5",
    "KEY_F6",
    "KEY_F7",
    "KEY_F8",
    "KEY_F9",
    "KEY_F10",
    "KEY_F11",
    "KEY_F12",
    "KEY_LWIN",
    "KEY_RWIN",
    "KEY_MENU",
    "KEY_PAUSE",
    "KEY_ESCAPE",
    "KEY_SYSRQ",
    "KEY_CAPSLK",
    "KEY_NUMLK",
    "KEY_SCRLK",
    "KEY_SPACE",
    "KEY_RESERVED_33",
    "KEY_RESERVED_34",
    "KEY_RESERVED_35",
    "KEY_RESERVED_36",
    "KEY_RESERVED_37",
    "KEY_RESERVED_38",
    "KEY_APOSTROPHE",
    "KEY_RESERVED_40",
    "KEY_RESERVED_41",
    "KEY_KPASTERISK",
    "KEY_KPPLUS",
    "KEY_COMMA",
    "KEY_MINUS",
    "KEY_DOT",
    "KEY_SLASH",
    "KEY_0",
    "KEY_1",
    "KEY_2",
    "KEY_3",
    "KEY_4",
    "KEY_5",
    "KEY_6",
    "KEY_7",
    "KEY_8",
    "KEY_9",
    "KEY_RESERVED_58",
    "KEY_SEMICOLON",
    "KEY_RESERVED_60",
    "KEY_EQUAL",
    "KEY_RESERVED_62",
    "KEY_RESERVED_63",
    "KEY_RESERVED_64",
    "KEY_A",
    "KEY_B",
    "KEY_C",
    "KEY_D",
    "KEY_E",
    "KEY_F",
    "KEY_G",
    "KEY_H",
    "KEY_I",
    "KEY_J",
    "KEY_K",
    "KEY_L",
    "KEY_M",
    "KEY_N",
    "KEY_O",
    "KEY_P",
    "KEY_Q",
    "KEY_R",
    "KEY_S",
    "KEY_T",
    "KEY_U",
    "KEY_V",
    "KEY_W",
    "KEY_X",
    "KEY_Y",
    "KEY_Z",
    "KEY_LEFTBRACKET",
    "KEY_BACKSLASH",
    "KEY_RIGHTBRACKET",
    "KEY_RESERVED_94",
    "KEY_RESERVED_95",
    "KEY_GRAVE",
    "KEY_KPMINUS",
    "KEY_KPDOT",
    "KEY_KPSLASH",
    "KEY_KPENTER",
    "KEY_KP1",
    "KEY_KP2",
    "KEY_KP3",
    "KEY_KP4",
    "KEY_KP5",
    "KEY_KP6",
    "KEY_KP7",
    "KEY_KP8",
    "KEY_KP9",
    "KEY_KP0",
    "KEY_PRTSC",
    "KEY_INSERT",
    "KEY_DELETE",
    "KEY_HOME",
    "KEY_END",
    "KEY_PGUP",
    "KEY_PGDOWN",
    "KEY_LEFT",
    "KEY_DOWN",
    "KEY_RIGHT",
    "KEY_UP",
};
