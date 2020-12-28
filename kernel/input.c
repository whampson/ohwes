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
 *    File: kernel/input.c                                                    *
 * Created: December 24, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <ctype.h>
#include <ohwes/kernel.h>
#include <ohwes/input.h>
#include <ohwes/irq.h>
#include <drivers/ps2.h>

static const uint8_t SC1[128];
static const uint8_t SC1_EX0[128];
static const uint8_t SC2[256];
static const uint8_t SC2_EX0[256];
static const uint8_t SC3[256];
static const uint8_t *SC_MAP[] = { SC1, SC1_EX0, SC2, SC2_EX0, SC3, SC3 };
static uint8_t m_scancode_set;

static uint64_t m_keypress_map[4] = { 0 };
static struct keystroke m_keystroke = { 0 };

#define EX0_CODE    0xE0    /* scancode sets 1 & 2 extension marker */
#define EX1_CODE    0xE1    /* scancode set 1 additional extension marker; only appears in Pause sequence AFAIK */
#define BRK_CODE    0xF0    /* scancode set 3 break marker */
#define BRK_MASK    0x80    /* scancode set 1 break mask; XOR to get scancode */

#define is_sc1()    (m_scancode_set==1)
#define is_sc2()    (m_scancode_set==2)
#define is_sc3()    (m_scancode_set==3)
#define sc_map(x)   (SC_MAP[((m_scancode_set-1)<<1)+x])

#define set_keydown(vk)     (m_keypress_map[vk/64]|= (1ULL<<(vk%64)))
#define clear_keydown(vk)   (m_keypress_map[vk/64]&=~(1ULL<<(vk%64)))
#define is_keydown(vk)      (m_keypress_map[vk/64]&  (1ULL<<(vk%64)))

static void switch_scancode(void);
static void keyboard_irq(void);

void input_init(void)
{
    ps2_init();
    if (!ps2_testctl()) panic("PS/2 controller self-test failed!");
    if (!ps2_testp1()) panic("PS/2 port 1 self-test failed!");
    if (!ps2_testp2()) panic("PS/2 port 1 self-test failed!");

    kbd_init();
    if (!kbd_test()) panic("keyboard self-test failed!");

    use_scancode_set2();
    kbd_cmd(KBD_CMD_SCANON, NULL, 0);

    irq_register_handler(IRQ_KEYBOARD, keyboard_irq);
    irq_unmask(IRQ_KEYBOARD);
}

bool keydown(vk_t key)
{
    return is_keydown(key);
}

void get_keystroke(struct keystroke *ks)
{
    *ks = m_keystroke;
}

void use_scancode_set1(void)
{
    m_scancode_set = 1;
    switch_scancode();
}

void use_scancode_set2(void)
{
    m_scancode_set = 2;
    switch_scancode();
}

void use_scancode_set3(void)
{
    m_scancode_set = 3;
    switch_scancode();
    kbd_cmd(KBD_CMD_ALL_MBTR, NULL, 0);
}

static void switch_scancode(void)
{
    uint8_t res;

    kbd_cmd(KBD_CMD_SCANCODE, &m_scancode_set, 1);
    res = 0;  /* sanity check */
    kbd_cmd(KBD_CMD_SCANCODE, &res, 1);
    if (ps2_inb() != m_scancode_set) {
        panic("failed to switch to scancode set %d!", m_scancode_set);
    }
}

