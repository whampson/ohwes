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
 *         File: kernel/char/tty.c
 *      Created: August 17, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <ring.h>
#include <i386/boot.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/char.h>
#include <kernel/fs.h>
#include <kernel/ioctls.h>
#include <kernel/mm.h>
#include <kernel/pool.h>
#include <kernel/task.h>
#include <kernel/tty.h>

#define TTY_MAGIC           (uint32_t) ' ytt'
#define TTY_DRIVER_MAGIC    (uint32_t) 'dytt'
#define TTY_PARANOID        1

#define tty_index(tty) \
    _DEV_MIN((tty)->device) - (tty)->driver.minor_start

// system line discipline table
struct tty_ldisc ldiscs[NR_LDISC];

static list_t tty_drivers;
static pool_t *tty_pool;
static pool_t *termios_pool;

//
// tty file operations
//
static int tty_open(struct inode *, struct file *);
static int tty_close(struct file *);
static ssize_t tty_read(struct file *, char *buf, size_t count);
static ssize_t tty_write(struct file *, const char *buf, size_t count);
static int tty_ioctl(struct file *, int op, void *arg);

static struct file_ops tty_fops = {
    .open = tty_open,
    .close = tty_close,
    .read = tty_read,
    .write = tty_write,
    .ioctl = tty_ioctl,
};

static ssize_t hung_up_tty_read(struct file *, char *buf, size_t count);
static ssize_t hung_up_tty_write(struct file *, const char *buf, size_t count);
static int hung_up_tty_ioctl(struct file *, int op, void *arg);

static struct file_ops hung_up_tty_fops = {
    .read = hung_up_tty_read,
    .write = hung_up_tty_write,
    .ioctl = hung_up_tty_ioctl,
};

//
// ioctl fns
//
static int get_termios(struct tty *tty, struct termios *user_termios);
static int set_termios(struct tty *tty, const struct termios *user_termios);
static int tiocsti(struct tty *tty, const char *user_char);

static bool tty_driver_sane(const struct tty_driver *driver)
{
    return driver && driver->magic == TTY_DRIVER_MAGIC
        && driver->name && driver->device_count > 0
        && driver->refcount && driver->tty_table && driver->termios
        && driver->open && driver->write;
}

static bool tty_sane(const struct tty *tty)
{
    return tty && tty->magic == TTY_MAGIC
        && tty->termios
#if TTY_PARANOID
        && tty_driver_sane(&tty->driver)
#endif
        ;
}

// ----------------------------------------------------------------------------
// public functions

int tty_register_driver(struct tty_driver *driver)
{
    if (!driver || driver->default_termios.c_line >= NR_LDISC
        || !driver->name || driver->device_count <= 0
        || !driver->refcount || !driver->tty_table || !driver->termios
        || !driver->open || !driver->write) {
        return -EINVAL;
    }

    int ret = register_chdev(driver->major, driver->name, &tty_fops);
    if (ret < 0) {
        return ret;
    }
    list_push_back(&tty_drivers, &driver->list);

    driver->magic = TTY_DRIVER_MAGIC;
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

void tty_flush(struct tty *tty)
{
    if (!tty) {
        return;
    }

    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }
}

void tty_hangup(struct tty *tty)
{
    if (!tty) {
        return;
    }

    // TODO: send SIGHUP and SIGCONT

    // replace fops with hung_up_fops
    // TODO: need to do this on all file descriptors that reference the TTY,
    // not just current task's
    struct task *task = current_task();
    for (int i = 0; i < MAX_OPEN; i++) {
        struct file *file = task->files[i];
        if (!file) {
            continue;
        }
        if (!file->inode) {
            continue;
        }
        if (file->private_data != tty) {
            continue;
        }
        // TODO: eventually we flush/sync here...
        file->fops = &hung_up_tty_fops;
    }

    if (tty->driver.hangup) {
        tty->driver.hangup(tty);
    }
}

int tty_hung_up(struct file *file)
{
    if (!file) {
        return -EINVAL;
    }

    return file->fops == &hung_up_tty_fops;
}

// ----------------------------------------------------------------------------
// private functions

extern __init void init_n_tty(void);
extern __init void init_serial_driver(void);
extern __init void init_terminal_driver(void);

__init void init_tty(void)
{
    list_init(&tty_drivers);
    tty_pool = pool_create("tty", NR_TTY, sizeof(struct tty), 0);
    termios_pool = pool_create("termios", NR_TTY, sizeof(struct termios), 0);

    init_n_tty();
    init_serial_driver();
    init_terminal_driver();

    // TODO: figure out which TTYs are valid
    // (like, no ttyS3 if PC has 1 serial port)
}

