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
// http://www.quadibloc.com/comp/scan.htm
// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html

#include <ctype.h>
#include <boot.h>
#include <ohwes.h>
#include <console.h>
#include <input.h>
#include <io.h>
#include <irq.h>
#include <ps2.h>
#include <string.h>

#define CHATTY_KB       0       // print extra debug messages

#define SCANCODE_SET    1       // using scancode set 1
#define TYPEMATIC_BYTE  0x22    // repeat rate = 24cps, delay = 500ms
#define RETRY_COUNT     3       // command resends before giving up
#define WARN_INTERVAL   10      // warn every N times a stray packet shows up

#define PRINT_EVENTS    0

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
    int ctrl;
    int alt;
    int shift;
    int meta;
    bool numlk;
    bool capslk;
    bool scrlk;
    int altcode;
    bool has_altcode;

    // // key event buffer
    // struct ring eventq;            // TODO: make queue w/ generic type
    // struct key_event ebuf[KB_BUFFER_SIZE];

    // spurious scancode tracking
    int ack_count;
    int resend_count;
    int error_count;
};

struct kb g_kb = { };

static const char g_keymap[256];
static const char g_keymap_shift[128];
static const uint8_t g_scanmap1[128];
static const uint8_t g_scanmap1_e0[128];

#if PRINT_EVENTS
static const char * g_keynames[122];
#endif

static void update_leds(void);
static void kb_interrupt(int irq_num);
static void kb_putq(char c);

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

extern void init_ps2(const struct boot_info *info);

void init_kb(const struct boot_info *info)
{
    uint8_t ps2cfg;

    init_ps2(info);

    zeromem(&g_kb, sizeof(struct kb));
    g_kb.numlk = 1;

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
    update_leds();
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

#if CHATTY_KB
    kprint("ps2kb: ident=%02Xh,%02Xh translation=%s\n",
        g_kb.ident[0], g_kb.ident[1], ONOFF(ps2cfg & PS2_CFG_TRANSLATE));
    kprint("ps2kb: leds=%02Xh typematic=%02Xh\n",
        g_kb.leds, g_kb.typematic_byte);
    kprint("ps2kb: scancode_set=%d sc2_support=%s sc3_support=%s\n",
        g_kb.scancode_set,  YN(g_kb.sc2_support), YN(g_kb.sc3_support));
#endif
}

static void kb_putq(char c)
{
    struct tty *tty = current_console()->tty;
    if (!tty || !tty->ldisc) {
        panic("no TTY attached to keyboard!");
    }

    if (!tty->ldisc->read) {
        panic("keyboard has no input receiver!");
    }

    tty->ldisc->recv(tty, &c, 1);   // just drop chars if its full...
}

static void update_leds(void)
{
    // update toggle key LEDs
    int leds = 0;
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
}

