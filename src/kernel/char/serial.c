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
 *         File: kernel/char/serial.c
 *      Created: December 24, 2024
 *       Author: Wes Hampson
 *
 * 16550 UART driver.
 * =============================================================================
 */

//
// Serial Drivers:
//     https://www.linux.it/~rubini/docs/serial/serial.html
//
// Linux serial driver came in handy as well:
//     https://elixir.bootlin.com/linux/2.2.26/source/drivers/char/serial.c
//

#include <assert.h>
#include <errno.h>
#include <ring.h>
#include <signal.h>
#include <i386/bitops.h>
#include <i386/debug.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/io.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

#include <i386/x86.h>

// print debugging info
#define CHATTY_COM          1
#define PRINT_TX_ENABLE     0
#define PRINT_LINE_STATUS   1
#define PRINT_MODEM_STATUS  0

// counts of things
#define FIFO_DEPTH          16          // hardware FIFO depth (assumed)
#define RECV_MAX            128         // max chars to receive per interrupt
#define XMIT_MAX            FIFO_DEPTH  // max chars to send per interrupt
#define INTR_MAX            16          // max num passes per interrupt
#define CONSOLE_IO_POLLCNT  (1 << 16)   // console poll iterations (TODO: real timer)

// check if a COM register returned a bad value
#define ERR_CHK(x)          ((x) == 0 || (x) == 0xFF)

struct com g_com[NR_COM];

// "serial" prefix refers to TTY functions
// "com" prefix refers to UART functions

static int serial_driver_refcount;
static struct tty *serial_ttys[NR_COM];
static struct termios *serial_termios[NR_COM];

static int serial_open(struct tty *);
static void serial_close(struct tty *);
static int serial_ioctl(struct tty *tty, int op, void *arg);
static int serial_write(struct tty *tty, const char *buf, size_t count);
static size_t serial_write_room(struct tty *);
static void serial_flush(struct tty *);
static void serial_clear(struct tty *);
static void serial_unthrottle(struct tty *);
static void serial_throttle(struct tty *);
static void serial_start(struct tty *);
static void serial_stop(struct tty *);
static void serial_hangup(struct tty *);

static struct tty_driver serial_driver = {
    .major = TTY_MAJOR,
    .minor_start = TTYS_MIN,
    .device_count = NR_COM,
    .name = "ttyS",
    .refcount = &serial_driver_refcount,
    .tty_table = serial_ttys,
    .termios = serial_termios,
    .default_termios = TTY_STD_TERMIOS,
    .open = serial_open,
    .close = serial_close,
    .ioctl = serial_ioctl,
    .write = serial_write,
    .write_room = serial_write_room,
    .flush = serial_flush,
    .clear = serial_clear,
    .unthrottle = serial_unthrottle,
    .throttle = serial_throttle,
    .start = serial_start,
    .stop = serial_stop,
    .hangup = serial_hangup,
};

static void com1_irq(int irq, struct iregs *regs);
static void com2_irq(int irq, struct iregs *regs);
static void com_interrupt(struct com *com, struct iregs *regs);

static int com_write(struct com *com, const char *buf, size_t count);

static void shadow_regs(struct com *com);
static bool set_baud(struct com *com, int baud_divisor);
static bool set_mode(struct com *com,
    enum uart_word_length wls, enum uart_parity parity, enum uart_stop_bits stb);
static void set_fifo(struct com *com, bool enabled, enum uart_recv_trig depth);

static void shutdown(struct com *com);

static void tx_enable(struct com *com);
static void tx_disable(struct com *com);

static void check_line_status(struct com *com);
static void check_modem_status(struct com *com);

static void send_chars(struct com *com);
static void recv_chars(struct com *com);

//
// ioctl fns
//
static int get_modem_info(struct com *com, int *user_info);
static int set_modem_info(struct com *com, const int *user_info);
static int get_modem_stats(struct com *com, struct serial_stats *user_stats);

// ----------------------------------------------------------------------------

struct com * get_com(int num)
{
    static_assert(COM4 - COM1 + 1 == NR_COM, "NR_COM");
    if (num < COM1 || num > COM4) {
        panic("invalid COM number %d", num);
    }
    return &g_com[num - COM1];
}

