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
 *    File: kernel/keyboard.c                                                 *
 * Created: December 28, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <ascii.h>
#include <ctype.h>
#include <string.h>
#include <queue.h>
#include <ohwes/debug.h>
#include <ohwes/kernel.h>
#include <ohwes/keyboard.h>
#include <ohwes/interrupt.h>
#include <ohwes/irq.h>
#include <ohwes/ohwes.h>
#include <drivers/ps2.h>

#define KBD_BUFLEN  128

static const uint8_t SC1[128];
static const uint8_t SC1_E0[128];
static const uint8_t CH[128];
static const uint8_t CH_SHIFT[128];

static bool m_num = false;
static bool m_caps = false;
static bool m_scroll = false;
static bool m_shift = false;
static bool m_ctrl = false;
static bool m_alt = false;
static bool m_super = false;
static uint8_t m_mode = KB_COOKED;
static bool m_echo = true;
static uint64_t m_keydown_map[2] = { 0 };
static char _qbuf[KBD_BUFLEN + 28];
static queue_t *m_queue = (queue_t *) (_qbuf + KBD_BUFLEN);

#define qempty()        (queue_empty(m_queue))
#define qfull()         (queue_full(m_queue))
#define getq()          (queue_get(m_queue))
#define putq(c)         (queue_put(m_queue, c))

#define keydown(vk)     (m_keydown_map[(vk)/64]|= (1ULL<<((vk)%64)))
#define keyup(vk)       (m_keydown_map[(vk)/64]&=~(1ULL<<((vk)%64)))
#define is_keydown(vk)  (m_keydown_map[(vk)/64]&  (1ULL<<((vk)%64)))

static bool scancode_set1(void);
static void kbd_interrupt(void);
static void kbd_putq(char c);
static void kbd_putqs(const char *s);

void kbd_init(void)
{
    queue_init(m_queue, _qbuf, KBD_BUFLEN);

    ps2_init();
    if (!ps2_testctl()) panic("ps2ctl: self-test failed!");
    if (!ps2_testp1()) panic("ps2ctl: port 1 self-test failed!");
    if (!ps2_testp2()) panic("ps2ctl: port 1 self-test failed!");

    if (!ps2kbd_test()) panic("ps2kbd: self-test failed!");
    if (!scancode_set1()) warn("ps2kbd: failed to switch to scancode set 1!\n");
    ps2kbd_on();

    irq_register_handler(IRQ_KEYBOARD, kbd_interrupt);
    irq_unmask(IRQ_KEYBOARD);
}

bool key_down(vk_t key)
{
    return is_keydown(key & 0x7F);
}

bool ctrl_down(void)
{
    return is_keydown(VK_LCTRL) || is_keydown(VK_RCTRL);
}

bool shift_down(void)
{
    return is_keydown(VK_LSHIFT) || is_keydown(VK_RSHIFT);
}

bool alt_down(void)
{
    return is_keydown(VK_LALT) || is_keydown(VK_RALT);
}

bool super_down(void)
{
    return is_keydown(VK_LSUPER) || is_keydown(VK_RSUPER);
}

bool capslock(void)
{
    return m_caps;
}

bool numlock(void)
{
    return m_num;
}

bool scrlock(void)
{
    return m_scroll;
}

int kbd_getmode(void)
{
    return m_mode;
}

bool kbd_setmode(int mode)
{
    switch (mode) {
        case KB_RAW:
        case KB_MEDIUMRAW:
        case KB_COOKED:
            m_mode = mode;
            return true;
    }

    return false;
}

void kbd_setecho(bool enabled)
{
    m_echo = enabled;
}

void kbd_flush(void)
{
    while (!qempty()) (void) getq();
}

ssize_t kbd_read(char *buf, size_t n)
{
    uint32_t flags;
    size_t i;

    cli_save(flags);
    for (i = 0; i < n; i++) {
        if (qempty()) break;
        *(buf++) = getq();
    }

    restore_flags(flags);
    return i;
}

