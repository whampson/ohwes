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

/**
 * ASCII control characters.
 */
enum ascii_cntl
{
    ASCII_NUL,  /* '\0' */
    ASCII_SOH,
    ASCII_STX,
    ASCII_ETX,
    ASCII_EOT,
    ASCII_ENQ,
    ASCII_ACK,
    ASCII_BEL,  /* '\a' */
    ASCII_BS,   /* '\b' */
    ASCII_HT,   /* '\t' */
    ASCII_LF,   /* '\n' */
    ASCII_VT,   /* '\v' */
    ASCII_FF,   /* '\f' */
    ASCII_CR,   /* '\r' */
    ASCII_SO,
    ASCII_SI,
    ASCII_DLE,
    ASCII_DC1,
    ASCII_DC2,
    ASCII_DC3,
    ASCII_DC4,
    ASCII_NAK,
    ASCII_SYN,
    ASCII_ETB,
    ASCII_CAN,
    ASCII_EM,
    ASCII_SUB,
    ASCII_ESC,  /* '\e' */
    ASCII_FS,
    ASCII_GS,
    ASCII_RS,
    ASCII_US,
    ASCII_DEL = 0x7F
};

#endif /* __INPUT_H */
