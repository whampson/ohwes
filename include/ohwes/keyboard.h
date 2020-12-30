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
 *    File: include/ohwes/keyboard.h                                          *
 * Created: December 28, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>
#include <types.h>

/**
 * Keyboard Modes
 */
enum kb_mode
{
    KB_RAW,         /* Raw scancodes only. */
    KB_MEDIUMRAW,   /* Translate scancodes into virtual keycodes. */
    KB_COOKED       /* Translate keycodes into ASCII character sequences. */
};

int kbd_getmode(void);
bool kbd_setmode(int mode);

/**
 * Keyboard Virtual Key Codes
 */
enum virtual_key
{
    /* DO NOT EDIT!
       unless you want to update the translaton maps
       or the key classificaton functions */

    /* Scancode Set 1 */
    VK_NONE,
    VK_ESCAPE,
    VK_1,
    VK_2,
    VK_3,
    VK_4,
    VK_5,
    VK_6,
    VK_7,
    VK_8,
    VK_9,
    VK_0,
    VK_OEM1,
    VK_OEM2,
    VK_BACKSPACE,
    VK_TAB,
    VK_Q,
    VK_W,
    VK_E,
    VK_R,
    VK_T,
    VK_Y,
    VK_U,
    VK_I,
    VK_O,
    VK_P,
    VK_OEM3,
    VK_OEM4,
    VK_RETURN,
    VK_LCTRL,
    VK_A,
    VK_S,
    VK_D,
    VK_F,
    VK_G,
    VK_H,
    VK_J,
    VK_K,
    VK_L,
    VK_OEM5,
    VK_OEM6,
    VK_OEM7,
    VK_LSHIFT,
    VK_INT2,
    VK_Z,
    VK_X,
    VK_C,
    VK_V,
    VK_B,
    VK_N,
    VK_M,
    VK_OEM8,
    VK_OEM9,
    VK_OEM10,
    VK_RSHIFT,
    VK_MULTIPLY,
    VK_LALT,
    VK_SPACE,
    VK_CAPSLK,
    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_F5,
    VK_F6,
    VK_F7,
    VK_F8,
    VK_F9,
    VK_F10,
    VK_NUMLK,
    VK_SCRLK,
    VK_NUMPAD7,
    VK_NUMPAD8,
    VK_NUMPAD9,
    VK_SUBTRACT,
    VK_NUMPAD4,
    VK_NUMPAD5,
    VK_NUMPAD6,
    VK_ADD,
    VK_NUMPAD1,
    VK_NUMPAD2,
    VK_NUMPAD3,
    VK_NUMPAD0,
    VK_DECIMAL,
    VK_SYSRQ,
    VK_BREAK,
    VK_INT1,
    VK_F11,
    VK_F12,
    VK_LSUPER,
    VK_RSUPER,
    VK_APPS,
    VK_ENTER,
    VK_RCTRL,
    VK_DIVIDE,
    VK_PRTSCN,
    VK_RALT,
    VK_PAUSE,
    VK_HOME,
    VK_UP,
    VK_PGUP,
    VK_LEFT,
    VK_RIGHT,
    VK_END,
    VK_DOWN,
    VK_PGDOWN,
    VK_INSERT,
    VK_DELETE,
    /* 0x6B-0x6F */
    VK_KATAKANA     = 0x70,
    /* 0x71-0x72 */
    VK_INT3         = 0x73,
    /* 0x74-0x76 */
    VK_FURIGANA     = 0x77,
    /* 0x78 */
    VK_KANJI        = 0x79,
    /* 0x7A */
    VK_HIRAGANA     = 0x7B,
    /* 0x7C */
    VK_INT4         = 0x7D,
    VK_INT5         = 0x7E,
    /* 0x7F */
};

typedef uint8_t vk_t;

static inline bool shift_key(vk_t key)
{
    return key == VK_LSHIFT || key == VK_RSHIFT;
}

static inline bool ctrl_key(vk_t key)
{
    return key == VK_LCTRL || key == VK_RCTRL;
}

static inline bool alt_key(vk_t key)
{
    return key == VK_LALT || key == VK_RALT;
}

static inline bool modifier_key(vk_t key)
{
    return shift_key(key) || ctrl_key(key) || alt_key(key);
}

static inline bool super_key(vk_t key)
{
    return key == VK_LSUPER || key == VK_RSUPER;
}

static inline bool system_key(vk_t key)
{
    return super_key(key)
        || key == VK_PRTSCN || key == VK_SYSRQ
        || key == VK_PAUSE || key == VK_BREAK
        || key == VK_ESCAPE;
}

static inline bool function_key(vk_t key)
{
    return (key >= VK_F1 && key <= VK_F10)
        || (key >= VK_F11 && key <= VK_F12);
}

static inline bool arrow_key(vk_t key)
{
    return key == VK_LEFT || key == VK_RIGHT
        || key == VK_UP || key == VK_DOWN;
}

static inline bool navigation_key(vk_t key)
{
    return arrow_key(key)
        || key == VK_HOME || key == VK_END
        || key == VK_PGUP || key == VK_PGDOWN
        || key == VK_TAB;
}

static inline bool editing_key(vk_t key)
{
    return key == VK_ENTER || key == VK_RETURN
        || key == VK_INSERT || key == VK_DELETE
        || key == VK_BACKSPACE;
}

static inline bool lock_key(vk_t key)
{
    return key == VK_NUMLK || key == VK_CAPSLK || key == VK_SCRLK;
}

static inline bool numpad_key(vk_t key)
{
    return (key >= VK_NUMPAD7 && key <= VK_DECIMAL)
        || (key == VK_MULTIPLY || key == VK_DIVIDE);
}

bool key_pressed(vk_t key);

ssize_t kbd_read(char *buf, size_t n);

#endif /* __KEYBOARD_H */