int tty_get_com(struct tty *tty, struct com **com)
{
    if (!tty || !com) {
        return -EINVAL;
    }

    if (_DEV_MAJ(tty->device) != TTY_MAJOR) {
        return -ENODEV; // char device is not a TTY
    }

    int minor = _DEV_MIN(tty->device);
    if (minor < TTYS_MIN || minor > TTYS_MAX) {
        return -ENXIO;  // TTY device is not a COM
    }

    *com = get_com(1 + __tty_driver_index(tty));    // COMs indexed at 1
    return 0;
}

// ----------------------------------------------------------------------------
//                        Serial Console Interface
// Unlike the TTY, the serial console does not use interrupts, instead relying
// on polling to determine when to transmit and receive characters.

#if CONFIG_SERIAL_CONSOLE

static inline int wait_and_recv(struct com *com)
{
    int timeout = CONSOLE_IO_POLLCNT;
    while (timeout > 0 && (com_in(com, UART_LSR) & UART_LSR_DR) == 0) {
        timeout--;
    }
    if (timeout > 0) {
        return (unsigned char) com_in(com, UART_RX);
    }
    return EOF;
}

static inline void wait_and_send(struct com *com, char c)
{
    while ((com_in(com, UART_LSR) & UART_LSR_THRE) == 0);
    com_out(com, UART_TX, c);
}

static dev_t serial_console_device(struct console *cons)
{
    return __mkserdev(cons->number);
}

