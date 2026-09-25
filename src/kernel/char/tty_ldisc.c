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
#include <ring.h>
#include <i386/x86.h>
#include <i386/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/tty.h>
#include <kernel/kprint.h>
//
// line discipline tty operations
//
static int n_tty_open(struct tty *);
static void n_tty_close(struct tty *);
static ssize_t n_tty_read(struct tty *tty, struct file *, char *buf, size_t count);
static ssize_t n_tty_write(struct tty *, struct file *, const char *buf, size_t count);
static int n_tty_ioctl(struct tty *, struct file *, int op, void *arg);
static void n_tty_recv(struct tty *, char *buf, size_t count);
static size_t n_tty_recv_room(struct tty *);
static void n_tty_clear(struct tty *tty);

static struct tty_ldisc n_tty = {
    .ldisc_num = N_TTY,
    .name = "n_tty",
    .open = n_tty_open,
    .close = n_tty_close,
    .read = n_tty_read,
    .write = n_tty_write,
    .ioctl = n_tty_ioctl,
    .clear = n_tty_clear,
    .recv = n_tty_recv,
    .recv_room = n_tty_recv_room,
};

struct n_tty_ldisc_data {
    struct ring rx_ring;
    char _rxbuf[TTY_BUFFER_SIZE];   // TODO: dynamically allocate
};
static struct n_tty_ldisc_data ldisc_data[NR_TTY];

static int opost(struct tty *tty, char c);
static int echo(struct tty *tty, char c);
static void write_char(struct tty *tty, char c);

__init void init_n_tty(void)
{
    if (tty_register_ldisc(N_TTY, &n_tty)) {
        panic("unable to register N_TTY line discipline!");
    }
}

static int n_tty_open(struct tty *tty)
{
    if (!tty || _DEV_MIN(tty->device) >= NR_TTY) {
        return -EINVAL;
    }

    struct n_tty_ldisc_data *data = &ldisc_data[_DEV_MIN(tty->device)];
    ring_init(&data->rx_ring, data->_rxbuf, TTY_BUFFER_SIZE);
    tty->ldisc_data = data;
    return 0;
}

static void n_tty_close(struct tty *tty)
{
    n_tty_clear(tty);
}

static void n_tty_clear(struct tty *tty)
{
    if (!tty || !tty->ldisc_data) {
        return;
    }

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ring_clear(&ldisc_data->rx_ring);
}

static ssize_t n_tty_read(struct tty *tty, struct file *file, char *buf, size_t count)
{
    struct n_tty_ldisc_data *ldisc_data;
    uint32_t flags;
    size_t nremain;
    char *ptr;
    int ret;

    if (!tty || !file || !buf) {
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
        cli_save(flags);
        nremain = ring_count(&ldisc_data->rx_ring);
        restore_flags(flags);
        if (!nremain) {
            if (tty_hung_up(file)) {
                break;  // that was rude! nothing left to receive
            }
            if (file->f_oflag & O_NONBLOCK) {
                if ((ptr - buf) == 0) {
                    ret = -EAGAIN;  // operation would block
                    break;
                }
                break;
            }
            if ((ptr - buf) > 0) {
                break;  // got at least one char
            }
            continue;   // spin until a char appears, TODO: SCHEDULE HERE
        }

        // grab the characters
        while (nremain > 0 && count > 0) {
            cli_save(flags);
            ring_pop_front(&ldisc_data->rx_ring, *ptr++, char);
            restore_flags(flags);
            nremain--; count--;
        }

        // check if we can unthrottle
        if (n_tty_recv_room(tty) >= TTY_THROTTLE_THRESH) {
            tty_unthrottle(tty);
        }
    }

    return (ret < 0) ? ret : ptr - buf;
}