static bool scancode_set1(void)
{
    uint8_t data;

    data = 1;
    if (ps2kbd_cmd(KBD_CMD_SCANCODE, &data, 1)) {
        ps2kbd_reset();
        return false;
    }

    data = 0;  /* sanity check */
    if (ps2kbd_cmd(KBD_CMD_SCANCODE, &data, 1)) {
        ps2kbd_reset();
        return false;
    }
    if (ps2_read() != 1) return false;

    return true;
}

static void kbd_interrupt(void)
{
    static bool e0 = false;
    static bool e1 = false;
    static bool brk = false;
    static int num_ack = 0;
    uint8_t sc, vk, ch, led;

    ps2_cmd(PS2_CMD_P1OFF);

readsc:
    sc = ps2_read();
    switch (sc)
    {
        case 0xFA:
            if (num_ack == 0) {
                kprintf("ps2kbd: got unexpected ACK\n");
            }
            --num_ack;
            goto done;
        case 0xFE:
            kprintf("ps2kbd: please resend command\n");
            goto done;
        case 0xFF:
        case 0x00:
            kprintf("ps2kbd: error %02X", sc);
            goto done;
    }

    if (m_mode == KB_RAW) {
        putq(sc);
        goto done;
    }
    if (sc == 0xE0) {
        e0 = true;
        goto done;
    }
    if (sc == 0xE1) {
        e1 = true;
        goto done;
    }

    if (sc >= 0x80) {
        brk = true;
        sc ^= 0x80;
    }

    vk = (e0) ? SC1_E0[sc] : SC1[sc];
    if (e0) {
        e0 = false;
    }
    if (e1 && vk == VK_NUMLK) {
        e1 = false;
        vk = VK_PAUSE;
    }
    if (brk) {
        vk |= 0x80;
        brk = false;
    }

    if ((vk & 0x7F) == 0) {
        kprintf("ps2kbd: got unrecognized scancode: %02X\n", sc);
        goto done;
    }

    switch (vk & 0x7F)
    {
        case VK_NUMLK:
            m_num = (vk & 0x80) ? m_num : !m_num;
            break;
        case VK_CAPSLK:
            m_caps = (vk & 0x80) ? m_caps : !m_caps;
            break;
        case VK_SCRLK:
            m_scroll = (vk & 0x80) ? m_scroll : !m_scroll;
            break;
    }

    if (vk & 0x80) {
        keyup(vk & 0x7F);
    }
    else {
        keydown(vk & 0x7F);
        if (num_ack == 0) {
            led = (m_caps << 2) | (m_num << 1) | (m_scroll << 0);
            ps2_write(KBD_CMD_SETLED);
            ps2_write(led);
            num_ack = 2;
        }
    }

    m_shift = is_keydown(VK_LSHIFT) || is_keydown(VK_RSHIFT);
    m_ctrl = is_keydown(VK_LCTRL) || is_keydown(VK_RCTRL);
    m_alt = is_keydown(VK_LALT) || is_keydown(VK_RALT);
    m_super = is_keydown(VK_LSUPER) || is_keydown(VK_RSUPER);

    if (m_mode == KB_MEDIUMRAW) {
        putq(vk);
        goto done;
    }
    if (vk & 0x80) {
        goto done;
    }

    if (vk == VK_SYSRQ) {
        ps2_cmd(PS2_CMD_SYSRESET);
    }

    switch (vk)
    {
        case VK_NUMPAD0: if (!m_num) vk = VK_INSERT; break;
        case VK_NUMPAD1: if (!m_num) vk = VK_END;    break;
        case VK_NUMPAD2: if (!m_num) vk = VK_DOWN;   break;
        case VK_NUMPAD3: if (!m_num) vk = VK_PGDOWN; break;
        case VK_NUMPAD4: if (!m_num) vk = VK_LEFT;   break;
        case VK_NUMPAD5: if (!m_num) vk = VK_NONE;   break;
        case VK_NUMPAD6: if (!m_num) vk = VK_RIGHT;  break;
        case VK_NUMPAD7: if (!m_num) vk = VK_HOME;   break;
        case VK_NUMPAD8: if (!m_num) vk = VK_UP;     break;
        case VK_NUMPAD9: if (!m_num) vk = VK_PGDOWN; break;
        case VK_DECIMAL: if (!m_num) vk = VK_DELETE; break;
    }

    ch = (m_shift) ? CH_SHIFT[vk] : CH[vk];
    if (ch == 0x00) {
        goto done;
    }

    if (ch == 0xE0) {
        kbd_putqs("\033[");
        switch (vk) {
            case VK_UP: kbd_putq('A'); break;
            case VK_DOWN: kbd_putq('B'); break;
            case VK_RIGHT: kbd_putq('C'); break;
            case VK_LEFT: kbd_putq('D'); break;
            case VK_PAUSE: kbd_putq('P'); break;
            case VK_BREAK: kbd_putq('Q'); break;
            case VK_SYSRQ: kbd_putq('R'); break;
            case VK_F1: kbd_putqs("1~"); break;
            case VK_F2: kbd_putqs("2~"); break;
            case VK_F3: kbd_putqs("3~"); break;
            case VK_F4: kbd_putqs("4~"); break;
            case VK_F5: kbd_putqs("5~"); break;
            case VK_F6: kbd_putqs("6~"); break;
            case VK_F7: kbd_putqs("7~"); break;
            case VK_F8: kbd_putqs("8~"); break;
            case VK_F9: kbd_putqs("9~"); break;
            case VK_F10: kbd_putqs("10~"); break;
            case VK_F11: kbd_putqs("11~"); break;
            case VK_F12: kbd_putqs("12~"); break;
            case VK_HOME: kbd_putqs("13~"); break;
            case VK_END: kbd_putqs("14~"); break;
            case VK_PGUP: kbd_putqs("15~"); break;
            case VK_PGDOWN: kbd_putqs("16~"); break;
            case VK_INSERT: kbd_putqs("17~"); break;
            case VK_DELETE: kbd_putqs("18~"); break;
            case VK_PRTSCN: kbd_putqs("19~"); break;
        }
        goto done;
    }

    if (m_ctrl) {
        if (ch >= 0x40 && ch < 0x60) {
            ch -= 0x40;
        }
        else if (ch >= 0x60 && ch < 0x7E) {
            ch -= 0x60;
        }
        else {
            switch (ch)
            {
                case '2': ch = ASCII_NUL; break;
                case '6': ch = ASCII_RS; break;
                case '-': ch = ASCII_US; break;
                case '/':
                case '?': ch = ASCII_DEL; break;
                default:  goto done;
            }
        }
    }

    if (m_caps) {
        if (isupper(ch)) {
            ch = tolower(ch);
        }
        else if (islower(ch)) {
            ch = toupper(ch);
        }
    }

    kbd_putq(ch);

done:
    if (ps2_canread()) {
        goto readsc;
    }
    ps2_cmd(PS2_CMD_P1ON);
}