static void kb_interrupt(int irq_num)
{
    uint32_t flags;
    uint8_t status;
    uint16_t sc;
    uint16_t key;
    bool release;
    unsigned char c;
    char *s;

    struct key_event evt;
    zeromem(&evt, sizeof(struct key_event));

    c = '\0';
    s = NULL;

    assert(irq_num == IRQ_KEYBOARD);

    //
    // Scan Code to Key Code Mapping
    // ----------------------------------------------------------------

    // prevent keyboard from sending more interrupts
    cli_save(flags);
    ps2_cmd(PS2_CMD_P1OFF);

    // check keyboard status
    status = ps2_status();
#if CHATTY_KB
    if (status & PS2_STATUS_TIMEOUT) {
        kprint("ps2kb: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kb: parity error\n");
    }
#endif
    (void) status;

    // grab the scancode
    sc = inb_delay(0x60);

    // check for some unexpected scancodes
    switch (sc) {
        case 0xFA:
            g_kb.ack_count++;
            if ((g_kb.ack_count % WARN_INTERVAL) == 0) {
                kprint("ps2kb: seen %d stray acks\n", g_kb.ack_count);
            }
            // TODO: panic after some amount...?
            goto done;

        case 0xFE:
            g_kb.resend_count++;
            if ((g_kb.resend_count % WARN_INTERVAL) == 0) {
                kprint("ps2kb: seen %d stray resend requests\n", g_kb.resend_count);
            }
            goto done;

        case 0xFC: __fallthrough;   // self-test failed
        case 0xFD: __fallthrough;   // self-test failed
        case 0xFF: __fallthrough;   // error
        case 0x00:                  // error
            g_kb.error_count++;
            if (g_kb.error_count == 1) {
                kprint("ps2kb: got error 0x%X\n", g_kb.error_count);
            }
            if ((g_kb.error_count % WARN_INTERVAL) == 0) {
                kprint("ps2kb: seen %d errors\n", g_kb.error_count);
            }
            goto done;
    }

    // the following translation is for scancode set 1 only
    assert(g_kb.scancode_set == 1);

    // did we get an escape code?
    if (sc == 0xE0) {
        g_kb.e0 = true;
        goto done;
    }
    if (sc == 0xE1) {
        g_kb.e1 = true;
        goto done;
    }

    // determine if it's a break code
    release = (sc & 0x80);
    if (release) {
        sc &= ~0x80;
    }

    // translate the scancode to a virtual key
    key = (g_kb.e0) ? g_scanmap1_e0[sc] : g_scanmap1[sc];

    // end E0 escape sequence (should only be one byte)
    if (g_kb.e0) {
        assert(!g_kb.e1);
        sc |= 0xE000;
        g_kb.e0 = false;
    }

    // special handling for the PAUSE key, because PAUSE and NUMLK share
    // a final scancode byte for some reason; also this is the only E1 key
    if (g_kb.e1) {
        assert(!g_kb.e0);
        if (key == KEY_NUMLK) {
            key = KEY_PAUSE;
            sc |= 0xE100;
            g_kb.e1 = false;
        }
    }

    //
    // Key Code Handling
    // ----------------------------------------------------------------

    // numlock handling
    if (!g_kb.numlk) {
        switch (key) {
            case KEY_KP0: key = KEY_INSERT; break;
            case KEY_KP1: key = KEY_END; break;
            case KEY_KP2: key = KEY_DOWN; break;
            case KEY_KP3: key = KEY_PGDOWN; break;
            case KEY_KP4: key = KEY_LEFT; break;
            case KEY_KP6: key = KEY_RIGHT; break;
            case KEY_KP7: key = KEY_HOME; break;
            case KEY_KP8: key = KEY_UP; break;
            case KEY_KP9: key = KEY_PGUP; break;
            case KEY_KPDOT: key = KEY_DELETE; break;
        }
    }

    // update toggle keys and LEDs
    if (!release && key == KEY_CAPSLK) {
        g_kb.capslk ^= true;
    }
    if (!release && key == KEY_NUMLK) {
        g_kb.numlk ^= true;
    }
    if (!release && key == KEY_SCRLK) {
        g_kb.scrlk ^= true;
    }
    update_leds();

    // update modifier key state
    #define handle_modifier(key,mod,l,r)        \
    ({                                          \
        int _mask = 0;                          \
        if ((key) == (l)) { _mask |= 1 << 0; }  \
        if ((key) == (r)) { _mask |= 1 << 1; }  \
        if (_mask && !release) {                \
            g_kb.mod |= _mask;                  \
        }                                       \
        else if (_mask && release) {            \
            g_kb.mod &= ~_mask;                 \
        }                                       \
    })
    handle_modifier(key, ctrl, KEY_LCTRL, KEY_RCTRL);
    handle_modifier(key, shift, KEY_LSHIFT, KEY_RSHIFT);
    handle_modifier(key, alt, KEY_LALT, KEY_RALT);
    handle_modifier(key, meta, KEY_LWIN, KEY_RWIN);

    // submit alt code upon release of ALT key
    if (isalt(key) && release && g_kb.has_altcode) {
        kb_putq(g_kb.altcode);
        g_kb.has_altcode = false;
        g_kb.altcode = 0;
        goto record_key_event;
    }

    //
    // handle special keystrokes
    //

    // CTRL+ALT+DEL: system reboot
    switch (key) {
        case KEY_DELETE:
        case KEY_KPDOT:
            if (g_kb.ctrl && g_kb.alt) {
                ps2_cmd(PS2_CMD_SYSRESET);
                for (;;);
            }
            break;
    }

#ifdef DEBUG
    if (g_kb.ctrl && g_kb.alt) {
        if (isfnkey(key)) {
            g_crash_kernel = key - KEY_F1 + 1;
        }
    }
#endif

    // TODO: CTRL+SCRLK = print kernel output buffer
    // TOOD: SYSRQ = something cool (debug menu?)

    // ALT+<FN>: switch terminal
    if (g_kb.alt && !g_kb.ctrl) {
        if (isfnkey(key) && !release) {
            int cons = key - KEY_F1 + 1;
            switch_console(cons);
            goto done;
        }
    }

    // ALT+<NUMPAD>: handle character code entry (if NumLk on)
    if (g_kb.alt && !release && iskpnum(key)) {
        g_kb.has_altcode = true;
        g_kb.altcode *= 10;
        g_kb.altcode += (key - KEY_KP0);
        goto record_key_event;
    }

    // end if it's a break event since they don't generate characters
    if (release) {
        goto record_key_event;
    }

    //
    // Keystroke to Character Sequence Mapping
    // ----------------------------------------------------------------

    // map key to character
    c = (g_kb.shift && key >= 0x20 && key <= 0x60)
        ? g_keymap_shift[key & 0x7F]
        : g_keymap[key & 0xFF];
    if (c == '\0') {
        goto record_key_event;
    }

    // handle non-character keys
    if (c == 0xE0 || (key == KEY_KP5 && !g_kb.numlk)) {
        c = '\0';
        switch (key) {
            // xterm sequences
            case KEY_UP:    s = "\e[A"; break;
            case KEY_DOWN:  s = "\e[B"; break;
            case KEY_RIGHT: s = "\e[C"; break;
            case KEY_LEFT:  s = "\e[D"; break;
            case KEY_KP5:   s = "\e[G"; break;  // maybe, conflicts with console
            case KEY_PRTSC: s = "\e[P"; break;  // maybe
            // VT sequences
            case KEY_HOME:  s = "\e[1~"; break;
            case KEY_INSERT:s = "\e[2~"; break;
            case KEY_DELETE:s = "\e[3~"; break;
            case KEY_END:   s = "\e[4~"; break;
            case KEY_PGUP:  s = "\e[5~"; break;
            case KEY_PGDOWN:s = "\e[6~"; break;
            case KEY_F1:    s = "\e[11~"; break;
            case KEY_F2:    s = "\e[12~"; break;
            case KEY_F3:    s = "\e[13~"; break;
            case KEY_F4:    s = "\e[14~"; break;
            case KEY_F5:    s = "\e[15~"; break;
            case KEY_F6:    s = "\e[17~"; break;
            case KEY_F7:    s = "\e[18~"; break;
            case KEY_F8:    s = "\e[19~"; break;
            case KEY_F9:    s = "\e[20~"; break;
            case KEY_F10:   s = "\e[21~"; break;
            case KEY_F11:   s = "\e[23~"; break;
            case KEY_F12:   s = "\e[24~"; break;
            // TODO: sysrq? pause? break?
            default:        s = "\0"; break;
        }
        while (*s != '\0') {
            kb_putq(*s++);
        }
        goto record_key_event;
    }

    // handle control characters
    if (g_kb.ctrl) {
        switch (key) {
            case KEY_2: c = '@'; break;
            case KEY_7: c = '^'; break;
            case KEY_LEFTBRACKET: c = '['; break;
            case KEY_BACKSLASH: c = '\\'; break;
            case KEY_RIGHTBRACKET: c = ']'; break;
            case KEY_MINUS: c = '_'; break;
            case KEY_SLASH: c = '?'; break;
            case KEY_BACKSPACE: c = '\b'; break;
        }
        if (key >= KEY_A && key <= KEY_Z) {
            c = toupper(c);
        }
        if ((c >= '@' && c <= '_') || c == '?') {
            c ^= 0x40;
        }
    }

    // handle caps lock
    if (g_kb.capslk && !g_kb.alt) {
        if (isupper(c)) {
            c = tolower(c);
        }
        else if (islower(c)) {
            c = toupper(c);
        }
    }

    // handle alt
    if (g_kb.alt) {
        kb_putq('\e');
    }

    // put the character in the queue
    kb_putq(c);

record_key_event:
    evt.keycode = key;
    evt.scancode = sc;
    evt.release = release;
    evt.c = c;
    // TODO: add to event queue

#if PRINT_EVENTS
    kprint("ps2kb: ");
    kprint("%-8s  ", (release) ? "release" : "press");
    kprint("%c  ", isprint(c) ? c : ' ');
    kprint("% 4.2x ", key);
    kprint("% 4.2x ", sc);
    kprint("  %s", g_keynames[key]);
    kprint("\n");
#endif

done:
    // re-enable keyboard interrupts from controller
    ps2_cmd(PS2_CMD_P1ON);
    restore_flags(flags);
}