static void keyboard_irq(void)
{
    static bool ex0 = false;
    static bool ex1 = false;
    static bool brk = false;
    static bool num = false;
    static bool caps = false;
    static bool scroll = false;
    uint8_t sc, vk;

    sc = ps2_inb();
    // kprintf("%02X ", sc);
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
    if (ex0) ex0 = false;
    if (vk == 0) {
        kprintf("Unrecognized scancode %02X!\n", sc);
        return;
    }

    if (!brk) {
        // kprintf("DN: ");
        set_keydown(vk);
    }
    else {
        // kprintf("UP: ");
        clear_keydown(vk);
    }

    if (ex1 && vk == VK_NUMLK) {
        ex1 = false;
        vk = VK_PAUSE;
    }

    switch (vk)
    {
        case VK_NUMLK:
            num = (!brk) ? !num : num;
            break;
        case VK_CAPSLK:
            caps = (!brk) ? !caps : caps;
            break;
        case VK_SCRLK:
            scroll = (!brk) ? !scroll : scroll;
            break;
    }

    m_keystroke.key = vk;
    m_keystroke.down = !brk;
    m_keystroke.ctrl = is_keydown(VK_LCTRL) || is_keydown(VK_RCTRL);
    m_keystroke.shift = is_keydown(VK_LSHIFT) || is_keydown(VK_RSHIFT);
    m_keystroke.alt = is_keydown(VK_LALT) || is_keydown(VK_RALT);
    m_keystroke.meta = is_keydown(VK_LMETA) || is_keydown(VK_RMETA);
    m_keystroke.num_lock = num;
    m_keystroke.caps_lock = caps;
    m_keystroke.scroll_lock = scroll;

    brk = false;

    // switch (vk) {
    //     case VK_LCTRL: kprintf("VK_LCTRL\n"); break;
    //     case VK_RCTRL: kprintf("VK_RCTRL\n"); break;
    //     case VK_LSHIFT: kprintf("VK_LSHIFT\n"); break;
    //     case VK_RSHIFT: kprintf("VK_RSHIFT\n"); break;
    //     case VK_LALT: kprintf("VK_LALT\n"); break;
    //     case VK_RALT: kprintf("VK_RALT\n"); break;
    //     case VK_BACKSPACE: kprintf("VK_BACKSPACE\n"); break;
    //     case VK_TAB: kprintf("VK_TAB\n"); break;
    //     case VK_RETURN: kprintf("VK_RETURN\n"); break;
    //     case VK_ENTER: kprintf("VK_ENTER\n"); break;
    //     case VK_LMETA: kprintf("VK_LMETA\n"); break;
    //     case VK_RMETA: kprintf("VK_RMETA\n"); break;
    //     case VK_APPLICATION: kprintf("VK_APPLICATION\n"); break;
    //     case VK_NUMLK: kprintf("VK_NUMLK\n"); break;
    //     case VK_CAPSLK: kprintf("VK_CAPSLK\n"); break;
    //     case VK_SCRLK: kprintf("VK_SCRLK\n"); break;
    //     case VK_ESCAPE: kprintf("VK_ESCAPE\n"); break;
    //     case VK_SPACE: kprintf("VK_SPACE\n"); break;
    //     case VK_OEM1: kprintf("VK_OEM1\n"); break;
    //     case VK_OEM2: kprintf("VK_OEM2\n"); break;
    //     case VK_OEM3: kprintf("VK_OEM3\n"); break;
    //     case VK_OEM4: kprintf("VK_OEM4\n"); break;
    //     case VK_OEM5: kprintf("VK_OEM5\n"); break;
    //     case VK_0: kprintf("VK_0\n"); break;
    //     case VK_1: kprintf("VK_1\n"); break;
    //     case VK_2: kprintf("VK_2\n"); break;
    //     case VK_3: kprintf("VK_3\n"); break;
    //     case VK_4: kprintf("VK_4\n"); break;
    //     case VK_5: kprintf("VK_5\n"); break;
    //     case VK_6: kprintf("VK_6\n"); break;
    //     case VK_7: kprintf("VK_7\n"); break;
    //     case VK_8: kprintf("VK_8\n"); break;
    //     case VK_9: kprintf("VK_9\n"); break;
    //     case VK_OEM6: kprintf("VK_OEM6\n"); break;
    //     case VK_OEM7: kprintf("VK_OEM7\n"); break;
    //     case VK_OEM8: kprintf("VK_OEM8\n"); break;
    //     case VK_OEM9: kprintf("VK_OEM9\n"); break;
    //     case VK_OEM10: kprintf("VK_OEM10\n"); break;
    //     case VK_OEM11: kprintf("VK_OEM11\n"); break;
    //     case VK_A: kprintf("VK_A\n"); break;
    //     case VK_B: kprintf("VK_B\n"); break;
    //     case VK_C: kprintf("VK_C\n"); break;
    //     case VK_D: kprintf("VK_D\n"); break;
    //     case VK_E: kprintf("VK_E\n"); break;
    //     case VK_F: kprintf("VK_F\n"); break;
    //     case VK_G: kprintf("VK_G\n"); break;
    //     case VK_H: kprintf("VK_H\n"); break;
    //     case VK_I: kprintf("VK_I\n"); break;
    //     case VK_J: kprintf("VK_J\n"); break;
    //     case VK_K: kprintf("VK_K\n"); break;
    //     case VK_L: kprintf("VK_L\n"); break;
    //     case VK_M: kprintf("VK_M\n"); break;
    //     case VK_N: kprintf("VK_N\n"); break;
    //     case VK_O: kprintf("VK_O\n"); break;
    //     case VK_P: kprintf("VK_P\n"); break;
    //     case VK_Q: kprintf("VK_Q\n"); break;
    //     case VK_R: kprintf("VK_R\n"); break;
    //     case VK_S: kprintf("VK_S\n"); break;
    //     case VK_T: kprintf("VK_T\n"); break;
    //     case VK_U: kprintf("VK_U\n"); break;
    //     case VK_V: kprintf("VK_V\n"); break;
    //     case VK_W: kprintf("VK_W\n"); break;
    //     case VK_X: kprintf("VK_X\n"); break;
    //     case VK_Y: kprintf("VK_Y\n"); break;
    //     case VK_Z: kprintf("VK_Z\n"); break;
    //     case VK_DELETE: kprintf("VK_DELETE\n"); break;
    //     case VK_INSERT: kprintf("VK_INSERT\n"); break;
    //     case VK_MULTIPLY: kprintf("VK_MULTIPLY\n"); break;
    //     case VK_DIVIDE: kprintf("VK_DIVIDE\n"); break;
    //     case VK_ADD: kprintf("VK_ADD\n"); break;
    //     case VK_SUBTRACT: kprintf("VK_SUBTRACT\n"); break;
    //     case VK_DECIMAL: kprintf("VK_DECIMAL\n"); break;
    //     case VK_HOME: kprintf("VK_HOME\n"); break;
    //     case VK_END: kprintf("VK_END\n"); break;
    //     case VK_PGUP: kprintf("VK_PGUP\n"); break;
    //     case VK_PGDOWN: kprintf("VK_PGDOWN\n"); break;
    //     case VK_LEFT: kprintf("VK_LEFT\n"); break;
    //     case VK_RIGHT: kprintf("VK_RIGHT\n"); break;
    //     case VK_UP: kprintf("VK_UP\n"); break;
    //     case VK_DOWN: kprintf("VK_DOWN\n"); break;
    //     case VK_PRTSCN: kprintf("VK_PRTSCN\n"); break;
    //     case VK_SYSRQ: kprintf("VK_SYSRQ\n"); break;
    //     case VK_PAUSE: kprintf("VK_PAUSE\n"); break;
    //     case VK_BREAK: kprintf("VK_BREAK\n"); break;
    //     case VK_NUMPAD0: kprintf("VK_NUMPAD0\n"); break;
    //     case VK_NUMPAD1: kprintf("VK_NUMPAD1\n"); break;
    //     case VK_NUMPAD2: kprintf("VK_NUMPAD2\n"); break;
    //     case VK_NUMPAD3: kprintf("VK_NUMPAD3\n"); break;
    //     case VK_NUMPAD4: kprintf("VK_NUMPAD4\n"); break;
    //     case VK_NUMPAD5: kprintf("VK_NUMPAD5\n"); break;
    //     case VK_NUMPAD6: kprintf("VK_NUMPAD6\n"); break;
    //     case VK_NUMPAD7: kprintf("VK_NUMPAD7\n"); break;
    //     case VK_NUMPAD8: kprintf("VK_NUMPAD8\n"); break;
    //     case VK_NUMPAD9: kprintf("VK_NUMPAD9\n"); break;
    //     case VK_F1: kprintf("VK_F1\n"); break;
    //     case VK_F2: kprintf("VK_F2\n"); break;
    //     case VK_F3: kprintf("VK_F3\n"); break;
    //     case VK_F4: kprintf("VK_F4\n"); break;
    //     case VK_F5: kprintf("VK_F5\n"); break;
    //     case VK_F6: kprintf("VK_F6\n"); break;
    //     case VK_F7: kprintf("VK_F7\n"); break;
    //     case VK_F8: kprintf("VK_F8\n"); break;
    //     case VK_F9: kprintf("VK_F9\n"); break;
    //     case VK_F10: kprintf("VK_F10\n"); break;
    //     case VK_F11: kprintf("VK_F11\n"); break;
    //     case VK_F12: kprintf("VK_F12\n"); break;
    // }
}