static bool serial_console_setup(struct console *cons)
{
    struct com *com;
    uint8_t data;

    com = get_com(cons->number);

#if CONFIG_EARLY_PRINT
    // COM struct might not have been initialized yet...
    // do some partial initialization here
    if (!com->num) {
        com->num = cons->number;
        if (com->num < COM1 || com->num > NR_COM) {
            pr_warn("serial_console_setup: invalid console number %d\n", cons->number);
            return false;
        }
    }
    assert(com->num == cons->number);
    if (!com->io_port) {
        com->io_port = get_com_port(com->num);
        if (!com->io_port) {
            pr_warn("serial_console_setup: no I/O port attached to com%d\n", com->num);
            return false;
        }
    }
#endif

#if CONFIG_SERIAL_DEBUG
    if (com->io_port == __mkcomport(CONFIG_SERIAL_DEBUG_COM)) {
        panic("serial console cannot share COM port with serial debugger!");
        return false;
    }
#endif

    // check for UART existence
    com_out(com, UART_SCR, 0);
    com_out(com, UART_SCR, 0x55);
    data = com_in(com, UART_SCR);
    if (data != 0x55) {
        pr_warn("serial_console_setup: no UART attached to I/O port %Xh", com->io_port);
        return false;
    }

    // set baud rate
    com_out(com, UART_LCR, UART_LCR_DLAB);
    com_out(com, UART_DLM, CONFIG_SERIAL_CONSOLE_BAUD >> 8);
    com_out(com, UART_DLL, CONFIG_SERIAL_CONSOLE_BAUD & 0xFF);

    // configure port params
    com_out(com, UART_LCR, __mkuart_lcr(WORDLEN_8, PARITY_NONE, STOPBITS_1));
    com_out(com, UART_MCR, UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
    com_out(com, UART_IER, 0);
    com_out(com, UART_FCR, __mkuart_fcr(DMA_OFF, FIFO_ON, FIFO_TRIG_14, 1, 1));

    // clear pending reads
    (void) com_in(com, UART_LSR);
    (void) com_in(com, UART_MSR);
    (void) com_in(com, UART_IIR);

    return true;
}

static ssize_t serial_console_write(struct console *cons, const char *buf, size_t count)
{
    struct com *com;
    uint32_t flags;

    com = get_com(cons->number);
    assert(com->num == cons->number);

    cli_save(flags);
    for (size_t i = 0; i < count; i++) {
        if (buf[i] == '\n') {
            wait_and_send(com, '\r');
        }
        wait_and_send(com, buf[i]);
    }
    restore_flags(flags);

    return (ssize_t) count;
}

static int serial_console_getc(struct console *cons)
{
    struct com *com = get_com(cons->number);
    assert(com->num == cons->number);

    return  wait_and_recv(com);
}

struct console serial_console =
{
    .name = "ttyS",
    .number = CONFIG_SERIAL_CONSOLE_COM,
    .flags = _CONSOLE_FLAG_KLOG,
    .device = serial_console_device,
    .init = serial_console_setup,
    .write = serial_console_write,
    .read_char = serial_console_getc,
};

#endif

// ----------------------------------------------------------------------------

__init void init_serial_driver(void)
{
    struct com *com;
    irq_mask(IRQ_COM1);
    irq_mask(IRQ_COM2);

    if (tty_register_driver(&serial_driver)) {
        panic("unable to register serial driver!");
    }

    for (int i = COM1; i <= NR_COM; i++) {
        // locate and init com struct
        com = get_com(i);
        com->num = i;
        com->io_port = get_com_port(i);
        assert(com->io_port);

        // check for existence of UART device
        if (com_in(com, UART_IER) == 0xFF) {
            continue;   // not a valid value, probably no UART here
        }

        // read/write scratch reg to determine port existence and sanity
        const int Magic = 0x55;
        com_out(com, UART_SCR, 0);
        com_out(com, UART_SCR, Magic);
        if (com_in(com, UART_SCR) != Magic) {
            pr_warn("com%d: probe failed\n", com->num);
            continue;
        }

        com->valid = true;
        pr_info("com%d: detected COM on port %Xh\n", com->num, com->io_port);

        // collect initial register state
        shadow_regs(com);

        // TODO: kind of want to remove this and check explicitly for kernel debug reservation
        if (reserve_io_range(com->io_port, 8, "serial") < 0) {
            com->reserved = true;   // serial port is reserved by another driver (e.g. debug interface)
            pr_warn("com%d: COM port reserved, not usable as a tty\n", com->num);
            continue;
        }

#if CONFIG_SERIAL_CONSOLE
        if (serial_console.number == com->num) {
            register_console(&serial_console);
            pr_info("com%d: registered as console device\n", com->num);
        }
#endif
    }

    irq_register(IRQ_COM1, com1_irq);   // TODO: disallow duplicate IH at API level
    irq_register(IRQ_COM2, com2_irq);
    irq_unmask(IRQ_COM1);
    irq_unmask(IRQ_COM2);
}

// ----------------------------------------------------------------------------
//                              Serial TTY Interface

static int serial_open(struct tty *tty)
{
    uint32_t flags;
    struct com *com;
    int ret;

    ret = tty_get_com(tty, &com);   // TODO: sanity check
    if (ret < 0) {
        return ret;
    }

    if (!com->valid) {
        return -EIO;    // port does not exist
    }
    if (com->open) {    // TODO: refcount instead of 'busy'
        assert(com->tty);
        return -EBUSY;  // COM already open      TODO: is this correct behavior?
    }
    if (com->reserved) {
        return -EBUSY;  // port reserved by another driver (e.g. serial debug interface)
    }

    cli_save(flags);

    // initialize ring buffer
    ring_init(&com->tx_ring, com->_txbuf, sizeof(com->_txbuf));
    com->xchar = 0;

    // disable all interrupts
    com_out(com, UART_IER, 0);

    // set default baud rate 9600
    if (!set_baud(com, BAUD_9600)) {
        ret = -EIO;
        goto done;
    }

    // set default mode (8N1; 8 bits, no parity, 1 stop bit)
    if (!set_mode(com, WLS_8, PARITY_NONE, STB_1)) {
        ret = -EIO;
        goto done;
    }

    // enable FIFOs and set default trigger level (14 bytes)
    set_fifo(com, true, FIFO_TRIG_14);

    // set modem control
    com->mcr._value = 0;
    com->mcr.dtr = 1;   // data terminal ready
    com->mcr.rts = 1;   // request to send
    com->mcr.out2 = 1;  // irq enable
    com_out(com, UART_MCR, com->mcr._value);

    // ensure no interrupts are pending
    (void) com_in(com, UART_RX);
    (void) com_in(com, UART_LSR);
    (void) com_in(com, UART_MSR);
    (void) com_in(com, UART_IIR);

    // enable interrupts
    com->ier._value = 0;
    com->ier.rda = 1;   // interrupt when data ready to read
    com->ier.rls = 1;   // interrupt when line status changes
    com->ier.ms = 1;    // interrupt when modem status changes
    com_out(com, UART_IER, com->ier._value);

    // reset statistics
    zeromem(&com->stats, sizeof(struct serial_stats));

    // check and save final register state
    shadow_regs(com);
    if (ERR_CHK(com->ier._value) || ERR_CHK(com->mcr._value)) {
        ret = -EIO;
        goto done;
    }

    com->tty = tty;
    com->open = true;

#if CHATTY_COM
    pr_debug("com%d: opened, port=%Xh div=%d lcr=%02Xh mcr=%02Xh iir=%02Xh ier=%02Xh\n",
        com->num, (int) com->io_port,
        (int) com->baud_divisor,
        (int) com->lcr._value, (int) com->mcr._value,
        (int) com->iir._value, (int) com->ier._value);
#endif

done:
    restore_flags(flags);
    return ret;
}

static void serial_close(struct tty *tty)
{
    uint32_t flags;
    struct com *com;
    int ret;

    // TODO: need a refcount

    ret = tty_get_com(tty, &com);
    if (ret < 0 || !com->open) {
        return;
    }

    cli_save(flags);

    // flush transmit buffer
    serial_flush(tty);
    // TODO: wait for flush to complete
    // TODO: this also relies on interrupts being enabled, meaning this will
    // currently NOT flush any pending chars because interrupts are disabled...

    // shut down COM port
    shutdown(com);

    // clear buffers
    if (tty->driver.clear) {
        tty->driver.clear(tty);
    }
    if (tty->ldisc.clear) {
        tty->ldisc.clear(tty);
    }

    com->tty = NULL;
    com->open = false;

#if CHATTY_COM
    pr_debug("com%d: closed\n", com->num);
#endif

    restore_flags(flags);
}

static int serial_ioctl(struct tty *tty, int op, void *arg)
{
    int ret;
    struct com *com;

    ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    switch (op) {
        case TIOCMGET:
            return get_modem_info(com, (int *) arg);

        case TIOCMSET:
            return set_modem_info(com, (const int * ) arg);

        case TIOCGICOUNT:
            return get_modem_stats(com, (struct serial_stats *) arg);
    }

    return -ENOTTY;
}

static int com_write(struct com *com, const char *buf, size_t count)
{
    const char *p;
    uint32_t flags;
    cli_save(flags);

    // fill the TX buffer
    p = buf;
    while (count > 0 && !ring_full(&com->tx_ring)) {
        ring_push_back(&com->tx_ring, *p, char);
        p++; count--;
    }

    restore_flags(flags);
    return p - buf;
}

static int serial_write(struct tty *tty, const char *buf, size_t count)
{
    struct com *com;
    uint32_t flags;
    int ret;

    // check params
    if (!buf) {
        return -EINVAL;
    }

    // get COM struct
    ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    cli_save(flags);

    ret = com_write(com, buf, count);
    if (ret > 0 && !tty->stopped && !tty->hw_stopped) {
        tx_enable(com);
    }

    restore_flags(flags);
    return ret;
}

static size_t serial_write_room(struct tty *tty)
{
    uint32_t flags;
    struct com *com;
    size_t room;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return 0;
    }

    cli_save(flags);
    room = ring_capacity(&com->tx_ring) - ring_count(&com->tx_ring);
    restore_flags(flags);

    return room;
}