static bool kb_selftest(void)
{
    uint8_t data;
    bool supported;
    int retries;

    supported = kb_sendcmd(PS2KB_CMD_SELFTEST);
    if (!supported) {
        return true;            // vacuous truth; can't fail if the test isn't supported! ;-)
    }

    retries = RETRY_COUNT;
    while (retries-- > 0) {
        data = kb_rdport();
        kb_rdport();            // may or may not transmit an ack after pass/fail byte
        if (data == 0xAA) {
            return true;        // pass!
        }
        else if (data == 0) {
            // 0 means we timed out reading... might be taking a while to complete test
            // let's try again...
            continue;
        }
        else if (data == 0xFC || data == 0xFD) {
#if CHATTY_KB
            kprint("ps2kb: self-test failed!\n");
#endif
            return false;
        }
        else {
#if CHATTY_KB
            kprint("ps2kb: self-test failed! (got 0x%X)\n", data);
#endif
            return false;
        }
    }

    if (data == 0) {
        // 0 means we timed out reading... on some machines, the command acks
        // but the result byte never comes... not sure why this is, let's
        // consider it a command support bug and thus vacuous
#if CHATTY_KB
        kprint("ps2kb: self-test did not respond!\n");
#endif
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
    kb_rdport();                    // ack

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
    kb_rdport();                    // ack

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

    retries = RETRY_COUNT;
    ack = false;

    do {
        --retries;
        kb_wrport(cmd);
        resp = kb_rdport();
        if (resp == 0xFA) {
            ack = true;
            break;
        }
#if CHATTY_KB
        if (resp != 0) {
            kprint("ps2kb: cmd 0x%X returned 0x%X, trying again...\n", cmd, resp);
        }
#endif
    } while (resp != 0 && retries);

#if CHATTY_KB
    if (!retries) {
        kprint("ps2kb: cmd 0x%X timed out after %d retries!\n", cmd, RETRY_COUNT);
    }
    else if (resp == 0) {
        kprint("ps2kb: cmd 0x%X not supported\n", cmd);
    }
#endif

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
    while (count++ < PS2_IO_TIMEOUT) {
        status = ps2_status();
        if (status & PS2_STATUS_OPF) {
            break;
        }
    }

#if CHATTY_KB
    if (status & PS2_STATUS_TIMEOUT) {
        kprint("ps2kb: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kb: parity error\n");
    }
#endif

    if (count >= PS2_IO_TIMEOUT) {
        return 0;
    }

    data = inb_delay(0x60);
    switch (data) {
        case 0xFF:
#if CHATTY_KB
            kprint("ps2kb: kb_rdport: inb 0x%X\n", data);
#endif
            g_kb.error_count++;
             __fallthrough;
        case 0x00:
            // going to consider 0 ok here...
            // some keyboards return 00 00 when identifying
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
    while (count++ < PS2_IO_TIMEOUT) {
        status = ps2_status();
        if (!(status & PS2_STATUS_IPF)) {
            break;
        }
    }

#if CHATTY_KB
    if (status & PS2_STATUS_TIMEOUT) {
        kprint("ps2kb: timeout error\n");
    }
    if (status & PS2_STATUS_PARITY) {
        kprint("ps2kb: parity error\n");
    }
#endif

    if (count >= PS2_IO_TIMEOUT) {
#if CHATTY_KB
        panic("ps2kb: timed out waiting for write\n");
#endif
    }
    else {
        // write the port
        outb_delay(0x60, data);
    }

    restore_flags(flags);
}

static const char g_keymap[256] =
{
/*00-0F*/  0,0,0,0,0,0,0,0,0x7F,'\t','\r',0xE0,0xE0,0xE0,0xE0,0xE0,
/*10-1F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,'\e',0,0,0,0,
/*20-2F*/  ' ',0,0,0,0,0,0,'\'',0,0,'*','+',',','-','.','/',
/*30-3F*/  '0','1','2','3','4','5','6','7','8','9',0,';',0,'=',0,0,
/*40-4F*/  0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
/*50-5F*/  'p','q','r','s','t','u','v','w','x','y','z','[','\\',']',0,0,
/*60-6F*/  '`','-','.','/','0','1','2','3','4','5','6','7','8','9','\r',0xE0,
/*70-7F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,0,
/*80-8F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const char g_keymap_shift[128] =
{
/*00-0F*/  0,0,0,0,0,0,0,0,0x7F,'\t','\r',0,0,0,0,0,
/*10-1F*/  0,0,0,0,0,0,0,0,0,0,0,'\e',0,0,0,0,
/*20-2F*/  ' ',0,0,0,0,0,0,'"',0,0,'*','+','<','_','>','?',
/*30-3F*/  ')','!','@','#','$','%','^','&','*','(',0,':',0,'+',0,0,
/*40-4F*/  0,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
/*50-5F*/  'P','Q','R','S','T','U','V','W','X','Y','Z','{','|','}',0,0,
/*60-6F*/  '~','-','.','/','0','1','2','3','4','5','6','7','8','9','\r',0,
/*70-7F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const uint8_t g_scanmap1[128] =
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
/*50-57*/  KEY_KP2,KEY_KP3,KEY_KP0,KEY_KPDOT,KEY_SYSRQ,0,0,KEY_F11,
/*58-5F*/  KEY_F12,0,0,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const uint8_t g_scanmap1_e0[128] =
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

#if PRINT_EVENTS
static const char * g_keynames[122] =
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
    "KEY_GRAVE",
    "KEY_KPMINUS",
    "KEY_KPDOT",
    "KEY_KPSLASH",
    "KEY_KP0",
    "KEY_KP1",
    "KEY_KP2",
    "KEY_KP3",
    "KEY_KP4",
    "KEY_KP5",
    "KEY_KP6",
    "KEY_KP7",
    "KEY_KP8",
    "KEY_KP9",
    "KEY_KPENTER",
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
#endif
