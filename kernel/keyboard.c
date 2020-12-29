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

#include <ctype.h>
#include <string.h>
#include <ohwes/debug.h>
#include <ohwes/kernel.h>
#include <ohwes/keyboard.h>
#include <ohwes/irq.h>
#include <drivers/ps2.h>

#define EX0_CODE    0xE0    /* scancode sets 1 & 2 extension marker */
#define EX1_CODE    0xE1    /* scancode set 1 additional extension marker; only appears in Pause sequence AFAIK */
#define BRK_CODE    0xF0    /* scancode set 3 break marker */
#define BRK_MASK    0x80    /* scancode set 1 break mask; XOR to get scancode */

#define KBD_BUFLEN  128

static const uint8_t SC1[128];
static const uint8_t SC1_EX0[128];
static const uint8_t SC2[256];
static const uint8_t SC2_EX0[256];
static const uint8_t SC3[256];
static const uint8_t *SC_MAP[] = { SC1, SC1_EX0, SC2, SC2_EX0, SC3, SC3 };

static bool m_num = false;
static bool m_caps = false;
static bool m_scroll = false;
static uint8_t m_mode = KB_RAW;
static uint8_t m_scancode_set = 2;
static uint64_t m_keydown_map[2] = { 0 };
static char m_q[KBD_BUFLEN];
static size_t m_qhead = 0;
static size_t m_qtail = 0;
static size_t m_qlen = 0;
static bool m_qfull = false;

#define is_sc1()        (m_scancode_set==1)
#define is_sc2()        (m_scancode_set==2)
#define is_sc3()        (m_scancode_set==3)
#define sc_map(x)       (SC_MAP[((m_scancode_set-1)<<1)+x])

#define keydown(vk)     (m_keydown_map[vk/64]|= (1ULL<<(vk%64)))
#define keyup(vk)       (m_keydown_map[vk/64]&=~(1ULL<<(vk%64)))
#define is_keydown(vk)  (m_keydown_map[vk/64]&  (1ULL<<(vk%64)))

static void switch_scancode(int set);
static void kbd_interrupt(void);
static void kbd_putq(char c);

void kbd_init(void)
{
    ps2_init();
    if (!ps2_testctl()) panic("PS/2 controller self-test failed!");
    if (!ps2_testp1()) panic("PS/2 port 1 self-test failed!");
    if (!ps2_testp2()) panic("PS/2 port 1 self-test failed!");

    ps2kbd_init();
    if (!ps2kbd_test()) panic("keyboard self-test failed!");

    switch_scancode(1);

    irq_register_handler(IRQ_KEYBOARD, kbd_interrupt);
    irq_unmask(IRQ_KEYBOARD);
}

bool key_pressed(vk_t key)
{
    return is_keydown(key);
}

ssize_t kbd_read(char *buf, size_t n)
{
    ssize_t len, chunk_len;

    if (n > m_qlen) {
        n = m_qlen;
    }

    len = 0;
    if (m_qtail + n > KBD_BUFLEN) {
        chunk_len = KBD_BUFLEN - m_qtail;
        memcpy(buf, &m_q[m_qtail], chunk_len);
        len += chunk_len;
        m_qtail = 0;
    }

    chunk_len = n - len;
    memcpy(((char *) buf) + len, &m_q[m_qtail], chunk_len);
    len += chunk_len;
    m_qtail += chunk_len;
    m_qlen -= len;

    if (m_qfull && len > 0) {
        m_qfull = false;
    }

    return len;
}

static void kbd_putq(char c)
{
    if (m_qfull) {
        return;
    }

    m_q[m_qhead++] = c;
    m_qlen++;

    if (m_qhead >= KBD_BUFLEN) {
        m_qhead = 0;
    }

    if (m_qhead == m_qtail) {
        m_qfull = true;
    }
}

static void switch_scancode(int set)
{
    uint8_t res;

    m_scancode_set = set;
    ps2kbd_cmd(KBD_CMD_SCANCODE, &m_scancode_set, 1);
    res = 0;  /* sanity check */
    ps2kbd_cmd(KBD_CMD_SCANCODE, &res, 1);
    if (ps2_inb() != m_scancode_set) {
        panic("failed to switch to scancode set %d!", m_scancode_set);
    }
    if (m_scancode_set == 3) {
        ps2kbd_cmd(KBD_CMD_ALL_MBTR, NULL, 0);
    }
}

