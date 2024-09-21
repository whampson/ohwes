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
 *         File: kernel/tty.c
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <config.h>
#include <errno.h>
#include <chdev.h>
#include <fs.h>
#include <tty.h>
#include <queue.h>
#include <ohwes.h>

// struct tty_driver *tty_drivers;   // TODO: linked list?

static struct termios default_termios = {
    .c_line = N_TTY,
    .c_oflag = OPOST | ONLCR,
    .c_lflag = ECHO
};

//
// tty file operations
//
static int tty_open(struct inode *, struct file *);
static int tty_close(struct file *);
static ssize_t tty_read(struct file *, char *buf, size_t count);
static ssize_t tty_write(struct file *, const char *buf, size_t count);
static int tty_ioctl(struct file *, unsigned int num, unsigned long arg);

struct file_ops tty_fops = {
    .open = tty_open,
    .close = tty_close,
    .read = tty_read,
    .write = tty_write,
    .ioctl = tty_ioctl,
};

//
// line discipline tty operations
//
static int tty_ldisc_open(struct tty *);
static int tty_ldisc_close(struct tty *);
static ssize_t tty_ldisc_read(struct tty *, char *buf, size_t count);
static ssize_t tty_ldisc_write(struct tty *, const char *buf, size_t count);
static int tty_ldisc_ioctl(struct tty *, unsigned int num, unsigned long arg);
static ssize_t tty_ldisc_recv(struct tty *, char *buf, size_t count);
static size_t  tty_ldisc_recv_room(struct tty *);

struct tty g_ttys[NR_TTY + NR_SERIAL];
struct tty_ldisc g_ldiscs[NR_LDISC] =
{
    {
        .num = N_TTY,
        .name = "n_tty",
        .open = tty_ldisc_open,
        .close = tty_ldisc_close,
        .read = tty_ldisc_read,
        .write = tty_ldisc_write,
        .ioctl = tty_ldisc_ioctl,
        .recv = tty_ldisc_recv,
        .recv_room = tty_ldisc_recv_room,
    }
};

// ----------------------------------------------------------------------------

void init_tty(void)
{
    zeromem(g_ttys, sizeof(struct tty) * NR_TTY);
}

// ----------------------------------------------------------------------------

extern struct tty_driver console_driver;
extern struct tty_driver serial_driver;

static int tty_open(struct inode *inode, struct file *file)
{
    int ret;
    struct tty *tty;
    uint16_t index;

    if (!inode) {
        return -EINVAL;
    }

    tty = NULL;
    switch (_DEV_MAJ(inode->device)) {
        case TTY_MAJOR:
            index = _DEV_MIN(inode->device);
            assert(index > 0); // TODO: tty0 should represent current process's TTY
            if ((index - 1) >= NR_TTY) {
                return -ENXIO;
            }
            tty = &g_ttys[index - 1];
            tty->driver = &console_driver;  // TODO: get from tty_drivers list
            break;
        case TTYS_MAJOR:
            index = _DEV_MIN(inode->device);
            if (index >= NR_SERIAL) {
                return -ENXIO;
            }
            tty = &g_ttys[NR_TTY + index];
            tty->driver = &serial_driver;   // TODO: get from tty_drivers list
            break;
    }

    if (!tty) {
        panic("invalid tty device!");
        return -ENXIO;
    }

    if (tty->open) {
        ret = 0;
        goto open_done;
    }

    tty->major = _DEV_MAJ(inode->device);
    tty->index = index;
    snprintf(tty->name, countof(tty->name), "%s%d", tty->driver->name, index);

    tty->driver->default_termios = default_termios; // TODO: move this to driver init
    tty->termios = &tty->driver->default_termios;

    // open line discipline
    tty->ldisc = &g_ldiscs[N_TTY];
    if (!tty->ldisc->open) {
        return -ENXIO;
    }
    ret = tty->ldisc->open(tty);
    if (ret) {
        return ret;
    }

    // open the tty driver
    if (!tty->driver->open) {
        return -ENXIO;
    }
    ret = tty->driver->open(tty);
    if (ret) {
        return ret;
    }

    // char buf[64];
    // snprintf(buf, sizeof(buf), "%s\n", tty->name);
    // tty_ldisc_write(tty, buf, strlen(buf));

open_done:
    // set file state
    if (file) {
        file->fops = &tty_fops;
        file->private_data = tty;
    }

    tty->open = true;
    return ret;
}