static const uint8_t SC1[128] =
{
/*00-07*/  0,VK_ESCAPE,VK_1,VK_2,VK_3,VK_4,VK_5,VK_6,
/*08-0F*/  VK_7,VK_8,VK_9,VK_0,VK_OEM3,VK_OEM7,VK_BACKSPACE,VK_TAB,
/*10-17*/  VK_Q,VK_W,VK_E,VK_R,VK_T,VK_Y,VK_U,VK_I,
/*18-1F*/  VK_O,VK_P,VK_OEM8,VK_OEM10,VK_RETURN,VK_LCTRL,VK_A,VK_S,
/*20-27*/  VK_D,VK_F,VK_G,VK_H,VK_J,VK_K,VK_L,VK_OEM6,
/*28-2F*/  VK_OEM1,VK_OEM11,VK_LSHIFT,VK_OEM9,VK_Z,VK_X,VK_C,VK_V,
/*30-37*/  VK_B,VK_N,VK_M,VK_OEM2,VK_OEM4,VK_OEM5,VK_RSHIFT,VK_MULTIPLY,
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
/*58-5F*/  0,0,0,VK_LMETA,VK_RMETA,VK_APPLICATION,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,0,0,0,0,0,0,0,
/*70-77*/  0,0,0,0,0,0,0,0,
/*78-7F*/  0,0,0,0,0,0,0,0,
};

