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
 *         File: include/kernel/termios.h
 *      Created: January 3, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __TERMIOS_H
#define __TERMIOS_H

typedef unsigned int    tcflag_t;
typedef unsigned char   cc_t;

#define N_TTY           0           // TTY line discipline
#define NR_LDISC        1           // num ldiscs

#define NCCS            0           // num control characters (TODO)

struct termios {
    tcflag_t c_line;                // ldisc number
    tcflag_t c_iflag;               // input mode flags
    tcflag_t c_oflag;               // output mode flags
    tcflag_t c_cflag;               // control flags
    tcflag_t c_lflag;               // local mode flags
    cc_t c_cc[NCCS];                // control characters
};

// c_iflag: input modes
#define ICRNL           (1 << 0)    // map CR to NL (unless IGNCR is set)
#define INLCR           (1 << 1)    // map NL to CR
#define IGNCR           (1 << 2)    // ignore carriage return
#define IXON            (1 << 3)    // enable software flow control on input
#define IXOFF           (1 << 4)    // enable software flow control on output

// c_oflag: output modes
#define OPOST           (1 << 0)    // enable post processing
#define OCRNL           (1 << 1)    // map CR to NL
#define ONLCR           (1 << 2)    // convert NL to CR-NL

// c_cflag: control modes
#define CRTSCTS         (1 << 0)    // enable RTS/CTS flow control

// c_lflag: local modes
#define ECHO            (1 << 0)    // echo input characters
#define ECHOCTL         (1 << 1)    // if ECHO set, echo control characters as ^C

// serial modem status
#define TIOCM_DTR       (1 << 0)    // DTR (data terminal ready)
#define TIOCM_RTS       (1 << 1)    // RTS (request to send)
#define TIOCM_OUT1	    (1 << 2)    // Aux Out #1 (terminal ring)
#define TIOCM_OUT2	    (1 << 3)    // Aux Out #2 (terminal data carrier detect)
#define TIOCM_CTS       (1 << 4)    // CTS (clear to send)
#define TIOCM_CAR       (1 << 5)    // DCD (data carrier detect)
#define TIOCM_RNG       (1 << 6)    // RNG (ring)
#define TIOCM_DSR       (1 << 7)    // DSR (data set ready)
#define TIOCM_CD        TIOCM_CAR
#define TIOCM_RI        TIOCM_RNG

#endif  // __TERMIOS_H