static const struct tty_driver * get_tty_driver(dev_t device)
{
    if (_DEV_MAJ(device) != TTY_MAJOR) {
        return NULL;
    }

    for (list_iterator(it, &tty_drivers)) {
        struct tty_driver *d = list_item(it, struct tty_driver, list);
        if (_DEV_MAJ(device) != d->major) {
            continue;
        }
        if (_DEV_MIN(device) < d->minor_start ||
            _DEV_MIN(device) >= d->minor_start + d->device_count) {
            continue;
        }
        return d;
    }

    return NULL;
}

static void tty_free_mem(struct tty *tty, int index)
{
    // TODO: control w/ flag in case we want to persist termios
    if (tty->driver.termios[index]) {
        pool_free(termios_pool, tty->driver.termios[index]);
        tty->driver.termios[index] = NULL;
    }

#if TTY_PARANOID
    if (tty != tty->driver.tty_table[index]) {
        panic("dev%d,%d: close: "
            "TTY in driver table does not match TTY in file descriptor!",
            _DEV_MAJ(tty->device), _DEV_MIN(tty->device));
    }
#endif

    // free TTY structure
    pool_free(tty_pool, tty);
    tty->driver.tty_table[index] = NULL;

    // decrease ref count
    if (--(*tty->driver.refcount) < 0) {
        *tty->driver.refcount = 0;
#if TTY_PARANOID
        pr_alert("dev%d,%d: close: tty->driver.refcount < 0!\n",
            _DEV_MAJ(tty->device), _DEV_MIN(tty->device));
#endif
    }
}

static void tty_shutdown(struct tty *tty)
{
    // we always call close on the driver, so it can do cleanup things
    if (tty->driver.close) {
        tty->driver.close(tty);
    }

    if (--tty->refcount < 0) {
        tty->refcount = 0;
#if TTY_PARANOID
        pr_alert("dev%d,%d: close: tty->refcount < 0!\n",
            _DEV_MAJ(tty->device), _DEV_MIN(tty->device));
#endif
    }
    if (tty->refcount) {
        goto shutdown_done;
    }

    // close line disc. on final tty shutdown
    if (tty->ldisc.close) {
        tty->ldisc.close(tty);
    }
    // TODO: reset line disc. to N_TTY?

    tty_free_mem(tty, tty_index(tty));
    tty->magic = 0;

shutdown_done:
    return;
}

int tty_startup(dev_t device, struct tty **out_tty)
{
    int ret;
    struct tty *tty;
    const struct tty_driver *driver;
    int index;

    if (!out_tty) {
        return -EINVAL;
    }

    // locate driver for TTY device class
    driver = get_tty_driver(device);
    if (!driver || !driver->open) {
        return -ENODEV;  // no driver registered for TTY device!
    }
    if (!tty_driver_sane(driver)) {
        return -EIO;
    }

    // check if driver instance already exists for this device
    index = _DEV_MIN(device) - driver->minor_start;
    if (driver->tty_table[index]) {
        tty = driver->tty_table[index];
        if (!tty_sane(tty)) {
            return -EIO;
        }
        goto startup_done;
    }

    ret = 0;

    // create new driver instance
    driver->tty_table[index] = pool_alloc(tty_pool, 0);
    if (!driver->tty_table[index]) {
        ret = -ENOMEM;
        goto fail;
    }
    tty = driver->tty_table[index];
    (*driver->refcount)++;

    // create new termios   TODO: flag for this
    driver->termios[index] = pool_alloc(termios_pool, 0);
    if (!driver->termios[index]) {
        ret = -ENOMEM;
        goto fail_dealloc;
    }
    tty->termios = driver->termios[index];

    // initialize TTY
    tty->magic = TTY_MAGIC;
    tty->device = device;
    *tty->termios = driver->default_termios;
    tty->driver = *driver;
    tty->ldisc = ldiscs[tty->termios->c_line];

    // open line discipline
    if (tty->ldisc.open) {
        ret = tty->ldisc.open(tty);
        if (ret) {
            goto fail_cleanup_ldisc;
        }
    }

startup_done:
    *out_tty = tty;
    return 0;

fail_cleanup_ldisc:
    if (tty->driver.close) {
        tty->driver.close(tty);
    }
fail_dealloc:
    tty_free_mem(tty, index);
fail:
    return ret;
}

static int tty_open(struct inode *inode, struct file *file)
{
    int ret;
    struct tty *tty;

    if (!inode || !file) {
        return -EINVAL;
    }

    ret = tty_startup(inode->device, &tty);
    if (ret) {
        return ret;
    }
    tty->refcount++;

    ret = tty->driver.open(tty);
    if (ret) {
        tty_shutdown(tty);  // handles refcount decrement
        return ret;
    }

    file->fops = &tty_fops;
    file->inode = inode;
    file->private_data = tty;
    return 0;
}