static int tty_close(struct file *file)
{
    // TODO: flush buffers, close ldisc, close/detach, driver
    return -ENOSYS;
}

static ssize_t tty_read(struct file *file, char *buf, size_t count)
{
    // TODO: read ldisc
    return -ENOSYS;
}

static ssize_t tty_write(struct file *file, const char *buf, size_t count)
{
    struct tty *tty;

    if (!file || !buf) {
        return -EINVAL;
    }

    // TODO: check device ID (ENODEV if not a TTY)
    // TODO: verify type with magic number check or something
    tty = (struct tty *) file->private_data;

    if (!tty || !tty->ldisc) {
        return -ENXIO;
    }
    if (!tty->ldisc->write) {
        return -ENOSYS;
    }

    return tty->ldisc->write(tty, buf, count);
}

static int tty_ioctl(struct file *file, unsigned int num, unsigned long arg)
{
    // TODO: forward ioctl to ldisc or driver
    return -ENOSYS;
}

// ----------------------------------------------------------------------------

struct n_tty_ldisc_data {
    struct ring iring;
    struct ring oring;
    char _iring_buf[TTY_BUFFER_SIZE];
    char _oring_buf[TTY_BUFFER_SIZE];
};
static struct n_tty_ldisc_data ldisc_data;

static int tty_ldisc_open(struct tty *tty)
{
    if (!tty) {
        return -EINVAL;
    }

    ring_init(&ldisc_data.iring, ldisc_data._iring_buf, TTY_BUFFER_SIZE);
    ring_init(&ldisc_data.oring, ldisc_data._oring_buf, TTY_BUFFER_SIZE);
    tty->ldisc_data = &ldisc_data;
    return 0;
}

static int tty_ldisc_close(struct tty *tty)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_read(
    struct tty *tty, char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_write(
    struct tty *tty, const char *buf, size_t count)
{
    if (!tty || !buf) {
        return -EINVAL;
    }

    if (!tty->driver) {
        return -ENXIO;
    }
    if (!tty->driver->write) {
        return -ENOSYS;
    }

    // TODO: need a write buffer, fill it up then send it to driver->write() until
    // source buffer is drained; don't send char-by-char (let the driver do that)

    ssize_t nwritten = 0;
    for (int i = 0; i < count; i++) {
        char c = buf[i];
        if (O_OPOST(tty)) {
            switch (c) {
                case '\n':
                    if (O_ONLCR(tty)) {
                        char c2 = '\r';
                        tty->driver->write(tty, &c2, 1);
                    }
                    break;
                case '\r':
                    if (O_OCRNL(tty)) {
                        char c2 = '\n';
                        tty->driver->write(tty, &c2, 1);
                    }
                    break;
            }
        }

        int ret = tty->driver->write(tty, &buf[i], 1);
        if (ret < 0) {
            return ret;
        }
        nwritten += ret;    // deliberately ignoring post processing chars
    }

    return nwritten;
}

static int tty_ldisc_ioctl(struct tty *tty, unsigned int num, unsigned long arg)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_recv(struct tty *tty, char *buf, size_t count)
{
    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    for (int i = 0; i < count; i++) {
        if (ring_full(&ldisc_data->iring)) {
            return -EIO;        // TODO: something to indicate buffer full
        }

        // TODO: termios processing (or does that happen on read?)

        ring_put(&ldisc_data->iring, buf[i]);
    }

    return count;
}

static size_t tty_ldisc_recv_room(struct tty *tty)
{
    struct n_tty_ldisc_data *ldisc_data;
    ldisc_data = (struct n_tty_ldisc_data *) tty->ldisc_data;

    return ring_length(&ldisc_data->iring) - ring_count(&ldisc_data->iring);
}
