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
#include <stddef.h>
#include <inttypes.h>
// #include <types.h>
#include <vk.h>

typedef int32_t ssize_t;        // TEMP for keyboard to work

/**
 * Keyboard Modes
 */
enum kb_mode
{
    KB_RAW,         /* Emit raw scancodes only. */
    KB_MEDIUMRAW,   /* Translate scancodes into virtual keycodes. */
    KB_COOKED       /* Translate keycodes into ASCII character sequences. */
};

// ssize_t kbd_read(char *buf, size_t n);


/* TODO: make these into ioctls */

int kbd_getmode(void);
bool kbd_setmode(int mode);
void kbd_setecho(bool enabled);
void kbd_flush(void);
bool key_down(vk_t key);
bool ctrl_down(void);
bool shift_down(void);
bool alt_down(void);
bool super_down(void);
bool capslock(void);
bool numlock(void);
bool scrlock(void);

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

#endif /* __KEYBOARD_H */
