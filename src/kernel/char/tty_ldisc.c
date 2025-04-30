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
 *         File: kernel/char/tty_ldisc.c
 *      Created: December 10, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <i386/interrupt.h>
#include <kernel/config.h>
#include <kernel/kernel.h>
#include <kernel/queue.h>
#include <kernel/tty.h>

//
// line discipline tty operations
//
static int n_tty_open(struct tty *);
static int n_tty_close(struct tty *);
static ssize_t n_tty_read(struct tty *tty, char *buf, size_t count);
static ssize_t n_tty_write(struct tty *, const char *buf, size_t count);
static int n_tty_ioctl(struct tty *, int op, void *arg);
static void n_tty_recv(struct tty *, char *buf, size_t count);
static size_t n_tty_recv_room(struct tty *);
static void n_tty_clear(struct tty *tty);

static struct tty_ldisc n_tty = {
    .disc = N_TTY,
    .name = "n_tty",
    .open = n_tty_open,
    .close = n_tty_close,
    .read = n_tty_read,
    .write = n_tty_write,
    .clear = n_tty_clear,
    .flush = NULL,  // TODO:
    .ioctl = n_tty_ioctl,
    .recv = n_tty_recv,
    .recv_room = n_tty_recv_room,
};

struct n_tty_ldisc_data {
    struct ring rx_ring;
    char _rxbuf[TTY_BUFFER_SIZE];
};
static struct n_tty_ldisc_data ldisc_data[NR_TTY];

static int opost(struct tty *tty, char c);
static int echo(struct tty *tty, char c);
static void write_char(struct tty *tty, char c);
static void unthrottle_tty(struct tty *tty);
static void throttle_tty(struct tty *tty);
static void start_tty(struct tty *tty);
static void stop_tty(struct tty *tty);

void init_n_tty(void)
{
    if (tty_register_ldisc(N_TTY, &n_tty)) {
        panic("unable to register N_TTY line discipline!");
    }
}

static int n_tty_open(struct tty *tty)
{
    if (!tty || !tty->ldisc) {
        return -EINVAL;
    }

    struct n_tty_ldisc_data *data = &ldisc_data[_DEV_MIN(tty->device)];
    ring_init(&data->rx_ring, data->_rxbuf, TTY_BUFFER_SIZE);
    tty->ldisc_data = data;
    return 0;
}

static int n_tty_close(struct tty *tty)
{
    // TODO: flush etc.
    assert(!"implement me!");
    return -ENOSYS;
}

void n_tty_clear(struct tty *tty)
{
    if (!tty || !tty->ldisc_data) {
        return;
    }

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ring_clear(&ldisc_data->rx_ring);
}

static ssize_t n_tty_read(struct tty *tty, char *buf, size_t count)
{
    struct n_tty_ldisc_data *ldisc_data;
    uint32_t flags;
    size_t nremain;
    char *ptr;
    int ret;

    if (!tty || !buf) {
        return -EINVAL;
    }
    if (!tty->ldisc_data) {
        assert(!"where's the ldsic data??");
        return -ENXIO;
    }

    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ptr = buf;

    ret = 0;
    while (count > 0) {
        nremain = ring_count(&ldisc_data->rx_ring);
        if (!nremain) {
            if (tty->file->f_oflag & O_NONBLOCK) {
                if ((ptr - buf) == 0) {
                    ret = -EAGAIN;  // operation would block
                    break;
                }
                break;
            }
            continue;   // spin until a char appears, TODO: timeout?
        }

        // grab the character
        cli_save(flags);
        *ptr = ring_get(&ldisc_data->rx_ring);
        ptr++; count--;
        restore_flags(flags);

        // check if we can unthrottle
        if (n_tty_recv_room(tty) >= TTY_THROTTLE_THRESH) {
            unthrottle_tty(tty);
        }
    }

    return (ret < 0) ? ret : ptr - buf;
}

static ssize_t n_tty_write(struct tty *tty, const char *buf, size_t count)
{
    ssize_t ret;
    const char *ptr;

    if (!tty || !buf) {
        return -EINVAL;
    }
    if (!tty->driver.write) {
        return -EIO;    // TODO: correct return value?
    }

    ptr = buf; ret = 0;
    while (count > 0) {
        if (O_OPOST(tty)) {
            ret = opost(tty, *ptr);
            if (ret < 0) {  // returns -1 if no chars in buffer
                ret = 0;
                break;
            }
            ptr++; count--;
        }
        else {
            ret = tty->driver.write(tty, ptr, count);
            if (ret < 0) {
                break;
            }
            count -= ret;
            ptr += ret;
        }
    }

    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    return (ret >= 0) ? ptr - buf : ret;
}

