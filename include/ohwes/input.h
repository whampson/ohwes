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
 *    File: include/ohwes/input.h                                             *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

/**
 * ASCII Control Characters
 */
enum ascii_cntl
{
    ASCII_NUL,          /* Null */
    ASCII_SOH,          /* Start of Heading */
    ASCII_STX,          /* Start of Text */
    ASCII_ETX,          /* End of Text */
    ASCII_EOT,          /* End of Transmission */
    ASCII_ENQ,          /* Enquiry */
    ASCII_ACK,          /* Acknowledgement */
    ASCII_BEL,          /* Bell */
    ASCII_BS,           /* Backspace */
    ASCII_HT,           /* Horizontal Tab */
    ASCII_LF,           /* Line Feed */
    ASCII_VT,           /* Vertical Tab */
    ASCII_FF,           /* Form Feed */
    ASCII_CR,           /* Carriage Return */
    ASCII_SO,           /* Shift Out */
    ASCII_SI,           /* Shift In */
    ASCII_DLE,          /* Data Link Escape */
    ASCII_DC1,          /* Device Control 1 (XON) */
    ASCII_DC2,          /* Device Control 2 */
    ASCII_DC3,          /* Device Control 3 (XOFF) */
    ASCII_DC4,          /* Device Control 4 */
    ASCII_NAK,          /* Negative Acknowledgement */
    ASCII_SYN,          /* Synchronous Idle */
    ASCII_ETB,          /* End of Transmission Block */
    ASCII_CAN,          /* Cancel */
    ASCII_EM,           /* End of Medium */
    ASCII_SUB,          /* Substitute */
    ASCII_ESC,          /* Escape */
    ASCII_FS,           /* File Separator */
    ASCII_GS,           /* Group Separator */
    ASCII_RS,           /* Record Separator */
    ASCII_US,           /* Unit Separator */
    ASCII_DEL = 0x7F    /* Delete */
};

/**
 * Keyboard Virtual Key Codes.
 */