static ssize_t n_tty_write(struct tty *tty, struct file *file, const char *buf, size_t count)
{
    ssize_t ret;
    const char *ptr;

    if (!tty || !file || !buf) {
        return -EINVAL;
    }

    // TODO: handle O_NONBLOCK

    ptr = buf; ret = 0;
    while (count > 0) {
        if (tty_hung_up(file)) {
            ret = -EIO;
            goto skip_flush;
        }
        if (O_OPOST(tty)) {
            ret = opost(tty, *ptr);
            if (ret < 0) {      // returns -1 if no chars in buffer
                if (file->f_oflag & O_NONBLOCK) {
                    ret = -EAGAIN;  // operation would block
                    break;
                }
                continue;   // spin until a character appears, TODO: SCHEDULE HERE
            }
            ptr++; count--;
        }
        else {
            if (tty->driver.write) {
                ret = tty->driver.write(tty, ptr, count);
                if (ret < 0) {
                    if (file->f_oflag & O_NONBLOCK) {
                        ret = -EAGAIN;  // operation would block
                        break;
                    }
                    continue;   // spin until a character appears, TODO: SCHEDULE HERE
                }
            }
            count -= ret;
            ptr += ret;
        }
    }

    // TODO: do we always want to flush? some kind of autoflush setting?
    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

skip_flush:
    return (ret >= 0) ? ptr - buf : ret;
}

static int n_tty_ioctl(struct tty *tty, struct file *file, int op, void *arg)
{
    // TODO
    return -ENOTTY;
}

int __n_tty_getc(struct tty *tty)
{
    struct n_tty_ldisc_data *ldisc_data;
    uint32_t flags;
    char c;

    if (!tty || !tty->ldisc_data) {
        return -EINVAL;
    }

    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    cli_save(flags);
    if (!ring_pop_front(&ldisc_data->rx_ring, c, char)) {
        restore_flags(flags);
        return -EAGAIN;
    }
    restore_flags(flags);

    // room freed up; let sender resume
    if (n_tty_recv_room(tty) >= TTY_THROTTLE_THRESH) {
        tty_unthrottle(tty);
    }

    return (unsigned char) c;
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
            if (c == CC_START(tty)) {
                tty_start(tty);
                goto next_char;
            }
            if (c == CC_STOP(tty)) {
                tty_stop(tty);
                goto next_char;
            }
        }

        // handle CR and NL translation
        switch (c) {
            case '\r':
                if (I_IGNCR(tty)) {
                    goto next_char;
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

        // add char to buffer
        cli_save(flags);
        if (!ring_push_back(&ldisc_data->rx_ring, c, char)) {
            restore_flags(flags);
            if (L_ECHO(tty)) {  // TODO: IMAXBEL!
                write_char(tty, '\a');  // we're full... beep!!
            }
            goto next_char; // =>
        }
        restore_flags(flags);

        // echo it back
        if (L_ECHO(tty)) {
            echo(tty, c);
        }

    next_char:
        ptr++; count--;
    }

    // flush any echoed chars
    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    // throttle the receiver channel if we're approaching capacity
    if (n_tty_recv_room(tty) < TTY_THROTTLE_THRESH) {
        tty_throttle(tty);
    }
}

static size_t n_tty_recv_room(struct tty *tty)
{
    uint32_t flags;
    size_t room;
    struct n_tty_ldisc_data *ldisc_data;

    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    cli_save(flags);
    room = ring_capacity(&ldisc_data->rx_ring) - ring_count(&ldisc_data->rx_ring);
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
    if (tty->driver.write) {
        tty->driver.write(tty, &c, 1);
    }
}

void tty_unthrottle(struct tty *tty)
{
    if (!tty->throttled) {
        return;
    }
    tty->throttled = false;

    // tell the peer to start sending
    // driver handles software (IXOFF) and hardware (CRTSCTS) flow control
    if (tty->driver.unthrottle) {
        tty->driver.unthrottle(tty);
    }
}

void tty_throttle(struct tty *tty)
{
    if (tty->throttled) {
        return;
    }
    tty->throttled = true;

    // tell the peer to stop sending
    // driver handles software (IXOFF) and hardware (CRTSCTS) flow control
    if (tty->driver.throttle) {
        tty->driver.throttle(tty);
    }
}

void tty_start(struct tty *tty)
{
    if (!tty->stopped) {
        return;
    }

    tty->stopped = false;
    if (tty->driver.start) {
        tty->driver.start(tty);
    }
}

void tty_stop(struct tty *tty)
{
    if (tty->stopped) {
        return;
    }

    tty->stopped = true;
    if (tty->driver.stop) {
        tty->driver.stop(tty);
    }
}