static int n_tty_ioctl(struct tty *tty, int op, void *arg)
{
    // TODO
    return -ENOTTY;
}

static void n_tty_recv(struct tty *tty, char *buf, size_t count)
{
    uint32_t flags;
    struct n_tty_ldisc_data *ldisc_data;
    char *ptr;
    char c;

    if (!tty || !buf) {
        return;
    }
    if (!tty->ldisc_data) {
        return;
    }

    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ptr = buf;

    while (count > 0) {
        c = *ptr;

        // handle software flow control
        if (I_IXON(tty)) {
            if (c == START_CHAR(tty)) {
                start_tty(tty);
                return;
            }
            if (c == STOP_CHAR(tty)) {
                stop_tty(tty);
                return;
            }
        }

        // handle CR and NL translation
        switch (c) {
            case '\r':
                if (I_IGNCR(tty)) {
                    continue;
                }
                if (I_ICRNL(tty)) {
                    c = '\n';
                }
                break;
            case '\n':
                if (I_INLCR(tty)) {
                    c = '\r';
                }
                break;
        }

        // handle character echo
        if (L_ECHO(tty)) {
            if (n_tty_recv_room(tty) <= 1) {
                write_char(tty, '\a');  // we're full... beep!!
                return;
            }
            else {
                echo(tty, c);
            }
        }

        // add char to buffer
        cli_save(flags);
        ring_put(&ldisc_data->rx_ring, c);
        restore_flags(flags);
        ptr++; count--;
    }

    // flush any echoed chars
    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    // throttle the receiver channel if we're approaching capacity
    if (n_tty_recv_room(tty) < TTY_THROTTLE_THRESH) {
        throttle_tty(tty);
    }
}

static size_t n_tty_recv_room(struct tty *tty)
{
    uint32_t flags;
    size_t room;
    struct n_tty_ldisc_data *ldisc_data;

    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    cli_save(flags);
    room = ring_length(&ldisc_data->rx_ring) - ring_count(&ldisc_data->rx_ring);
    restore_flags(flags);

    return room;
}

static int opost(struct tty *tty, char c)
{
    size_t room = tty->driver.write_room(tty);
    if (room < 1) {
        return -1;
    }

    if (O_OPOST(tty)) {
        switch (c) {
            case '\r':
                if (O_OCRNL(tty)) {
                    c = '\n';
                }
                break;
            case '\n':
                if (O_ONLCR(tty)) {
                    if (room < 2) {
                        return -1;
                    }
                    write_char(tty, '\r');
                }
                break;
        }
    }

    write_char(tty, c);
    return 0;
}

static int echo(struct tty *tty, char c)
{
    size_t room;

    if (L_ECHOCTL(tty) && iscntrl(c) && c != '\t' && c != '\n') {
        room = tty->driver.write_room(tty);
        if (room < 2) {
            return -1;
        }
        write_char(tty, '^');
        write_char(tty, c ^ 0x40);
        return 0;
    }

    return opost(tty, c);
}

static void write_char(struct tty *tty, char c)
{
    tty->driver.write(tty, &c, 1);
}

static void unthrottle_tty(struct tty *tty)
{
    if (!tty->throttled) {
        return;
    }

    tty->throttled = false;
    if (tty->driver.unthrottle) {
        tty->driver.unthrottle(tty);
    }
}

static void throttle_tty(struct tty *tty)
{
    if (tty->throttled) {
        return;
    }

    tty->throttled = true;
    if (tty->driver.throttle) {
        tty->driver.throttle(tty);
    }
}

static void start_tty(struct tty *tty)
{
    if (!tty->stopped) {
        return;
    }

    tty->stopped = false;
    if (tty->driver.start) {
        tty->driver.start(tty);
    }
}

static void stop_tty(struct tty *tty)
{
    if (tty->stopped) {
        return;
    }

    tty->stopped = true;
    if (tty->driver.stop) {
        tty->driver.stop(tty);
    }
}