static int tty_close(struct file *file)
{
    struct tty *tty;

    if (!file) {
        return -EINVAL;
    }
    if (!file->inode) {
        return -ENXIO;
    }

    tty = (struct tty *) file->private_data;
    if (!tty_sane(tty)) {
        return -EIO;
    }

#ifdef TTY_PARANOID
    {
        int index = _DEV_MIN(file->inode->device) - tty->driver.minor_start;
        const struct tty_driver *driver = get_tty_driver(file->inode->device);
        if (!driver) {
            panic("dev%d,%d: %s: no TTY driver registered!",
                _DEV_MAJ(tty->device), _DEV_MIN(tty->device), __FUNCTION__);
        }
        if (!tty_driver_sane(driver)) {
            panic("dev%d,%d: %s: bad TTY driver!",
                _DEV_MAJ(tty->device), _DEV_MIN(tty->device), __FUNCTION__);
        }
        if (driver->tty_table[index] != tty) {
            panic("dev%d,%d: %s: "
                "TTY in driver table does not match TTY in file descriptor!",
                _DEV_MAJ(tty->device), _DEV_MIN(tty->device), __FUNCTION__);
        }
    }
#endif

    tty_shutdown(tty);
    file->fops = NULL;
    file->inode = NULL;
    file->private_data = NULL;
    return 0;
}

static ssize_t tty_read(struct file *file, char *buf, size_t count)
{
    struct tty *tty;

    if (!file || !buf) {
        return -EINVAL;
    }
    if (!file->inode || _DEV_MAJ(file->inode->device) != TTY_MAJOR) {
        return -ENXIO;
    }

    tty = (struct tty *) file->private_data;
    if (!tty_sane(tty)) {
        return -EIO;
    }

    if (!tty->ldisc.read) {
        return -ENOSYS;
    }
    return tty->ldisc.read(tty, file, buf, count);
}

static ssize_t tty_write(struct file *file, const char *buf, size_t count)
{
    struct tty *tty;

    if (!file || !buf) {
        return -EINVAL;
    }
    if (!file->inode || _DEV_MAJ(file->inode->device) != TTY_MAJOR) {
        return -ENXIO;
    }

    tty = (struct tty *) file->private_data;
    if (!tty_sane(tty)) {
        return -EIO;
    }

    if (!tty->ldisc.write) {
        return -ENOSYS;
    }
    return tty->ldisc.write(tty, file, buf, count);
}

static int tty_ioctl(struct file *file, int op, void *arg)
{
    int ret;
    struct tty *tty;

    if (!file) {
        return -EINVAL;
    }
    if (!file->inode || _DEV_MAJ(file->inode->device) != TTY_MAJOR) {
        return -ENXIO;
    }

    tty = (struct tty *) file->private_data;
    if (!tty_sane(tty)) {
        return -EIO;
    }

    switch (op) {
        case TCGETS:
            return get_termios(tty, (struct termios *) arg);

        case TCSETS:
            return set_termios(tty, (const struct termios *) arg);

        case TIOCSTI:
            return tiocsti(tty, (const char *) arg);
    }

    // forward to driver and ldisc
    ret = -ENOTTY;
    if (tty->driver.ioctl) {
        ret = tty->driver.ioctl(tty, op, arg);
        if (ret != -ENOTTY) {
            return ret;
        }
    }
    if (tty->ldisc.ioctl) {
        ret = tty->ldisc.ioctl(tty, file, op, arg);
        if (ret != -ENOTTY) {
            return ret;
        }
    }

    return ret;
}

static int get_termios(struct tty *tty, struct termios *user_termios)
{
    if (!copy_to_user(user_termios, tty->termios, sizeof(struct termios))) {
        return -EFAULT;
    }
    return 0;
}

static int set_termios(struct tty *tty, const struct termios *user_termios)
{
    // TODO: flush buffers, prevent new input, etc. before overwriting termios
    if (!copy_from_user(tty->termios, user_termios, sizeof(struct termios))) {
        return -EFAULT;
    }
    return 0;
}

static int tiocsti(struct tty *tty, const char *user_char)
{
    char c;
    if (!copy_from_user(&c, user_char, sizeof(char))) {
        return -EFAULT;
    }

    tty->ldisc.recv(tty, &c, 1);
    return 0;
}

static ssize_t hung_up_tty_read(struct file *file, char *buf, size_t count)
{
    return -EIO;
}

static ssize_t hung_up_tty_write(struct file *file, const char *buf, size_t count)
{
    return -EIO;
}

static int hung_up_tty_ioctl(struct file *file , int op, void *arg)
{
    return -ENOTTY;
}