static void serial_flush(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

    cli_save(flags);
    if (!ring_empty(&com->tx_ring) && !tty->stopped && !tty->hw_stopped) {
        tx_enable(com);
    }
    restore_flags(flags);
}

static void serial_clear(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

    cli_save(flags);
    ring_clear(&com->tx_ring);
    restore_flags(flags);
}

static void serial_unthrottle(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

    cli_save(flags);

    // re-enable receive interrupt now that receiver has drained
    com->ier._value = com_in(com, UART_IER);
    if (!com->ier.rda) {
#if CHATTY_COM
        pr_debug("com%d: receiver enabled\n", com->num);
#endif
        com->ier.rda = 1;
        com_out(com, UART_IER, com->ier._value);
    }

    // tell software transmitter to start sending
    if (I_IXOFF(tty)) {
#if CHATTY_COM
        pr_debug("com%d: IXOFF: tx ^%c START\n", com->num, CC_START(tty) ^ 0x40);
#endif
        com->xchar = CC_START(tty);
        tx_enable(com);
    }

    // tell hardware transmitter to start sending
    if (C_CRTSCTS(tty)) {
#if CHATTY_COM
        pr_debug("com%d: CRTSCTS: rts=1 HW_START\n", com->num);
#endif
        com->mcr._value = com_in(com, UART_MCR);
        com->mcr.rts = 1;
        com_out(com, UART_MCR, com->mcr._value);
    }

    restore_flags(flags);
}

