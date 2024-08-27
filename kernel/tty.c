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

static struct tty ttys[NR_TTYS];   // TODO: dynamically alloc each tty
static struct tty_ldisc ldiscs[NR_LDISCS]; // default line disciplines

// struct tty_driver *tty_drivers;   // TODO: linked list

static struct termios default_termios = {
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

static struct file_ops tty_fops = {
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
static ssize_t tty_ldisc_read(struct tty *, struct file *, char *buf, size_t count);
static ssize_t tty_ldisc_write(struct tty *, struct file *, const char *buf, size_t count);
static int tty_ldisc_ioctl(struct tty *, struct file *, unsigned int num, unsigned long arg);
static ssize_t tty_ldisc_recv(struct tty *, char *buf, size_t count);
static size_t  tty_ldisc_recv_room(struct tty *);

static struct tty_ldisc n_tty = {
    .num = N_TTY,
    .name = "n_tty",
    .open = tty_ldisc_open,
    .close = tty_ldisc_close,
    .read = tty_ldisc_read,
    .write = tty_ldisc_write,
    .ioctl = tty_ldisc_ioctl,
    .recv = tty_ldisc_recv,
    .recv_room = tty_ldisc_recv_room,
};

// ----------------------------------------------------------------------------

void init_tty(void)
{
    ldiscs[N_TTY] = n_tty;

    if (chdev_register(TTY_DEVICE, "tty", &tty_fops)) {
        panic("could not register tty!");
    }

    if (chdev_register(TTYS_DEVICE, "ttyS", &tty_fops)) {
        panic("could not register ttyS!");
    }
}

// ----------------------------------------------------------------------------

extern struct tty_driver console_driver;
extern struct tty_driver serial_driver;

static int tty_open(struct inode *inode, struct file *file)
{
    int ret;
    struct tty *tty;

    // TODO: use inode data (device Id, etc.) to find the appropriate
    // driver, create a private data instance for output buffers, etc.
    switch (inode->dev_id) {
        case TTY_DEVICE:
            tty = &ttys[0]; // TODO: allocate tty memory
            tty->driver = &console_driver;
            break;
        case TTYS_DEVICE:
            tty = &ttys[1]; // !!! BAD BAD BAD
            tty->driver = &serial_driver;
            break;
        default:
            panic("invalid tty device!");
            return -EINVAL;
    }
    tty->driver->default_termios = default_termios; // TODO: move this to driver init
    tty->termios = &tty->driver->default_termios;


    // open line discipline
    tty->ldisc = &ldiscs[N_TTY];
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
    ret = tty->driver->open(tty, file);
    if (ret) {
        return ret;
    }

    // set file state
    file->fops = &tty_fops;
    file->private_data = tty;
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

    return tty->ldisc->write(tty, file, buf, count);
}

static int tty_ioctl(struct file *file, unsigned int num, unsigned long arg)
{
    // TODO: forward ioctl to ldisc or driver
    return -ENOSYS;
}

// ----------------------------------------------------------------------------

static int tty_ldisc_open(struct tty *tty)
{
    if (!tty) {
        return -EINVAL;
    }

    ring_init(&tty->iring, tty->iring_buf, TTY_BUFFER_SIZE);
    return 0;
}

static int tty_ldisc_close(struct tty *tty)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_read(
    struct tty *tty, struct file *file, char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_write(
    struct tty *tty, struct file *file, const char *buf, size_t count)
{
    if (!tty || !file || !buf) {
        return -EINVAL;
    }

    if (!tty->driver) {
        return -ENXIO;
    }
    if (!tty->driver->write) {
        return -ENOSYS;
    }

    // TODO: termios processing

    return tty->driver->write(tty, file, buf, count);
}

static int tty_ldisc_ioctl(struct tty *tty, struct file *file, unsigned int num, unsigned long arg)
{
    return -ENOSYS;
}

static ssize_t tty_ldisc_recv(struct tty *tty, char *buf, size_t count)
{
    return -ENOSYS;
}

static size_t tty_ldisc_recv_room(struct tty *tty)
{
    return -ENOSYS;
}