static void kbd_putq(char c)
{
    if (m_echo) {
        if (iscntrl(c)) {
            printf("^");
            printf("%c", (c + 0x40) & 0x7F);
        }
        else {
            printf("%c", c);
        }
    }
    putq(c);
}

static void kbd_putqs(const char *s)
{
    char c;
    while ((c = *(s++)) != '\0') {
        kbd_putq(c);
    }
}

static const uint8_t SC1[128] =
{
/*00-07*/  0,VK_ESCAPE,VK_1,VK_2,VK_3,VK_4,VK_5,VK_6,
/*08-0F*/  VK_7,VK_8,VK_9,VK_0,VK_OEM1,VK_OEM2,VK_BACKSPACE,VK_TAB,
/*10-17*/  VK_Q,VK_W,VK_E,VK_R,VK_T,VK_Y,VK_U,VK_I,
/*18-1F*/  VK_O,VK_P,VK_OEM3,VK_OEM4,VK_RETURN,VK_LCTRL,VK_A,VK_S,
/*20-27*/  VK_D,VK_F,VK_G,VK_H,VK_J,VK_K,VK_L,VK_OEM5,
/*28-2F*/  VK_OEM6,VK_OEM7,VK_LSHIFT,VK_INT2,VK_Z,VK_X,VK_C,VK_V,
/*30-37*/  VK_B,VK_N,VK_M,VK_OEM8,VK_OEM9,VK_OEM10,VK_RSHIFT,VK_MULTIPLY,
/*38-3F*/  VK_LALT,VK_SPACE,VK_CAPSLK,VK_F1,VK_F2,VK_F3,VK_F4,VK_F5,
/*40-47*/  VK_F6,VK_F7,VK_F8,VK_F9,VK_F10,VK_NUMLK,VK_SCRLK,VK_NUMPAD7,
/*48-4F*/  VK_NUMPAD8,VK_NUMPAD9,VK_SUBTRACT,VK_NUMPAD4,VK_NUMPAD5,VK_NUMPAD6,VK_ADD,VK_NUMPAD1,
/*50-57*/  VK_NUMPAD2,VK_NUMPAD3,VK_NUMPAD0,VK_DECIMAL,VK_SYSRQ,0,VK_INT1,VK_F11,
/*58-5F*/  VK_F12,0,0,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  VK_KATAKANA,0,0,VK_INT3,0,0,0,VK_FURIGANA,
/*78-7F*/  0,VK_KANJI,0,VK_HIRAGANA,0,VK_INT4,VK_INT5,0,
};

