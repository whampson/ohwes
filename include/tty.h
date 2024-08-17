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

#ifndef __TTY_H
#define __TTY_H

#include <fs.h>
#include <queue.h>
#include <stddef.h>

#define TTY_BUFFER_SIZE     128

struct termios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_lflag;
};

enum iflag {
    INLCR = 1 << 0,             // map NL to CR
    IGNCR = 1 << 1,             // ignore carriage return
    ICRNL = 1 << 2,             // map CR to NL (unless IGNCR is set)
};
enum oflag {
    OPOST = 1 << 0,             // enable post processing
    ONLCR = 1 << 1,             // convert NL to CRNL
    OCRNL = 1 << 2,             // map CR to NL
};
enum lflag {
    ECHO    = 1 << 0,           // echo input characters
    ECHOCTL = 1 << 1,           // if ECHO set, echo control characters as ^X
};

struct tty;
struct tty_ldisc;
struct tty_driver;
struct termios;

//
// TTY State
//
struct tty {
    char *name;

    struct termios termios;     // input/output behavior
    struct tty_ldisc ldisc;     // line discipline
    struct tty_driver *driver;  // low-level device
    void *driver_data;          // private per-instance driver data

    struct ring oq;       // output queue (TODO: move to driver_data)
    char _obuf[TTY_BUFFER_SIZE];
};

//
// TTY Line Discipline
//
struct tty_ldisc {
    int     num;
    char    *name;

    struct ring iq;       // input queue
    char _ibuf[TTY_BUFFER_SIZE];// TODO: get from allocator

    // called from above (user)
    int     (*open)(struct tty *);
    void    (*close)(struct tty *);
    ssize_t (*read)(struct tty *, struct file *, char *buf, size_t count);
    ssize_t (*write)(struct tty *, struct file *, const char *buf, size_t count);
    int     (*ioctl)(struct tty *, struct file *, unsigned int cmd, unsigned long arg);
    // TODO: poll? flush? ICANON buffering

    // called from below (interrupt)
    ssize_t (*recv)(struct tty *, char *buf, size_t count);
    size_t  (*recv_room)(struct tty *);
};

//
// TTY Device Driver
//
struct tty_driver {
    char    *name;

    // called from above (user)
    int     (*open)(struct tty *);
    void    (*close)(struct tty *);
    ssize_t (*write)(struct tty *, struct file *, const char *buf, size_t count);
    int     (*ioctl)(struct tty *, struct file *, unsigned int cmd, unsigned long arg);
    // TODO: flush?

    // called from below (interrupt)
    size_t  (*write_room)(struct tty *);
};

#endif // __TTY_H