static void serial_throttle(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

    cli_save(flags);

    // tell software transmitter to stop sending
    if (I_IXOFF(tty)) {
#if CHATTY_COM
        pr_debug("com%d: IXOFF: tx ^%c STOP\n", com->num, CC_STOP(tty) ^ 0x40);
#endif
        com->xchar = CC_STOP(tty);
        tx_enable(com);
    }

    // tell hardware transmitter to stop sending
    if (C_CRTSCTS(tty)) {
#if CHATTY_COM
        pr_debug("com%d: CRTSCTS: rts=0 HW_STOP\n", com->num);
#endif
        com->mcr._value = com_in(com, UART_MCR);
        com->mcr.rts = 0;
        com_out(com, UART_MCR, com->mcr._value);
    }

    restore_flags(flags);
}

static void serial_start(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

#if CHATTY_COM
    pr_debug("com%d: start\n", com->num);
#endif

    cli_save(flags);
    if (!ring_empty(&com->tx_ring)) {
        tx_enable(com);
    }
    restore_flags(flags);
}

static void serial_stop(struct tty *tty)
{
    uint32_t flags;
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

#if CHATTY_COM
    pr_debug("com%d: stop\n", com->num);
#endif

    cli_save(flags);
    tx_disable(com);
    restore_flags(flags);
}

static void serial_hangup(struct tty *tty)
{
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return;
    }

    serial_clear(tty);
    if (tty->ldisc.clear) {
        tty->ldisc.clear(tty);
    }

    shutdown(com);

    com->tty = NULL;
    com->open = false;

#if CHATTY_COM
    pr_debug("com%d: hangup, closed\n", com->num);
#endif
}

// ----------------------------------------------------------------------------
//                          COM Port Interface

static void shutdown(struct com *com)
{
    // disable interrupts
    com->ier._value = 0;
    com_out(com, UART_IER, com->ier._value);

    // disable IRQ
    com->mcr.out2 = 0;

    // hang up
    if (!com->tty || C_HUPCL(com->tty)) {
        com->mcr.dtr = 0;
        com->mcr.rts = 0;
    }
    com_out(com, UART_MCR, com->mcr._value);

    // disable fifos
    set_fifo(com, false, 0);

    // read regs to clear/reset things
    (void) com_in(com, UART_IIR);
    (void) com_in(com, UART_MSR);
    (void) com_in(com, UART_LSR);
    (void) com_in(com, UART_RX);
}

static void shadow_regs(struct com *com)
{
    // shadow register state
    com->ier._value = com_in(com, UART_IER);
    com->iir._value = com_in(com, UART_IIR);
    com->lcr._value = com_in(com, UART_LCR);
    com->mcr._value = com_in(com, UART_MCR);
    com->lsr._value = com_in(com, UART_LSR);
    com->msr._value = com_in(com, UART_MSR);
}

static bool set_baud(struct com *com, int baud_divisor)
{
    uint8_t div_lo, div_hi;
    uint8_t lcr;

    // calculate divisor
    div_lo = baud_divisor & 0xFF;
    div_hi = (baud_divisor >> 8) & 0xFF;

    // set DLAB=1 so we can access the divisor regs
    lcr = com_in(com, UART_LCR);
    com_out(com, UART_LCR, lcr | UART_LCR_DLAB);

    // set the divisor and readback
    com_out(com, UART_DLL, div_lo);
    com_out(com, UART_DLM, div_hi);
    com->baud_divisor = com_in(com, UART_DLL);
    com->baud_divisor |= com_in(com, UART_DLM) << 8;

    // if readback failed, we might have a bad COM port
    if (ERR_CHK(com->baud_divisor) ) {
        pr_error("com%d: error: unable to set baud rate (div=%Xh)\n",
                com->num, baud_divisor);
        return false;
    }

    // otherwise, we're golden, clear the DLAB bit and exit
    com_out(com, UART_LCR, lcr & ~UART_LCR_DLAB);
    return true;
}

