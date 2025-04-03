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
 *         File: src/include/kernel/tty.h
 *      Created: August 14, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

//
// Very Linux-like
//
// The TTY demystified - https://www.linusakesson.net/programming/tty/
//

#ifndef __TTY_H
#define __TTY_H

#include <stddef.h>
#include <unistd.h>
#include <kernel/device.h>
#include <kernel/list.h>
#include <kernel/termios.h>

#define NR_TTY                  (1+NR_CONSOLE+NR_SERIAL)    // +1 for tty0

#define TTY_BUFFER_SIZE         1024
#define TTY_THROTTLE_THRESH     128

// TTY device minor numbers
#define CONSOLE_MIN             1
#define CONSOLE_MAX             NR_CONSOLE
#define SERIAL_MIN              (NR_CONSOLE+1)
#define SERIAL_MAX              (SERIAL_MIN+NR_SERIAL)

// handy macros for working with termios flags
#define _I_FLAG(tty,f)          ((tty)->termios.c_iflag & (f))
#define _O_FLAG(tty,f)          ((tty)->termios.c_oflag & (f))
#define _C_FLAG(tty,f)          ((tty)->termios.c_cflag & (f))
#define _L_FLAG(tty,f)          ((tty)->termios.c_lflag & (f))

// termios input flag macros
#define I_ICRNL(tty)            _I_FLAG(tty, ICRNL)
#define I_INLCR(tty)            _I_FLAG(tty, INLCR)
#define I_IGNCR(tty)            _I_FLAG(tty, IGNCR)
#define I_IXON(tty)             _I_FLAG(tty, IXON)
#define I_IXOFF(tty)            _I_FLAG(tty, IXOFF)

// termios output flag macros
#define O_OPOST(tty)            _O_FLAG(tty, OPOST)
#define O_ONLCR(tty)            _O_FLAG(tty, ONLCR)
#define O_OCRNL(tty)            _O_FLAG(tty, OCRNL)

// termios control flag macros
#define C_CRTSCTS(tty)          _C_FLAG(tty, CRTSCTS)

// termios local flag macros
#define L_ECHO(tty)             _L_FLAG(tty, ECHO)
#define L_ECHOCTL(tty)          _L_FLAG(tty, ECHOCTL)

#define STOP_CHAR(tty)          0x13    // TODO: tty->termios.c_cc[VSTOP]
#define START_CHAR(tty)         0x11    // TODO: tty->termios.c_cc[VSTART]

// type shit
struct tty;
struct tty_ldisc;

//
// TTY Driver
//
// This is the low level character device driver.
//
struct tty_driver {
    struct list_node driver_list;   // linked list node data
    const char *name;               // device name
    uint16_t major;                 // major device number
    uint16_t minor_start;           // initial minor device number
    int count;                      // max num devices

    // interface functions
    int     (*open)(struct tty *);
    int     (*close)(struct tty *);
    int     (*ioctl)(struct tty *, unsigned int cmd, void *arg);
    int     (*write)(struct tty *, const char *buf, size_t count);
    size_t  (*write_room)(struct tty *);    // query space in write buffer
    void    (*flush)(struct tty *);         // flush write buffer
    void    (*throttle)(struct tty *);      // stop receiving chars (tell transmitter to stop)
    void    (*unthrottle)(struct tty *);    // start receiving chars (tell transmitter to start)
    void    (*stop)(struct tty *);          // stop transmitting chars
    void    (*start)(struct tty *);         // start transmitting chars
};

//
// TTY - Teletype Emulation
//
// The TTY serves as the "portal" between a character device and a program (or
// job or session).
//
struct tty {
    dev_t device;                   // device major/minor numbers
    bool open;                      // is the TTY device currently open?
    bool throttled;                 // is the receiver channel throttled?
    bool stopped;                   // is transmitter channel stopped? (XON/XOFF)
    bool hw_stopped;                // is transmitter stopped? (CTS/RTS)
    int line;                       // device line number

    struct file *file;              // connected file description

    struct tty_ldisc *ldisc;        // line discipline
    struct tty_driver driver;       // low-level device driver
    struct termios termios;         // input/output behavior

    // private per-instance data
    void *ldisc_data;
};

//
// TTY Line Discipline
//
// The line discipline controls how data is written to and read from the
// character device.
//
struct tty_ldisc {
    int disc;           // line discipline number (N_TTY, etc.)
    const char *name;   // line discipline name

    // called from above (system)
    int     (*open)(struct tty *);
    int     (*close)(struct tty *);
    ssize_t (*read)(struct tty *, char *buf, size_t count);
    ssize_t (*write)(struct tty *, const char *buf, size_t count);
    void    (*flush)(struct tty *);
    void    (*clear)(struct tty *);
    int     (*ioctl)(struct tty *, unsigned int cmd, void *arg);

    // called from below (interrupt)
    void    (*recv)(struct tty *, char *buf, size_t count);
    size_t  (*recv_room)(struct tty *);
};

int tty_register_driver(struct tty_driver *driver);
int tty_register_ldisc(int ldsic_num, struct tty_ldisc *ldisc);

int get_tty(dev_t device, struct tty **tty);

#endif // __TTY_H
