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
 *         File: src/kernel/drivers/char/tty.c
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <i386/boot.h>
#include <kernel/chdev.h>
#include <kernel/config.h>
#include <kernel/fs.h>
#include <kernel/ohwes.h>
#include <kernel/pool.h>
#include <kernel/queue.h>
#include <kernel/tty.h>

static struct list_node tty_drivers;                // linked list of TTY drivers
static struct tty_ldisc ldiscs[NR_LDISC] = { };     // TTY line disciplines
static struct tty ttys[NR_TTY] = { };               // TTY structs

static struct termios tty_default_termios = {
    .c_line = N_TTY,        // TODO: necessary?
    .c_iflag = ICRNL,
    .c_oflag = OPOST | ONLCR,
    .c_lflag = ECHO | ECHOCTL
};

static void default_write_char(struct tty *tty, char c);

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

int tty_register_driver(struct tty_driver *driver)
{
    if (!driver) {
        return -EINVAL;
    }

    if (!driver->write) {
        return -EINVAL;
    }

    int error = register_chdev(driver->major, driver->name, &tty_fops);
    if (error < 0) {
        return error;
    }

    list_add_tail(&tty_drivers, &driver->driver_list);
    return 0;
}

int tty_register_ldisc(int ldsic_num, struct tty_ldisc *ldisc)
{
    if (ldsic_num < 0 || ldsic_num >= NR_LDISC || !ldisc) {
        return -EINVAL;
    }

    ldiscs[ldsic_num] = *ldisc;
    return 0;
}

int get_tty(dev_t device, struct tty **tty)
{
    if (_DEV_MAJ(device) != TTY_MAJOR || !tty) {
        return -EINVAL;
    }

    int index = _DEV_MIN(device);
    if (index < 1 || index >= NR_TTY) {
        return -ENODEV;
    }

    *tty = &ttys[index];
    return 0;
}

// ----------------------------------------------------------------------------

extern void init_n_tty(void);
extern void init_serial(void);
extern void init_console(void);
extern void init_kb(void);

void init_tty(void)
{
    list_init(&tty_drivers);

    for (int i = 1; i < NR_TTY; i++) {
        ttys[i].device = __mkdev(TTY_MAJOR, i);
    }

    init_n_tty();
    init_serial();
    init_console();
    init_kb();
}

int do_tty_open(struct tty *tty)
{
    int ret;
    struct tty_driver *driver;
    struct list_node *n;

    if (tty->open) {
        return 0;       // TTY already open, no action needed
    }

    // associate termios
    tty->termios = tty_default_termios;

    // associate and open line discipline
    tty->ldisc = &ldiscs[N_TTY];
    if (!tty->ldisc->open) {
        assert(!"where's ldisc->open??");
        return -ENOSYS; // no open fn registered on line discipline! (panic?)
    }
    ret = tty->ldisc->open(tty);
    if (ret) {
        return ret;
    }

    // locate driver for device
    driver = NULL;
    for (list_iterator(&tty_drivers, n)) {
        struct tty_driver *d = list_item(n, struct tty_driver, driver_list);
        if (_DEV_MAJ(tty->device) != d->major) {
            continue;
        }
        if (_DEV_MIN(tty->device) >= d->minor_start &&
            _DEV_MIN(tty->device) < d->minor_start + d->count) {
            driver = d;
            break;
        }
    }
    if (!driver) {
        return -ENXIO;  // no TTY driver registered for device!
    }

    // associate driver with TTY
    tty->driver = *driver;
    tty->line = _DEV_MIN(tty->device) - tty->driver.minor_start;

    // open the tty driver
    if (!tty->driver.open) {
        assert(!"where's driver.open??");
        return -ENOSYS;
    }
    ret = tty->driver.open(tty);
    if (ret) {
        return ret;
    }

    // TODO: need to ensure things get closed if something fails
    // after something else has been opened.

    // TODO: might be better to use a single 'flags' word
    // so we can clear flags in aggregate (and not miss any)
    tty->open = true;
    tty->throttled = false;
    return ret;
}

static int tty_open(struct inode *inode, struct file *file)
{
    int ret;
    struct tty *tty;

    if (!inode || !file) {
        return -EINVAL;
    }

    // locate TTY device
    ret = get_tty(inode->device, &tty);
    if (ret < 0) {
        return -ENODEV; // not a TTY device
    }

    // open the TTY device
    ret = do_tty_open(tty);
    if (ret < 0) {
        return ret;
    }

    // set file state
    tty->file = file;
    file->fops = &tty_fops;
    file->private_data = tty;

    return 0;
}

static int tty_close(struct file *file)
{
    // TODO: flush buffers, close ldisc, close/detach, driver
    assert(!"implement me!");
    return -ENOSYS;
}

static ssize_t tty_read(struct file *file, char *buf, size_t count)
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
    if (!tty->ldisc->read) {
        assert(!"where's ldisc->read??");
        return -ENOSYS;
    }

    return tty->ldisc->read(tty, buf, count);
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
        assert(!"where's ldisc->write??");
        return -ENOSYS;
    }

    return tty->ldisc->write(tty, buf, count);
}

static int tty_ioctl(struct file *file, unsigned int num, unsigned long arg)
{
    // TODO: forward ioctl to ldisc and driver
    return -ENOTTY;
}