static bool set_mode(struct com *com,
    enum uart_word_length wls, enum uart_parity parity, enum uart_stop_bits stb)
{
    struct lcr lcr;
    uint8_t lcr_rdbk;

    // program the line control register
    lcr._value = 0;
    lcr.word_length = wls;
    lcr.parity = parity;
    lcr.stop_bits = stb;
    com_out(com, UART_LCR, lcr._value);

    // readback for sanity
    lcr_rdbk = com_in(com, UART_LCR);
    if (ERR_CHK(lcr_rdbk) || lcr_rdbk != lcr._value) {
        pr_error("com%d: error: unable to set line control (lcr=%Xh, lcr_rdbk=%Xh)\n",
            com->num, lcr._value, lcr_rdbk);
        return false;
    }

    return true;
}

static void set_fifo(struct com *com, bool enabled, enum uart_recv_trig depth)
{
    struct fcr fcr;

    com->fifo_trig = depth;
    com->fifo_enabled = enabled;

    // program FIFO control register
    fcr._value = 0;
    fcr.enable = enabled;
    if (enabled) {
        // fcr.dma = 1;
        fcr.rx_reset = 1;
        fcr.tx_reset = 1;
        fcr.trig = depth;
    }
    com_out(com, UART_FCR, fcr._value);
}

static int get_modem_info(struct com *com, int *user_info)
{
    int sts, ctl;
    int result;
    uint32_t flags;

    cli_save(flags);
    check_modem_status(com);
    sts = com->msr._value;
    ctl = com->mcr._value;
    restore_flags(flags);

    result = 0;
    result |= ((ctl & UART_MCR_DTR)  ? TIOCM_DTR  : 0)
           |  ((ctl & UART_MCR_RTS)  ? TIOCM_RTS  : 0)
           |  ((ctl & UART_MCR_OUT1) ? TIOCM_OUT1 : 0)
           |  ((ctl & UART_MCR_OUT2) ? TIOCM_OUT2 : 0)
           |  ((sts & UART_MSR_CTS)  ? TIOCM_CTS  : 0)
           |  ((sts & UART_MSR_DCD)  ? TIOCM_CD   : 0)
           |  ((sts & UART_MSR_RI)   ? TIOCM_RI   : 0)
           |  ((sts & UART_MSR_DSR)  ? TIOCM_DSR  : 0);

    if (!copy_to_user(user_info, &result, sizeof(int))) {
        return -EFAULT;
    }
    return 0;
}

static int set_modem_info(struct com *com, const int *user_info)
{
    int info;
    uint32_t flags;

    if (!copy_from_user(&info, user_info, sizeof(int))) {
        return -EFAULT;
    }

    cli_save(flags);
    com->mcr._value &= ~(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT1 | UART_MCR_OUT2);
    com->mcr._value |= ((info & TIOCM_DTR) ? UART_MCR_DTR : 0)
                    |  ((info & TIOCM_RTS) ? UART_MCR_RTS : 0)
                    |  ((info & TIOCM_OUT1)? UART_MCR_OUT1: 0)
                    |  ((info & TIOCM_OUT2)? UART_MCR_OUT2: 0);
    com_out(com, UART_MCR, com->mcr._value);
    restore_flags(flags);

    return 0;
}

static int get_modem_stats(struct com *com, struct serial_stats *user_stats)
{
    struct serial_stats stats;
    uint32_t flags;

    cli_save(flags);
    stats = com->stats;
    restore_flags(flags);

    if (!copy_to_user(user_stats, &stats, sizeof(struct serial_stats))) {
        return -EFAULT;
    }
    return 0;
}

