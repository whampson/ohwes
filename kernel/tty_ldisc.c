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
 *         File: kernel/tty_ldisc.c
 *      Created: December 10, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <tty.h>
#include <interrupt.h>

//
// line discipline tty operations
//
static int tty_ldisc_open(struct tty *);
static int tty_ldisc_close(struct tty *);
static int tty_ldisc_read(struct tty *tty, char *buf, size_t count);
static ssize_t tty_ldisc_write(struct tty *, const char *buf, size_t count);
static int tty_ldisc_ioctl(struct tty *, unsigned int num, unsigned long arg);
static void tty_ldisc_recv(struct tty *, char *buf, size_t count);
static size_t tty_ldisc_recv_room(struct tty *);
static void tty_ldisc_clear(struct tty *tty);

struct tty_ldisc n_tty = {
    .num = N_TTY,
    .name = "n_tty",
    .open = tty_ldisc_open,
    .close = tty_ldisc_close,
    .read = tty_ldisc_read,
    .write = tty_ldisc_write,
    .ioctl = tty_ldisc_ioctl,
    .recv = tty_ldisc_recv,
    .recv_room = tty_ldisc_recv_room,
    .clear = tty_ldisc_clear
};

struct n_tty_ldisc_data {
    struct ring iring;
    char _iring_buf[TTY_BUFFER_SIZE];
};
static struct n_tty_ldisc_data _n_tty_data;

// void tty_ldisc_init(struct tty *tty)
// {
//     tty_register_ldisc(tty, N_TTY, &n_tty);
// }

static int tty_ldisc_open(struct tty *tty)
{
    if (!tty || !tty->ldisc) {
        return -EINVAL;
    }

    // TODO: buffer needs to be dynamically allocated,
    // CANNOT exist in singleton struct
    ring_init(&_n_tty_data.iring, _n_tty_data._iring_buf, TTY_BUFFER_SIZE);
    tty->ldisc_data = &_n_tty_data;
    return 0;
}

static int tty_ldisc_close(struct tty *tty)
{
    return -ENOSYS;
}

void tty_ldisc_clear(struct tty *tty)
{
    if (!tty || !tty->ldisc_data) {
        return;
    }

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ring_clear(&ldisc_data->iring);
}

static void n_tty_clear(struct tty *tty)
{
    if (!tty || !tty->ldisc_data) {
        return;
    }

    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;
    ring_clear(&ldisc_data->iring);
}

static int tty_ldisc_read(struct tty *tty, char *buf, size_t count)
{
    if (!tty || !buf) {
        return -EINVAL;
    }
    if (!tty->ldisc_data) {
        return -ENXIO;
    }

    char c;
    int nread;
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

static ssize_t tty_ldisc_write(struct tty *tty, const char *buf, size_t count)
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

static int tty_ldisc_ioctl(struct tty *tty, unsigned int num, unsigned long arg)
{
    return -ENOSYS;
}

static void tty_ldisc_recv(struct tty *tty, char *buf, size_t count)
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

static size_t tty_ldisc_recv_room(struct tty *tty)
{
    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    return ring_length(&ldisc_data->iring) - ring_count(&ldisc_data->iring);
}
