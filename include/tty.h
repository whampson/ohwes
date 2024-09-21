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
 *         File: include/tty.h
 *      Created: August 14, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

//
// Very Linux-like
//
// The TTY demystified - https://www.linusakesson.net/programming/tty/
// Serial Drivers - https://www.linux.it/~rubini/docs/serial/serial.html
//

#ifndef __TTY_H
#define __TTY_H

#include <fs.h>
#include <queue.h>
#include <stddef.h>

#define TTY_BUFFER_SIZE             128

#define N_TTY                       0
#define NR_LDISC                    1

// c_iflag
#define INLCR   0x01                // map NL to CR
#define IGNCR   0x02                // ignore carriage return
#define ICRNL   0x04                // map CR to NL (unless IGNCR is set)

// c_oflag
#define OPOST   0x01                // enable post processing
#define ONLCR   0x02                // convert NL to CRNL
#define OCRNL   0x04                // map CR to NL

// c_lflag
#define ECHO    0x01                // echo input characters
#define ECHOCTL 0x02                // if ECHO set, echo control characters as ^X

struct tty;

//
// Line Discipline Behavior
//
struct termios {
    uint32_t c_iflag;               // input mode flags
    uint32_t c_oflag;               // output mode flags
    uint32_t c_cflag;               // control flags
    uint32_t c_lflag;               // local mode flags
    uint32_t c_line;                // line discipline
};

#define _I_FLAG(tty,f)  ((tty)->termios->c_oflag & (f))
#define _O_FLAG(tty,f)  ((tty)->termios->c_oflag & (f))
#define _C_FLAG(tty,f)  ((tty)->termios->c_oflag & (f))
#define _L_FLAG(tty,f)  ((tty)->termios->c_oflag & (f))

#define O_OPOST(tty)    _O_FLAG(tty, OPOST)
#define O_ONLCR(tty)    _O_FLAG(tty, ONLCR)
#define O_OCRNL(tty)    _O_FLAG(tty, OCRNL)

//
// TTY Line Discipline
//
// The line discipline controls how data is written to and read from the
// character device.
//
struct tty_ldisc {
    int num;
    char *name;

    // called from above (user)
    int     (*open)(struct tty *);
    int     (*close)(struct tty *);
    ssize_t (*read)(struct tty *, char *buf, size_t count);
    ssize_t (*write)(struct tty *, const char *buf, size_t count);
    int     (*ioctl)(struct tty *, unsigned int cmd, unsigned long arg);
    // TODO: poll? flush? ICANON buffering

    // called from below (interrupt)
    ssize_t (*recv)(struct tty *, char *buf, size_t count);
    size_t  (*recv_room)(struct tty *);
};

//
// TTY Driver
//
// This is the low level character device driver.
//
struct tty_driver {
    char *name;
    uint16_t major;
    uint16_t minor;

    // interface functions
    int     (*open)(struct tty *);
    int     (*close)(struct tty *);
    int     (*ioctl)(struct tty *, unsigned int cmd, unsigned long arg);
    int     (*write)(struct tty *, const char *buf, size_t count);
    size_t  (*write_room)(struct tty *);
    // TODO: flush?

    struct termios default_termios;
};

//
// TTY - Teletype Emulation
//
// The TTY serves as the "portal" between a character device and a program (or
// job or session).
//
struct tty {
    char name[32];
    uint16_t major;
    int index;
    bool open;

    struct tty_ldisc *ldisc;        // line discipline
    struct tty_driver *driver;      // low-level device driver
    struct termios *termios;        // input/output behavior

    // input buffer
    struct ring iring;
    char iring_buf[TTY_BUFFER_SIZE];// TODO: allocate

    // private per-instance driver data
    void *ldisc_data;
    void *driver_data;
};

#endif // __TTY_H