static void tx_enable(struct com *com)
{
    com->ier._value = com_in(com, UART_IER);
    if (!com->ier.thre) {
#if CHATTY_COM && PRINT_TX_ENABLE
        pr_debug("com%d: tx enable\n", com->num);
#endif
        com->ier.thre = 1;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void tx_disable(struct com *com)
{
    com->ier._value = com_in(com, UART_IER);
    if (com->ier.thre) {
#if CHATTY_COM && PRINT_TX_ENABLE
        pr_debug("com%d: tx disable\n", com->num);
#endif
        com->ier.thre = 0;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void check_modem_status(struct com *com)
{
    uint8_t old_msr = com->msr._value;
    com->msr._value = com_in(com, UART_MSR);
    (void) old_msr;

#if CHATTY_COM && PRINT_MODEM_STATUS
    if ((com->msr._value & 0x0F) != (old_msr & 0x0F)) {
        pr_debug("com%d: modem status:%s%s%s%s%s%s%s%s\n", com->num,
            com->msr.dcts ? " dcts" : "",
            com->msr.ddsr ? " ddsr" : "",
            com->msr.teri ? " teri" : "",
            com->msr.ddcd ? " ddcd" : "",
            com->msr.cts  ? " cts"  : "",
            com->msr.dsr  ? " dsr"  : "",
            com->msr.ri   ? " ri"   : "",
            com->msr.dcd  ? " dcd"  : "");
    }
#endif

    // statistics
    if (com->msr._value & UART_MSR_ANY_DELTA) {
        if (com->msr.cts) {
            com->stats.n_cts++;     // clear to send
        }
        if (com->msr.dsr) {
            com->stats.n_dsr++;     // data set ready
        }
        if (com->msr.teri) {
            com->stats.n_ring++;    // trailing-edge ring indicator
        }
        if (com->msr.dcd) {
            com->stats.n_dcd++;     // data carrier detect
        }
    }

    // handle carrier detect
    if (com->tty && com->msr.ddcd) {
        if (com->msr.dcd) {
            // TODO: babe, wake up
        }
        else {
            tty_hangup(com->tty);
        }
    }

    // handle CTS/RTS flow control
    if (com->tty && C_CRTSCTS(com->tty) && com->msr.dcts) {
        if (com->tty->hw_stopped) {
            if (com->msr.cts) {
#if CHATTY_COM
                pr_debug("com%d: RTSCTS: cts=1, xmit start\n", com->num);
#endif
                com->tty->hw_stopped = false;
                tx_enable(com);
            }
        }
        else {
            if (!com->msr.cts) {
#if CHATTY_COM
                pr_debug("com%d: RTSCTS: cts=0, xmit stop\n", com->num);
#endif
                com->tty->hw_stopped = true;
                tx_disable(com);
            }
        }
    }
}

static void check_line_status(struct com *com)
{
    uint8_t old_lsr = com->lsr._value;
    com->lsr._value = com_in(com, UART_LSR);
    (void) old_lsr;

#if CHATTY_COM && PRINT_LINE_STATUS
    if ((com->lsr._value & 0x1E) != (old_lsr & 0x1E)) {
        pr_debug("com%d: line status:%s%s%s%s\n", com->num,
            com->lsr.oe  ? " oe " : "",
            com->lsr.pe  ? " pe " : "",
            com->lsr.fe  ? " fe"  : "",
            com->lsr.brk ? " brk" : "");
    }
#endif

    if (com->lsr.oe) {
        com->stats.n_overrun++;
    }
    if (com->lsr.pe) {
        com->stats.n_parity++;
    }
    if (com->lsr.fe) {
        com->stats.n_framing++;
    }
    if (com->lsr.brk) {
        com->stats.n_break++;
    }
}

static void send_chars(struct com *com)
{
    char c;
    int count;

    // transmit high-priority control char
    if (com->xchar) {
        com_out(com, UART_TX, com->xchar);
        com->xchar = 0;
        com->stats.n_xchar++;
    }

    // no TTY attached, no chars to send or output stopped? disable transmitter
    if (!com->tty || ring_empty(&com->tx_ring) ||
        com->tty->stopped || com->tty->hw_stopped) {
        tx_disable(com);
        // TODO: wake sleeping writer (do we wake when stopped?)
        return;
    }

    // send chars
    count = XMIT_MAX;
    while (count > 0 && ring_pop_front(&com->tx_ring, c, char)) {
        com_out(com, UART_TX, c);
        count--;
    }

    // nothing left to send? disable transmitter
    if (ring_empty(&com->tx_ring)) {
        tx_disable(com);
        // TODO: wake sleeping writer
        return;
    }
}

static void recv_chars(struct com *com)
{
    struct tty *tty = com->tty;

    // was there a timeout?
    if (com->iir.timeout) {
        com->stats.n_timeout++;
    }

    // receive chars while data ready
    int count = RECV_MAX;
    do {
        // buffer full? disable RDA interrupt 'til it can drain
        if (tty && tty->ldisc.recv_room && tty->ldisc.recv_room(tty) == 0) {
            if (com->ier.rda) {
#if CHATTY_COM
                pr_debug("com%d: receiver disabled\n", com->num);
#endif
                com->ier.rda = 0;
                com_out(com, UART_IER, com->ier._value);
            }
            break;
        }

        bool brk = com->lsr.brk;    // break condition
        bool fe = com->lsr.fe;      // framing error
        bool pe = com->lsr.pe;      // parity error
        // TODO: log stats?

        // accept char and put it in the ldisc
        char c = com_in(com, UART_RX);

        if (!tty || !com->open) {
#if CHATTY_COM
            pr_debug("com%d: character '%c' received via interrupt with no TTY attached!", com->num, c);
#endif
            goto next_char;     // drop char if nowhere for it to go!
        }

        // apply termios c_iflag
        if (brk && I_IGNBRK(tty)) {
            goto next_char;     // ignore break
        }
        if (I_INPCK(tty)) {
            if (I_IGNPAR(tty) && (pe || fe) && !brk) {
                goto next_char; // drop byte with parity/framing error
            }
            else if (!I_IGNPAR(tty) && I_PARMRK(tty)) {
                // send prefix to mark parity/framing errors and
                static char s_prefix[] = { 0xFF, 0x00 };
                if (pe || fe) {
                    // send '\377' '\0' as prefix on parity/framing error
                    tty->ldisc.recv(tty, s_prefix, sizeof(s_prefix));
                }
                else if ((unsigned char) c == 0xFF) {
                    // if no char error but char is 0xFF,
                    // send as two bytes 0xFF 0xFF to avoid confusion
                    tty->ldisc.recv(tty, &s_prefix[0], 1);
                }
            }
        }
        if (brk) {
            c = '\0';   // break char sent as '\0'
            // TODO: I_BRKINT, send SIGINT (need signals!!)
        }

        // put char in process's input queue
        tty->ldisc.recv(tty, &c, 1);


next_char:
        // read new line status, continue receiving while data is available
        check_line_status(com);
        if (!com->lsr.dr) {
            break;
        }
        // ...or until we've reached the limit
    } while (--count > 0);
}

static void com_interrupt(struct com *com, struct iregs *regs)
{
    int npass;
    (void) regs;

    com->iir._value = com_in(com, UART_IIR);
    if (com->iir.no_int) {
        return; // nothing to service!
    }

    shadow_regs(com);

#if CONFIG_SERIAL_DEBUG
    if (com->io_port == __mkcomport(CONFIG_SERIAL_DEBUG_COM)) {
        if (com->iir.id == ID_RDA || com->iir.timeout) {
            __enter_debugger_loop(regs, /*from_com=*/true);
        }
        return;
    }
#endif

    npass = 0;
    do {
        check_line_status(com);     // reads LSR

        // handle rx
        if (com->iir.id == ID_RDA || com->iir.timeout || com->lsr.dr) {
            recv_chars(com);
        }

        check_modem_status(com);    // reads MSR

        // handle tx
        if (com->iir.id == ID_THRE || com->lsr.thre) {
            send_chars(com);
        }

        // break-out if we've exceed the max number of passes
        if (++npass > INTR_MAX) {
            break;
        }

        // reread for next iteration
        com->iir._value = com_in(com, UART_IIR);
    } while (com->iir.no_int == 0);
}

#define _do_com_irq(port,regs)      \
do {                                \
    struct com *__c;                \
    __c = get_com(port);            \
    com_interrupt(__c, regs);       \
} while (0);

static void com1_irq(int irq, struct iregs *regs)
{
    assert(irq == IRQ_COM1);

    _do_com_irq(COM1, regs);
    _do_com_irq(COM3, regs);
}

static void com2_irq(int irq, struct iregs *regs)
{
    assert(irq == IRQ_COM2);

    _do_com_irq(COM2, regs);
    _do_com_irq(COM4, regs);
}
