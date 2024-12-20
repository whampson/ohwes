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
 *         File: src/kernel/drivers/char/tty_ldisc.c
 *      Created: December 10, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <i386/interrupt.h>
#include <kernel/config.h>
#include <kernel/kernel.h>
#include <kernel/pool.h>
#include <kernel/queue.h>
#include <kernel/tty.h>

//
// line discipline tty operations
//
static int n_tty_open(struct tty *);
static int n_tty_close(struct tty *);
static ssize_t n_tty_read(struct tty *tty, char *buf, size_t count);
static ssize_t n_tty_write(struct tty *, const char *buf, size_t count);
static int n_tty_ioctl(struct tty *, unsigned int num, unsigned long arg);
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
    struct ring iring;
    char _iring_buf[TTY_BUFFER_SIZE];
};
static struct n_tty_ldisc_data ldisc_data[NR_TTY];

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
    ring_init(&data->iring, data->_iring_buf, TTY_BUFFER_SIZE);
    tty->ldisc_data = data;
    return 0;
}

static int n_tty_close(struct tty *tty)
{
    // TODO: free pool item
    return -ENOSYS;
}

void n_tty_clear(struct tty *tty)
{
    if (!tty || !tty->ldisc_data) {
        return;
    }

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ring_clear(&ldisc_data->iring);
}

static ssize_t n_tty_read(struct tty *tty, char *buf, size_t count)
{
    if (!tty || !buf) {
        return -EINVAL;
    }
    if (!tty->ldisc_data) {
        return -ENXIO;
    }

    char c;
    ssize_t nread;
    uint32_t flags;

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    nread = 0;
    while (count--) {
        // block until a character appears
        // TODO: allow nonblocking input
        while (ring_empty(&ldisc_data->iring)) { }

        cli_save(flags);
        c = ring_get(&ldisc_data->iring);
        restore_flags(flags);

        *buf++ = c;
        nread++;
    }

    return nread;
}

static ssize_t n_tty_write(struct tty *tty, const char *buf, size_t count)
{
    if (!tty || !buf) {
        return -EINVAL;
    }

    if (!tty->driver.write_char) {
        return -EIO;
    }

    // TODO: need a write buffer, fill it up then send it to driver->write() until
    // source buffer is drained; don't send char-by-char (let the driver do that)

    ssize_t nwritten = 0;

    for (int i = 0; i < count; i++) {
        char c = buf[i];
        if (O_OPOST(tty)) {
             if (c == '\r' && O_OCRNL(tty)) {
                c = '\n';
            }
            if (c == '\n' && O_ONLCR(tty)) {
                tty->driver.write_char(tty, '\r');
            }
        }

        tty->driver.write_char(tty, c);
        nwritten++; // only count chars written from input buf
    }

    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    return nwritten;
}

static int n_tty_ioctl(struct tty *tty, unsigned int num, unsigned long arg)
{
    return -ENOSYS;
}

static void n_tty_recv(struct tty *tty, char *buf, size_t count)
{
    if (!tty || !buf) {
        return;
    }
    if (!tty->ldisc_data) {
        return;
    }

    if (!tty->driver.write_char) {
        return;
    }

    uint32_t flags;
    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    for (int i = 0; i < count; i++) {
        char c = buf[i];
        if (ring_full(&ldisc_data->iring)) {
            tty->driver.write_char(tty, '\a');  // beep!
            break;
        }
        if (L_ECHO(tty)) {
            if (L_ECHOCTL(tty) && iscntrl(c)) {
                tty->driver.write_char(tty, '^');
                tty->driver.write_char(tty, c ^ 0x40);
            }
            else {
                tty->driver.write_char(tty, c);
            }
        }
        if (c == '\r') {
            if (I_IGNCR(tty)) {
                continue;
            }
            if (I_ICRNL(tty)) {
                c = '\n';
            }
        }
        else if (c == '\n' && I_INLCR(tty)) {
            c = '\r';
        }

        cli_save(flags);
        ring_put(&ldisc_data->iring, buf[i]);
        restore_flags(flags);
    }

    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }
}

static size_t n_tty_recv_room(struct tty *tty)
{
    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    return ring_length(&ldisc_data->iring) - ring_count(&ldisc_data->iring);
}
