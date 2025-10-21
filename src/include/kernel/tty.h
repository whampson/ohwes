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
#include <list.h>
#include <kernel/termios.h>

#define NR_TTY                  (1+NR_TERMINAL+NR_SERIAL)    // +1 for tty0

#define TTY_BUFFER_SIZE         1024
#define TTY_THROTTLE_THRESH     128

#define TTY_MIN                 1                   // TTY min minor ID
#define TTY_MAX                 NR_TERMINAL         // TTY max minor ID
#define TTYS_MIN                (TTY_MAX+1)         // serial TTY min minor ID
#define TTYS_MAX                (TTYS_MIN+NR_SERIAL)// serial TTY max minor ID

#define TTY_STD_TERMIOS {       \
    .c_line = N_TTY,            \
    .c_iflag = ICRNL | IXON,    \
    .c_oflag = OPOST | ONLCR,   \
    .c_lflag = ECHO | ECHOCTL,  \
    .c_cflag = HUPCL,           \
    .c_cc = {                   \
        0x13, /* VSTOP  = ^X */ \
        0x11  /* VSTART = ^S */ \
    }                           \
}

#define __mkttydev(num)         __mkdev(TTY_MAJOR, TTY_MIN+(num)-1)
#define __mkserdev(num)         __mkdev(TTY_MAJOR, TTYS_MIN+(num)-1)

#define _I_FLAG(tty,f)          ((tty)->termios->c_iflag & (f))
#define _O_FLAG(tty,f)          ((tty)->termios->c_oflag & (f))
#define _C_FLAG(tty,f)          ((tty)->termios->c_cflag & (f))
#define _L_FLAG(tty,f)          ((tty)->termios->c_lflag & (f))

#define CC_STOP(tty)            ((tty)->termios->c_cc[VSTOP])
#define CC_START(tty)           ((tty)->termios->c_cc[VSTART])

#define I_ICRNL(tty)            _I_FLAG(tty, ICRNL)
#define I_INLCR(tty)            _I_FLAG(tty, INLCR)
#define I_IGNCR(tty)            _I_FLAG(tty, IGNCR)
#define I_IXON(tty)             _I_FLAG(tty, IXON)
#define I_IXOFF(tty)            _I_FLAG(tty, IXOFF)

#define O_OPOST(tty)            _O_FLAG(tty, OPOST)
#define O_ONLCR(tty)            _O_FLAG(tty, ONLCR)
#define O_OCRNL(tty)            _O_FLAG(tty, OCRNL)

#define C_CRTSCTS(tty)          _C_FLAG(tty, CRTSCTS)
#define C_HUPCL(tty)            _C_FLAG(tty, CRTSCTS)

#define L_ECHO(tty)             _L_FLAG(tty, ECHO)
#define L_ECHOCTL(tty)          _L_FLAG(tty, ECHOCTL)

struct tty;

//
// TTY Line Discipline
//
// The line discipline controls how data is written to and read from the
// character device.
//
struct tty_ldisc {
    const char *name;                   // line disc. name
    int ldisc_num;                      // line disc. identifier (N_TTY, etc.)

    // called from above (system)
    int     (*open)(struct tty *);      // open line disc.
    void    (*close)(struct tty *);     // close line disc.
    ssize_t (*read)(struct tty *,       // read buffered chars from line disc.
                struct file *, char *buf, size_t count);
    ssize_t (*write)(struct tty *,      // write chars to line disc.
                struct file *, const char *buf, size_t count);
    int     (*ioctl)(struct tty *,      // device I/O control functions
                struct file *, int op, void *arg);
    // ssize_t (*count)(struct tty *);     // get number of avail. chars in buffer
    void    (*clear)(struct tty *);     // clear line disc. input buffer

    // called from below (interrupt)
    void    (*recv)(struct tty *,       // put received chars in input buffer
                char *buf, size_t count);
    size_t  (*recv_room)(struct tty *); // get input buffer available size
};

// system line discipline table
extern struct tty_ldisc ldiscs[NR_LDISC];

//
// TTY Driver
//
// This is the low level character device driver.
//
struct tty_driver {
    uint32_t magic;                     // tty_driver magic number
    uint16_t major;                     // major device number
    uint16_t minor_start;               // initial minor device number
    int device_count;                   // max num instances
    const char *name;                   // device name
    struct list_node list;              // node in system driver list

    int flags;                          // driver flags
    int *refcount;                      // driver instance count

    struct tty **tty_table;             // per-instance TTY pointers
    struct termios **termios;           // per-instance termios
    struct termios default_termios;     // default termios

    // interface functions
    int     (*open)(struct tty *);      // open TTY device
    void    (*close)(struct tty *);     // close TTY device
    int     (*ioctl)(struct tty *,      // device I/O control functions
                int op, void *arg);
    int     (*write)(struct tty *,      // send characters over TTY device
                const char *buf, size_t count);
    size_t  (*write_room)(struct tty *);// query space in write buffer
    void    (*flush)(struct tty *);     // flush write buffer
    void    (*clear)(struct tty *);     // clear write buffer
    void    (*unthrottle)(struct tty *);// tell far end to start sending
    void    (*throttle)(struct tty *);  // tell far end to stop sending
    void    (*stop)(struct tty *);      // stop sending chars
    void    (*start)(struct tty *);     // start sending chars
    void    (*hangup)(struct tty *);    // hang up TTY (terminate connection)
};

//
// TTY - Teletype Emulation
//
// The TTY serves as the "portal" between a character device and a program (or
// job or session).
//
struct tty {
    uint32_t magic;                 // tty magic number
    dev_t device;                   // device ID
    int refcount;                   // reference count

    bool throttled;                 // is the receiver channel throttled?
    bool stopped;                   // is transmitter channel stopped? (XON/XOFF)
    bool hw_stopped;                // is transmitter stopped? (CTS/RTS)

    struct termios *termios;        // input/output behavior
    struct tty_driver driver;       // low-level device driver
    struct tty_ldisc ldisc;         // line discipline

    void *ldisc_data;               // N_TTY data
};

int tty_register_driver(struct tty_driver *driver);
int tty_register_ldisc(int ldsic_num, struct tty_ldisc *ldisc);

// int tty_putchar(struct tty *tty, char c);

void tty_flush(struct tty *tty);

void tty_hangup(struct tty *tty);
int tty_hung_up(struct file *file);

#endif // __TTY_H