static void kbd_interrupt(void)
{
    static bool ex0 = false;
    static bool ex1 = false;
    static bool brk = false;
    uint8_t sc, vk;

    sc = ps2_inb();
    if (m_mode == KB_RAW) {
        kbd_putq(sc);
        return;
    }

    if (is_sc1()) {
        if (sc == EX0_CODE) {
            ex0 = true;
            return;
        }
        if (sc == EX1_CODE) {
            ex1 = true;
            return;
        }
        if (sc >= BRK_MASK) {
            brk = true;
            sc ^= BRK_MASK;
        }
    }
    if (is_sc2()) {
        if (sc == EX0_CODE) {
            ex0 = true;
            return;
        }
        if (sc == EX1_CODE) {
            ex1 = true;
            return;
        }
        if (sc == BRK_CODE) {
            brk = true;
            return;
        }
    }
    if (is_sc3()) {
        if (sc == BRK_CODE) {
            brk = true;
            return;
        }
    }

    vk = sc_map(ex0)[sc];
    if (ex0) {
        ex0 = false;
    }
    if (ex1 && vk == VK_NUMLK) {
        ex1 = false;
        vk = VK_PAUSE;
    }
    if (vk == 0) {
        dbgprintf("Unrecognized scancode %02X!\n", sc);
        return;
    }

    switch (vk)
    {
        case VK_NUMLK:
            m_num = (!brk) ? !m_num : m_num;
            break;
        case VK_CAPSLK:
            m_caps = (!brk) ? !m_caps : m_caps;
            break;
        case VK_SCRLK:
            m_scroll = (!brk) ? !m_scroll : m_scroll;
            break;
    }

    if (!brk) {
        keydown(vk);
    }
    else {
        keyup(vk);
        vk |= KB_KEYUP;
        brk = false;
    }

    if (m_mode == KB_TRANSLATE) {
        kbd_putq(vk);
        return;
    }

    /* TODO: ASCII */
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
/*50-57*/  VK_NUMPAD2,VK_NUMPAD3,VK_NUMPAD0,VK_DECIMAL,VK_SYSRQ,0,0,VK_F11,
/*58-5F*/  VK_F12,0,0,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const uint8_t SC1_EX0[128] =
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

static const uint8_t SC2[256] =
{
// /*00-07*/  0,VK_F9,0,VK_F5,VK_F3,VK_F1,VK_F2,VK_F12,
// /*08-0F*/  VK_F17,VK_F10,VK_F8,VK_F6,VK_F4,VK_TAB,VK_OEM11,0,
// /*10-17*/  VK_F18,VK_LALT,VK_LSHIFT,0,VK_LCTRL,VK_Q,VK_1,0,
// /*18-1F*/  VK_F19,0,VK_Z,VK_S,VK_A,VK_W,VK_2,VK_F13,
// /*20-27*/  VK_F20,VK_C,VK_X,VK_D,VK_E,VK_4,VK_3,VK_F14,
// /*28-2F*/  VK_F21,VK_SPACE,VK_V,VK_F,VK_T,VK_R,VK_5,VK_F15,
// /*30-37*/  VK_F22,VK_N,VK_B,VK_H,VK_G,VK_Y,VK_6,0,
// /*38-3F*/  VK_F23,0,VK_M,VK_J,VK_U,VK_7,VK_8,0,
// /*40-47*/  VK_F24,VK_OEM2,VK_K,VK_I,VK_O,VK_0,VK_9,0,
// /*48-4F*/  0,VK_OEM4,VK_OEM5,VK_L,VK_OEM6,VK_P,VK_OEM3,0,
// /*50-57*/  0,0,VK_OEM1,0,VK_OEM8,VK_OEM7,0,0,
// /*58-5F*/  VK_CAPSLK,VK_RSHIFT,VK_RETURN,VK_OEM10,0,VK_OEM9,VK_F16,0,
// /*60-67*/  0,0,0,0,0,0,VK_BACKSPACE,0,
// /*68-6F*/  0,VK_NUMPAD1,0,VK_NUMPAD4,VK_NUMPAD7,0,0,0,
// /*70-77*/  VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_ESCAPE,VK_NUMLK,
// /*78-7F*/  VK_F11,VK_ADD,VK_NUMPAD3,VK_SUBTRACT,VK_MULTIPLY,VK_NUMPAD9,VK_SCRLK,0,
// /*80-87*/  0,0,0,VK_F7,VK_SYSRQ,0,0,0,
// /*88-8F*/  0,0,0,0,0,0,0,0,
// /*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const uint8_t SC2_EX0[256] =
{
// /*00-07*/  0,0,0,0,0,0,0,0,
// /*08-0F*/  0,0,0,0,0,0,0,0,
// /*10-17*/  0,VK_RALT,VK_LSHIFT,0,VK_RCTRL,0,0,0,
// /*18-1F*/  0,0,0,0,0,0,0,VK_LMETA,
// /*20-27*/  0,0,0,0,0,0,0,VK_RMETA,
// /*28-2F*/  0,0,0,0,0,0,0,VK_APPLICATION,
// /*30-37*/  0,0,0,0,0,0,0,0,
// /*38-3F*/  0,0,0,0,0,0,0,0,
// /*40-47*/  0,0,0,0,0,0,0,0,
// /*48-4F*/  0,0,VK_DIVIDE,0,0,0,0,0,
// /*50-57*/  0,0,0,0,0,0,0,0,
// /*58-5F*/  0,0,VK_ENTER,0,0,0,0,0,
// /*60-67*/  0,0,0,0,0,0,0,0,
// /*68-6F*/  0,VK_END,0,VK_LEFT,VK_HOME,0,0,0,
// /*70-77*/  VK_INSERT,VK_DELETE,VK_DOWN,0,VK_RIGHT,VK_UP,0,VK_PAUSE,
// /*78-7F*/  0,0,VK_PGDOWN,0,VK_PRTSCN,VK_PGUP,VK_BREAK,0,
// /*80-87*/  0,0,0,VK_SYSRQ,0,0,0,0,
// /*88-8F*/  0,0,0,0,0,0,0,0,
// /*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const uint8_t SC3[256] =
{
// /*00-07*/  0,0,0,0,0,0,0,VK_F1,
// /*08-0F*/  VK_ESCAPE,0,0,0,0,VK_TAB,VK_OEM11,VK_F2,
// /*10-17*/  0,VK_LCTRL,VK_LSHIFT,0,VK_CAPSLK,VK_Q,VK_1,VK_F3,
// /*18-1F*/  0,VK_LALT,VK_Z,VK_S,VK_A,VK_W,VK_2,VK_F4,
// /*20-27*/  0,VK_C,VK_X,VK_D,VK_E,VK_4,VK_3,VK_F5,
// /*28-2F*/  0,VK_SPACE,VK_V,VK_F,VK_T,VK_R,VK_5,VK_F6,
// /*30-37*/  0,VK_N,VK_B,VK_H,VK_G,VK_Y,VK_6,VK_F7,
// /*38-3F*/  0,VK_RALT,VK_M,VK_J,VK_U,VK_7,VK_8,VK_F8,
// /*40-47*/  0,VK_OEM2,VK_K,VK_I,VK_O,VK_0,VK_9,VK_F9,
// /*48-4F*/  0,VK_OEM4,VK_OEM5,VK_L,VK_OEM6,VK_P,VK_OEM3,VK_F10,
// /*50-57*/  0,0,VK_OEM1,VK_OEM9,VK_OEM8,VK_OEM7,VK_F11,VK_PRTSCN,
// /*58-5F*/  VK_RCTRL,VK_RSHIFT,VK_RETURN,VK_OEM10,VK_OEM9,0,VK_F12,VK_SCRLK,
// /*60-67*/  VK_DOWN,VK_LEFT,VK_PAUSE,VK_UP,VK_DELETE,VK_END,VK_BACKSPACE,VK_INSERT,
// /*68-6F*/  0,VK_NUMPAD1,VK_RIGHT,VK_NUMPAD4,VK_NUMPAD7,VK_PGDOWN,VK_HOME,VK_PGUP,
// /*70-77*/  VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_NUMLK,VK_DIVIDE,
// /*78-7F*/  0,VK_ENTER,VK_NUMPAD3,0,VK_ADD,VK_NUMPAD9,VK_MULTIPLY,0,
// /*80-87*/  0,0,0,0,VK_SUBTRACT,0,0,0,
// /*88-8F*/  0,0,0,VK_LMETA,VK_RMETA,VK_APPLICATION,0,0,
// /*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
// /*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};