static const uint8_t SC1_E0[128] =
{
/*00-07*/  0,0,0,0,0,0,0,0,
/*08-0F*/  0,0,0,0,0,0,0,0,
/*10-17*/  0,0,0,0,0,0,0,0,
/*18-1F*/  0,0,0,0,VK_ENTER,VK_RCTRL,0,0,
/*20-27*/  0,0,0,0,0,0,0,0,
/*28-2F*/  0,0,VK_LSHIFT,0,0,0,0,0,
/*30-37*/  0,0,0,0,0,VK_DIVIDE,VK_RSHIFT,VK_PRTSCN,
/*38-3F*/  VK_RALT,0,0,0,0,0,0,0,
/*40-47*/  0,0,0,0,0,0,VK_BREAK,VK_HOME,
/*48-4F*/  VK_UP,VK_PGUP,VK_DOWN,VK_LEFT,0,VK_RIGHT,0,VK_END,
/*50-57*/  VK_DOWN,VK_PGDOWN,VK_INSERT,VK_DELETE,0,0,0,0,
/*58-5F*/  0,0,0,VK_LSUPER,VK_RSUPER,VK_APPS,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const uint8_t CH[128] =
{
/*00-0F*/  0,'\033','1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
/*10-1F*/  'q','w','e','r','t','y','u','i','o','p','[',']','\r',0,'a','s',
/*20-2F*/  'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
/*30-3F*/  'b','n','m',',','.','/',0,'*',0,' ',0,0xE0,0xE0,0xE0,0xE0,0xE0,
/*40-4F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0,0,'7','8','9','-','4','5','6','+','1',
/*50-5F*/  '2','3','0','.',0xE0,0xE0,0,0xE0,0xE0,0,0,0,'\r',0,'/',0,
/*60-6F*/  0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,
/*70-7F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const uint8_t CH_SHIFT[128] =
{
/*00-0F*/  0,'\033','!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
/*10-1F*/  'Q','W','E','R','T','Y','U','I','O','P','{','}','\r',0,'A','S',
/*20-2F*/  'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
/*30-3F*/  'B','N','M','<','>','?',0,'*',0,' ',0,0xE0,0xE0,0xE0,0xE0,0xE0,
/*40-4F*/  0xE0,0xE0,0xE0,0xE0,0xE0,0,0,'7','8','9','-','4','5','6','+','1',
/*50-5F*/  '2','3','0','.',0xE0,0xE0,0,0xE0,0xE0,0,0,0,'\r',0,'/',0,
/*60-6F*/  0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0,0,0,0,
/*70-7F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