static const uint8_t SC2[256] =
{
/*00-07*/  0,VK_F9,0,VK_F5,VK_F3,VK_F1,VK_F2,VK_F12,
/*08-0F*/  VK_F17,VK_F10,VK_F8,VK_F6,VK_F4,VK_TAB,VK_OEM11,0,
/*10-17*/  VK_F18,VK_LALT,VK_LSHIFT,0,VK_LCTRL,VK_Q,VK_1,0,
/*18-1F*/  VK_F19,0,VK_Z,VK_S,VK_A,VK_W,VK_2,VK_F13,
/*20-27*/  VK_F20,VK_C,VK_X,VK_D,VK_E,VK_4,VK_3,VK_F14,
/*28-2F*/  VK_F21,VK_SPACE,VK_V,VK_F,VK_T,VK_R,VK_5,VK_F15,
/*30-37*/  VK_F22,VK_N,VK_B,VK_H,VK_G,VK_Y,VK_6,0,
/*38-3F*/  VK_F23,0,VK_M,VK_J,VK_U,VK_7,VK_8,0,
/*40-47*/  VK_F24,VK_OEM2,VK_K,VK_I,VK_O,VK_0,VK_9,0,
/*48-4F*/  0,VK_OEM4,VK_OEM5,VK_L,VK_OEM6,VK_P,VK_OEM3,0,
/*50-57*/  0,0,VK_OEM1,0,VK_OEM8,VK_OEM7,0,0,
/*58-5F*/  VK_CAPSLK,VK_RSHIFT,VK_RETURN,VK_OEM10,0,VK_OEM9,VK_F16,0,
/*60-67*/  0,0,0,0,0,0,VK_BACKSPACE,0,
/*68-6F*/  0,VK_NUMPAD1,0,VK_NUMPAD4,VK_NUMPAD7,0,0,0,
/*70-77*/  VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_ESCAPE,VK_NUMLK,
/*78-7F*/  VK_F11,VK_ADD,VK_NUMPAD3,VK_SUBTRACT,VK_MULTIPLY,VK_NUMPAD9,VK_SCRLK,0,
/*80-87*/  0,0,0,VK_F7,VK_SYSRQ,0,0,0,
/*88-8F*/  0,0,0,0,0,0,0,0,
/*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const uint8_t SC2_EX0[256] =
{
/*00-07*/  0,0,0,0,0,0,0,0,
/*08-0F*/  0,0,0,0,0,0,0,0,
/*10-17*/  0,VK_RALT,VK_LSHIFT,0,VK_RCTRL,0,0,0,
/*18-1F*/  0,0,0,0,0,0,0,VK_LMETA,
/*20-27*/  0,0,0,0,0,0,0,VK_RMETA,
/*28-2F*/  0,0,0,0,0,0,0,VK_APPLICATION,
/*30-37*/  0,0,0,0,0,0,0,0,
/*38-3F*/  0,0,0,0,0,0,0,0,
/*40-47*/  0,0,0,0,0,0,0,0,
/*48-4F*/  0,0,VK_DIVIDE,0,0,0,0,0,
/*50-57*/  0,0,0,0,0,0,0,0,
/*58-5F*/  0,0,VK_ENTER,0,0,0,0,0,
/*60-67*/  0,0,0,0,0,0,0,0,
/*68-6F*/  0,VK_END,0,VK_LEFT,VK_HOME,0,0,0,
/*70-77*/  VK_INSERT,VK_DELETE,VK_DOWN,0,VK_RIGHT,VK_UP,0,VK_PAUSE,
/*78-7F*/  0,0,VK_PGDOWN,0,VK_PRTSCN,VK_PGUP,VK_BREAK,0,
/*80-87*/  0,0,0,VK_SYSRQ,0,0,0,0,
/*88-8F*/  0,0,0,0,0,0,0,0,
/*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const uint8_t SC3[256] =
{
/*00-07*/  0,0,0,0,0,0,0,VK_F1,
/*08-0F*/  VK_ESCAPE,0,0,0,0,VK_TAB,VK_OEM11,VK_F2,
/*10-17*/  0,VK_LCTRL,VK_LSHIFT,0,VK_CAPSLK,VK_Q,VK_1,VK_F3,
/*18-1F*/  0,VK_LALT,VK_Z,VK_S,VK_A,VK_W,VK_2,VK_F4,
/*20-27*/  0,VK_C,VK_X,VK_D,VK_E,VK_4,VK_3,VK_F5,
/*28-2F*/  0,VK_SPACE,VK_V,VK_F,VK_T,VK_R,VK_5,VK_F6,
/*30-37*/  0,VK_N,VK_B,VK_H,VK_G,VK_Y,VK_6,VK_F7,
/*38-3F*/  0,VK_RALT,VK_M,VK_J,VK_U,VK_7,VK_8,VK_F8,
/*40-47*/  0,VK_OEM2,VK_K,VK_I,VK_O,VK_0,VK_9,VK_F9,
/*48-4F*/  0,VK_OEM4,VK_OEM5,VK_L,VK_OEM6,VK_P,VK_OEM3,VK_F10,
/*50-57*/  0,0,VK_OEM1,VK_OEM9,VK_OEM8,VK_OEM7,VK_F11,VK_PRTSCN,
/*58-5F*/  VK_RCTRL,VK_RSHIFT,VK_RETURN,VK_OEM10,VK_OEM9,0,VK_F12,VK_SCRLK,
/*60-67*/  VK_DOWN,VK_LEFT,VK_PAUSE,VK_UP,VK_DELETE,VK_END,VK_BACKSPACE,VK_INSERT,
/*68-6F*/  0,VK_NUMPAD1,VK_RIGHT,VK_NUMPAD4,VK_NUMPAD7,VK_PGDOWN,VK_HOME,VK_PGUP,
/*70-77*/  VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_NUMLK,VK_DIVIDE,
/*78-7F*/  0,VK_ENTER,VK_NUMPAD3,0,VK_ADD,VK_NUMPAD9,VK_MULTIPLY,0,
/*80-87*/  0,0,0,0,VK_SUBTRACT,0,0,0,
/*88-8F*/  0,0,0,VK_LMETA,VK_RMETA,VK_APPLICATION,0,0,
/*90-9F*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A0-AF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B0-BF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C0-CF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*D0-DF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*E0-EF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*F0-FF*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};