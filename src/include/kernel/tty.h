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
// Serial Drivers - https://www.linux.it/~rubini/docs/serial/serial.html
//

#ifndef __TTY_H
#define __TTY_H

#include <stddef.h>
#include <unistd.h>
#include <kernel/device.h>
#include <kernel/list.h>

#define TTY_BUFFER_SIZE         128

#define N_TTY                   0
#define NR_LDISC                1

#define NR_TTY                  (1+NR_CONSOLE+NR_SERIAL)    // +1 for tty0

//
// Line Discipline Behavior
//
struct termios {
    uint32_t c_iflag;           // input mode flags
    uint32_t c_oflag;           // output mode flags
    uint32_t c_cflag;           // control flags
    uint32_t c_lflag;           // local mode flags
    uint32_t c_line;            // line discipline
};

// c_iflag
#define ICRNL   (1 << 0)        // map CR to NL (unless IGNCR is set)
#define INLCR   (1 << 1)        // map NL to CR
#define IGNCR   (1 << 2)        // ignore carriage return

// c_oflag
#define OPOST   (1 << 0)        // enable post processing
#define OCRNL   (1 << 1)        // map CR to NL
#define ONLCR   (1 << 2)        // convert NL to CR-NL

// c_lflag
#define ECHO    (1 << 0)        // echo input characters
#define ECHOCTL (1 << 1)        // if ECHO set, echo control characters as ^X

#define _I_FLAG(tty,f)          ((tty)->termios.c_iflag & (f))
#define _O_FLAG(tty,f)          ((tty)->termios.c_oflag & (f))
#define _C_FLAG(tty,f)          ((tty)->termios.c_cflag & (f))
#define _L_FLAG(tty,f)          ((tty)->termios.c_lflag & (f))

#define I_ICRNL(tty)            _I_FLAG(tty, ICRNL)
#define I_INLCR(tty)            _I_FLAG(tty, INLCR)
#define I_IGNCR(tty)            _I_FLAG(tty, IGNCR)

#define O_OPOST(tty)            _O_FLAG(tty, OPOST)
#define O_ONLCR(tty)            _O_FLAG(tty, ONLCR)
#define O_OCRNL(tty)            _O_FLAG(tty, OCRNL)

#define L_ECHO(tty)             _L_FLAG(tty, ECHO)
#define L_ECHOCTL(tty)          _L_FLAG(tty, ECHOCTL)

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
    int     (*ioctl)(struct tty *, unsigned int cmd, unsigned long arg);
    int     (*write)(struct tty *, const char *buf, size_t count);
    void    (*write_char)(struct tty *, char c);
    size_t  (*write_room)(struct tty *);    // query space in write buffer
    void    (*flush)(struct tty *);         // flush write buffer
    void    (*stop)(struct tty *);          // stop transmitting chars
    void    (*start)(struct tty *);         // start transmitting chars
    void    (*throttle)(struct tty *);      // stop receiving chars (tell transmitter to stop)
    void    (*unthrottle)(struct tty *);    // start receiving chars (tell transmitter to start)
};

//
// TTY - Teletype Emulation
//
// The TTY serves as the "portal" between a character device and a program (or
// job or session).
//
struct tty {
    bool open;                      // is the TTY device currently open?
    dev_t device;                   // device major/minor numbers

    struct tty_ldisc *ldisc;        // line discipline
    struct tty_driver driver;       // low-level device driver
    struct termios termios;         // input/output behavior

    // private per-instance driver data
    void *ldisc_data;
    void *driver_data;  // TODO: needed?
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
    int     (*ioctl)(struct tty *, unsigned int cmd, unsigned long arg);

    // called from below (interrupt)
    void    (*recv)(struct tty *, char *buf, size_t count);
    size_t  (*recv_room)(struct tty *);
};

int tty_register_driver(struct tty_driver *driver);
int tty_register_ldisc(int ldsic_num, struct tty_ldisc *ldisc);

int get_tty(dev_t device, struct tty **tty);

#endif // __TTY_H