enum virtual_key
{
    VK_LCTRL        = 1,
    VK_RCTRL,
    VK_LSHIFT,
    VK_RSHIFT,
    VK_LALT,
    VK_RALT,
    VK_BACKSPACE    = ASCII_BS,
    VK_TAB          = ASCII_HT,
    VK_RETURN       = ASCII_CR,
    VK_NUMLK,
    VK_CAPSLK,
    VK_SCRLK,
    VK_ESCAPE       = ASCII_ESC,
    VK_SPACE        = ' ',
    VK_OEM1         = '\'',
    VK_OEM2         = ',',
    VK_OEM3         = '-',
    VK_OEM4         = '.',
    VK_OEM5         = '/',
    VK_0            = '0',
    VK_1            = '1',
    VK_2            = '2',
    VK_3            = '3',
    VK_4            = '4',
    VK_5            = '5',
    VK_6            = '6',
    VK_7            = '7',
    VK_8            = '8',
    VK_9            = '9',
    VK_OEM6         = ';',
    VK_OEM7         = '=',
    VK_OEM8         = '[',
    VK_OEM9         = '\\',
    VK_OEM10        = ']',
    VK_OEM11        = '`',
    VK_A            = 'a',
    VK_B            = 'b',
    VK_C            = 'c',
    VK_D            = 'd',
    VK_E            = 'e',
    VK_F            = 'f',
    VK_G            = 'g',
    VK_H            = 'h',
    VK_I            = 'i',
    VK_J            = 'j',
    VK_K            = 'k',
    VK_L            = 'l',
    VK_M            = 'm',
    VK_N            = 'n',
    VK_O            = 'o',
    VK_P            = 'p',
    VK_Q            = 'q',
    VK_R            = 'r',
    VK_S            = 's',
    VK_T            = 't',
    VK_U            = 'u',
    VK_V            = 'v',
    VK_W            = 'w',
    VK_X            = 'x',
    VK_Y            = 'y',
    VK_Z            = 'z',
    VK_DELETE       = ASCII_DEL,
    VK_INSERT       = 0x80,
    VK_END          = 0x81,
    VK_DOWN         = 0x82,
    VK_PGDOWN       = 0x83,
    VK_LEFT         = 0x84,
    VK_RIGHT        = 0x86,
    VK_HOME         = 0x87,
    VK_UP           = 0x88,
    VK_PGUP         = 0x89,
    VK_MULTIPLY,
    VK_DIVIDE,
    VK_ADD,
    VK_SUBTRACT,
    VK_ENTER,
    VK_DECIMAL,
    VK_NUMPAD0      = 0x90,
    VK_NUMPAD1      = 0x91,
    VK_NUMPAD2      = 0x92,
    VK_NUMPAD3      = 0x93,
    VK_NUMPAD4      = 0x94,
    VK_NUMPAD5      = 0x95,
    VK_NUMPAD6      = 0x96,
    VK_NUMPAD7      = 0x97,
    VK_NUMPAD8      = 0x98,
    VK_NUMPAD9      = 0x99,
    VK_PRTSCN,
    VK_SYSRQ,
    VK_PAUSE,
    VK_BREAK,
    VK_LMETA,
    VK_RMETA,
    VK_APPLICATION,
    VK_F1           = 0xA1,
    VK_F2           = 0xA2,
    VK_F3           = 0xA3,
    VK_F4           = 0xA4,
    VK_F5           = 0xA5,
    VK_F6           = 0xA6,
    VK_F7           = 0xA7,
    VK_F8           = 0xA8,
    VK_F9           = 0xA9,
    VK_F10          = 0xAA,
    VK_F11          = 0xAB,
    VK_F12          = 0xAC,
    VK_F13          = 0xAD,
    VK_F14          = 0xAE,
    VK_F15          = 0xAD,
    VK_F16          = 0xB0,
    VK_F17          = 0xB1,
    VK_F18          = 0xB2,
    VK_F19          = 0xB3,
    VK_F20          = 0xB4,
    VK_F21          = 0xB5,
    VK_F22          = 0xB6,
    VK_F23          = 0xB7,
    VK_F24          = 0xB8,
    VK_OEM12        = 0xC0,
    /* 0xC0-0xFF: additional OEM Keys */
};

typedef uint8_t vk_t;
typedef uint16_t keystroke_t;

struct keystroke
{
    union {
        struct {
            struct {
                uint8_t down        : 1;
                uint8_t ctrl        : 1;
                uint8_t shift       : 1;
                uint8_t alt         : 1;
                uint8_t meta        : 1;
                uint8_t num_lock    : 1;
                uint8_t caps_lock   : 1;
                uint8_t scroll_lock : 1;
            };
            vk_t key;
        };
        keystroke_t stroke;
    };
};
_Static_assert(sizeof(struct keystroke) == 2, "sizeof(struct keystroke)");

bool keydown(vk_t key);
void get_keystroke(struct keystroke *ks);

void use_scancode_set1(void);
void use_scancode_set2(void);
void use_scancode_set3(void);

static inline bool is_ctrl_key(vk_t key)
{
    return key == VK_LCTRL || key == VK_RCTRL;
}

static inline bool is_shift_key(vk_t key)
{
    return key == VK_LSHIFT || key == VK_RSHIFT;
}

static inline bool is_alt_key(vk_t key)
{
    return key == VK_LALT || key == VK_RALT;
}

static inline bool is_meta_key(vk_t key)
{
    return key == VK_LMETA || key == VK_RMETA;
}

static inline bool is_modifier_key(vk_t key)
{
    return is_ctrl_key(key)
        || is_shift_key(key)
        || is_alt_key(key)
        || is_meta_key(key);
}

static inline bool is_lock_key(vk_t key)
{
    return key == VK_SCRLK || key == VK_CAPSLK || key == VK_NUMLK;
}

#endif /* __INPUT_H */
