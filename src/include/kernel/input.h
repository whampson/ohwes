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
 *         File: src/include/kernel/input.h
 *      Created: March 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __INPUT_H
#define __INPUT_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define is_ctrl(k)              (((k) == KEY_LCTRL || (k) == KEY_RCTRL))
#define is_shift(k)             (((k) == KEY_LSHIFT || (k) == KEY_RSHIFT))
#define is_alt(k)               (((k) == KEY_LALT || (k) == KEY_RALT))
#define is_meta(k)              (((k) == KEY_LWIN || (k) == KEY_RWIN))
#define is_kpnum(k)             ((k) >= KEY_KP0 && (k) <= KEY_KP9)
#define is_fnkey(k)             ((k) >= KEY_F1 && (k) <= KEY_F12)
#define is_sysrq(k)             ((k) == KEY_SYSRQ)

struct key_event
{
    uint16_t keycode;
    uint16_t scancode;
    bool release;
    char c;
};

//
// Virtual Key Code Definitions
//
// Below are the virtual key codes for a 104-key US keyboard. Key codes match
// the ASCII code of the symbol printed on the keyboard where possible.
//

#define KEY_NONE                0
#define KEY_LCTRL               1
#define KEY_RCTRL               2
#define KEY_LSHIFT              3
#define KEY_RSHIFT              4
#define KEY_LALT                5
#define KEY_RALT                6
#define KEY_BREAK               7
#define KEY_BACKSPACE           8   // matches ASCII
#define KEY_TAB                 9   // matches ASCII
#define KEY_ENTER               10  // matches ASCII
#define KEY_F1                  11
#define KEY_F2                  12
#define KEY_F3                  13
#define KEY_F4                  14
#define KEY_F5                  15
#define KEY_F6                  16
#define KEY_F7                  17
#define KEY_F8                  18
#define KEY_F9                  19
#define KEY_F10                 20
#define KEY_F11                 21
#define KEY_F12                 22
#define KEY_LWIN                23
#define KEY_RWIN                24
#define KEY_MENU                25
#define KEY_PAUSE               26
#define KEY_ESCAPE              27  // matches ASCII
#define KEY_SYSRQ               28
#define KEY_CAPSLK              29
#define KEY_NUMLK               30
#define KEY_SCRLK               31
#define KEY_SPACE               32  // matches ASCII
// 33-38 reserved
#define KEY_APOSTROPHE          39  // matches ASCII
// 40-41 reserved
#define KEY_KPASTERISK          42  // matches ASCII
#define KEY_KPPLUS              43  // matches ASCII
#define KEY_COMMA               44  // matches ASCII
#define KEY_MINUS               45  // matches ASCII
#define KEY_DOT                 46  // matches ASCII
#define KEY_SLASH               47  // matches ASCII
#define KEY_0                   48  // matches ASCII
#define KEY_1                   49  // matches ASCII
#define KEY_2                   50  // matches ASCII
#define KEY_3                   51  // matches ASCII
#define KEY_4                   52  // matches ASCII
#define KEY_5                   53  // matches ASCII
#define KEY_6                   54  // matches ASCII
#define KEY_7                   55  // matches ASCII
#define KEY_8                   56  // matches ASCII
#define KEY_9                   57  // matches ASCII
// 58 reserved
#define KEY_SEMICOLON           59  // matches ASCII
// 60 reserved
#define KEY_EQUAL               61  // matches ASCII
// 62-64 reserved
#define KEY_A                   65  // matches ASCII
#define KEY_B                   66  // matches ASCII
#define KEY_C                   67  // matches ASCII
#define KEY_D                   68  // matches ASCII
#define KEY_E                   69  // matches ASCII
#define KEY_F                   70  // matches ASCII
#define KEY_G                   71  // matches ASCII
#define KEY_H                   72  // matches ASCII
#define KEY_I                   73  // matches ASCII
#define KEY_J                   74  // matches ASCII
#define KEY_K                   75  // matches ASCII
#define KEY_L                   76  // matches ASCII
#define KEY_M                   77  // matches ASCII
#define KEY_N                   78  // matches ASCII
#define KEY_O                   79  // matches ASCII
#define KEY_P                   80  // matches ASCII
#define KEY_Q                   81  // matches ASCII
#define KEY_R                   82  // matches ASCII
#define KEY_S                   83  // matches ASCII
#define KEY_T                   84  // matches ASCII
#define KEY_U                   85  // matches ASCII
#define KEY_V                   86  // matches ASCII
#define KEY_W                   87  // matches ASCII
#define KEY_X                   88  // matches ASCII
#define KEY_Y                   89  // matches ASCII
#define KEY_Z                   90  // matches ASCII
#define KEY_LEFTBRACKET         91  // matches ASCII
#define KEY_BACKSLASH           92  // matches ASCII
#define KEY_RIGHTBRACKET        93  // matches ASCII
// 94-95 reserved
#define KEY_GRAVE               96  // matches ASCII
#define KEY_KPMINUS             97
#define KEY_KPDOT               98
#define KEY_KPSLASH             99
#define KEY_KP0                 100
#define KEY_KP1                 101
#define KEY_KP2                 102
#define KEY_KP3                 103
#define KEY_KP4                 104
#define KEY_KP5                 105
#define KEY_KP6                 106
#define KEY_KP7                 107
#define KEY_KP8                 108
#define KEY_KP9                 109
#define KEY_KPENTER             110
#define KEY_PRTSC               111
#define KEY_INSERT              112
#define KEY_DELETE              113
#define KEY_HOME                114
#define KEY_END                 115
#define KEY_PGUP                116
#define KEY_PGDOWN              117
#define KEY_LEFT                118
#define KEY_DOWN                119
#define KEY_RIGHT               120
#define KEY_UP                  121

// TODO:
static_assert(KEY_KP0 < KEY_KP9, "KEY_KP0 < KEY_KP9");
static_assert(KEY_A == 'A', "KEY_A == 'A'");
// etc.

#endif /* __INPUT_H */
