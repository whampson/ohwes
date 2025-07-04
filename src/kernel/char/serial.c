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
#include <i386/interrupt.h>
#include <i386/io.h>
#include <kernel/io.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>
#include <kernel/queue.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

// print debugging info
#define CHATTY_COM          1
#define PRINT_TX_ENABLE     0
#define PRINT_LINE_STATUS   0
#define PRINT_MODEM_STATUS  0
#define PRINT_TIMEOUT       0

// counts of things
#define FIFO_DEPTH          16          // hardware FIFO depth (assumed)
#define RECV_MAX            128         // max chars to receive per interrupt
#define XMIT_MAX            FIFO_DEPTH  // max chars to send per interrupt
#define INTR_MAX            16          // max num passes per interrupt

// check if a COM register returned a bad value
#define ERR_CHK(x)          ((x) == 0 || (x) == 0xFF)

// warn print
#define COM_WARN(...) \
    alert(__VA_ARGS__)

//
// COM port identifiers
//
enum {
    COM1 = 1,
    COM2 = 2,
    COM3 = 3,
    COM4 = 4,
};

//
// COM port state
//
struct com {
    // port info
    int num;                    // COM port number
    uint16_t io_port;           // I/O base port number
    struct tty *tty;            // TTY

    // flags
    bool valid      : 1;        // port exists and is usable
    bool open       : 1;        // port is currently in use
    bool reserved   : 1;        // port exists, but is reserved by another driver

    // buffers
    struct ring tx_ring;        // output queue
    char _txbuf[TTY_BUFFER_SIZE];
    char xchar;                 // high-priority control character

    // register shadows
    struct iir iir;             // interrupt indicator register
    struct ier ier;             // interrupt enable register
    struct lcr lcr;             // line control register
    struct lsr lsr;             // line status register
    struct mcr mcr;             // modem control register
    struct msr msr;             // modem status register
    uint16_t baud_divisor;      // baud rate divisor

    // statistics
    struct serial_stats stats;
};
struct com g_com[NR_SERIAL];

// "serial" prefix refers to TTY functions
// "com" prefix refers to UART functions

static int serial_open(struct tty *);
static int serial_close(struct tty *);
static int serial_ioctl(struct tty *tty, int op, void *arg);
static void serial_flush(struct tty *);
static int serial_write(struct tty *tty, const char *buf, size_t count);
static size_t serial_write_room(struct tty *);
static void serial_unthrottle(struct tty *);
static void serial_throttle(struct tty *);
static void serial_start(struct tty *);
static void serial_stop(struct tty *);

struct tty_driver serial_driver = {
    .name = "ttyS",
    .major = TTY_MAJOR,
    .minor_start = TTYS_MIN,
    .count = NR_SERIAL,
    .open = serial_open,
    .close = serial_close,
    .ioctl = serial_ioctl,
    .flush = serial_flush,
    .write = serial_write,
    .write_room = serial_write_room,
    .throttle = serial_throttle,
    .unthrottle = serial_unthrottle,
    .start = serial_start,
    .stop = serial_stop
};

static void com_interrupt(struct com *com);
static void com1_irq(int irq, struct iregs *regs);
static void com2_irq(int irq, struct iregs *regs);

static inline uint8_t com_in(struct com *com, uint8_t reg);
static inline void com_out(struct com *com, uint8_t reg, uint8_t data);

static void shadow_regs(struct com *com);
static bool set_baud(struct com *com, int baud_divisor);
static bool set_mode(struct com *com,
    enum word_length wls, enum parity parity, enum stop_bits stb);
static void set_fifo(struct com *com, bool enabled, enum recv_trig depth);

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

static uint16_t get_com_port(int num)
{
    switch (num) {
        case 1: return COM1_PORT;
        case 2: return COM2_PORT;
        case 3: return COM3_PORT;
        case 4: return COM4_PORT;
    }
    return 0;   // invalid
}

static struct com * get_com(int num)
{
    static_assert(COM4 - COM1 + 1 == NR_SERIAL, "NR_SERIAL");
    if (num < COM1 || num > COM4) {
        panic("invalid COM number %d", num);
    }
    return &g_com[num - COM1];
}

static int tty_get_com(struct tty *tty, struct com **com)
{
    if (!tty || !com) {
        return -EINVAL;
    }

    if (_DEV_MAJ(tty->device) != TTY_MAJOR) {
        return -ENODEV; // char device is not a TTY
    }

    int index = _DEV_MIN(tty->device);
    if (index < TTYS_MIN || index > TTYS_MAX) {
        return -ENXIO;  // TTY device is not a COM
    }

    int com_index = index - TTYS_MIN + 1;
    *com = get_com(com_index);
    return 0;
}

// ----------------------------------------------------------------------------
//                        Serial Console Interface
// Unlike the TTY, the serial console does not use interrupts, instead relying
// on polling to determine when to transmit and receive characters.

#if SERIAL_CONSOLE

static inline char wait_and_recv(struct com *com)
{
    // TODO: timeout?
    while ((com_in(com, UART_LSR) & UART_LSR_DR) == 0);
    return com_in(com, UART_RX);
}

static inline void wait_and_send(struct com *com, char c)
{
    // TODO: timeout?
    while ((com_in(com, UART_LSR) & UART_LSR_THRE) == 0);
    com_out(com, UART_TX, c);
}

static dev_t serial_console_device(struct console *cons)
{
    return __mkserdev(cons->index);
}

static void serial_console_setup(struct console *cons)
{
    struct com *com;
    uint8_t data;

    com = get_com(cons->index);

#if EARLY_PRINT
    // COM struct might not have been initialized yet...
    // do some partial initialization here
    if (!com->num) {
        com->num = cons->index;
        if (com->num < COM1 || com->num > NR_SERIAL) {
            panic("invalid serial console number '%d", com->num);
        }
    }
    assert(com->num == cons->index);
    if (!com->io_port) {
        com->io_port = get_com_port(com->num);
        if (!com->io_port) {
            panic("invalid serial console number '%d'", com->num);
        }
    }
#endif

#if SERIAL_DEBUGGING
    if (com->io_port == SERIAL_DEBUG_PORT) {
        panic("serial console cannot share COM port with serial debugger!");
    }
#endif

    // check for UART existence
    com_out(com, UART_SCR, 0);
    com_out(com, UART_SCR, 0x55);
    data = com_in(com, UART_SCR);
    if (data != 0x55) {
        panic("unable to open serial console on IO port %Xh, "
            "UART does not exist!", com->io_port);
    }

    // set baud rate
    com_out(com, UART_LCR, 0x80);
    com_out(com, UART_DLM, SERIAL_CONSOLE_BAUD >> 8);
    com_out(com, UART_DLL, SERIAL_CONSOLE_BAUD & 0xFF);

    // configure port params
    com_out(com, UART_LCR, 0x03);  // 8 data bits, no party, 1 stop bit
    com_out(com, UART_MCR, 0x0B);  // DTR RTS OUT2
    com_out(com, UART_IER, 0);     // no interrupts
    com_out(com, UART_FCR, 0xC7);  // use fifo

    // clear pending reads
    (void) com_in(com, UART_LSR);
    (void) com_in(com, UART_MSR);
    (void) com_in(com, UART_IIR);
}

static int serial_console_write(struct console *cons, const char *buf, size_t count)
{
    struct com *com;
    const char *p;
    uint8_t ier;

    // get port info
    com = get_com(cons->index);
    assert(com->num == cons->index);

    // disable interrupts
    ier = com_in(com, UART_IER);
    com_out(com, UART_IER, 0);

    // send chars
    p = buf;
    while (*p != '\0' && (p - buf) < count) {
        if (*p == '\n') {
            wait_and_send(com, '\r');
        }
        wait_and_send(com, *p);
        p++;
    }

    // restore interrupts and return
    com_out(com, UART_IER, ier);
    return (p - buf);
}

static int serial_console_getc(struct console *cons)
{
    struct com *com;
    uint8_t ier;
    char c;

    // get port info
    com = get_com(cons->index);
    assert(com->num == cons->index);

    // disable interrupts
    ier = com_in(com, UART_IER);
    com_out(com, UART_IER, 0);

    // receive the character
    c = wait_and_recv(com);

    // restore interrupt and return
    com_out(com, UART_IER, ier);
    return c;
}

struct console serial_console =
{
    .name = "ttyS",
    .index = SERIAL_CONSOLE_NUM,
    .flags = 0,
    .device = serial_console_device,
    .setup = serial_console_setup,
    .write = serial_console_write,
    .getc = serial_console_getc
};

#endif

// ----------------------------------------------------------------------------

void init_serial(void)
{
    struct com *com;

    if (tty_register_driver(&serial_driver)) {
        panic("unable to register serial driver!");
    }

    for (int i = COM1; i <= NR_SERIAL; i++) {
        // locate and init com struct
        com = get_com(i);
        com->num = i;
        com->io_port = get_com_port(i);
        assert(com->io_port);

        // TODO: I kind of want to remove this and just explicitly check for
        // serial debug port and skip...
        if (reserve_io_range(com->io_port, 8, "serial") < 0) {
            // serial port is reserved by another driver (e.g. debug interface)
            com->reserved = true;
            kprint("com%d: I/O port %Xh reserved, not usable as TTY device\n",
                com->num, com->io_port);
            continue;
        }

        // collect initial register state
        shadow_regs(com);
        if (com->ier._value == 0xFF) {
            continue;
        }

        // try storing a value in scratch reg to determine port existence
        com_out(com, UART_SCR, 0);
        com_out(com, UART_SCR, 0x55);
        if (com_in(com, UART_SCR) != 0x55) {
            kprint("com%d: error: probe failed\n", com->num);
            continue;
        }

        com->valid = true;
        kprint("com%d: detected on port %Xh\n", com->num, com->io_port);
    }

#if SERIAL_CONSOLE
    register_console(&serial_console);
#endif

    irq_register(IRQ_COM1, com1_irq);
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

    ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    if (com->reserved) {
        return -EBUSY;  // port reserved by another driver (e.g. serial debug interface)
    }

    if (com->open) {
        assert(com->tty);
        return -EBUSY;  // COM already open
    }

    if (!com->valid) {
        return -EIO;    // port does not exist
    }

    cli_save(flags);

    // initialize ring buffer
    ring_init(&com->tx_ring, com->_txbuf, sizeof(com->_txbuf));
    com->xchar = 0;

    // disable all interrupts
    com_out(com, UART_IER, 0);

    // set default baud rate 9600
    if (!set_baud(com, BAUD_9600)) {
        ret = -EIO; goto done;
    }

    // set default mode (8N1; 8 bits, no parity, 1 stop bit)
    if (!set_mode(com, WLS_8, PARITY_NONE, STB_1)) {
        ret = -EIO; goto done;
    }

    // enable FIFOs and set default trigger level (14 bytes)
    set_fifo(com, true, RCVR_TRIG_14);

    // set modem control
    com->mcr._value = 0;
    com->mcr.dtr = 1;   // data terminal ready
    com->mcr.rts = 1;   // request to send
    com->mcr.out2 = 1;  // like carrier detect, I think...
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

    // collect final register state
    shadow_regs(com);
    if (ERR_CHK(com->ier._value) || ERR_CHK(com->mcr._value)) {
        ret = -EIO; goto done;
    }

    com->tty = tty;
    com->open = true;

#if CHATTY_COM
    kprint("com%d: opened, port=%Xh div=%d lcr=%02Xh mcr=%02Xh iir=%02Xh ier=%02Xh\n",
        com->num, (int) com->io_port,
        (int) com->baud_divisor,
        (int) com->lcr._value, (int) com->mcr._value,
        (int) com->iir._value, (int) com->ier._value);
#endif

done:
    restore_flags(flags);
    return ret;
}

static int serial_close(struct tty *tty)
{
    struct com *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    // TODO: flush, etc.

    com->tty = NULL;
    com->open = false;
    return 0;
}

static int serial_ioctl(struct tty *tty, int op, void *arg)
{
    int ret;
    struct com *com;

    if (!tty) {
        return -EINVAL;
    }

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

static int serial_write(struct tty *tty, const char *buf, size_t count)
{
    uint32_t flags;
    struct com *com;
    size_t room;
    const char *ptr;
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

    // disable interrupts while poking TX buffer
    cli_save(flags);

    // calculate remaining buffer space
    room = ring_length(&com->tx_ring) - ring_count(&com->tx_ring);
    if (count > room) {
        count = room;
    }

    // fill the TX buffer
    ptr = buf;
    while (count > 0) {
        ring_put(&com->tx_ring, *ptr);
        ptr++; count--;
    }

#if CHATTY_COM
    if (ring_full(&com->tx_ring)) {
        COM_WARN("com%d: write buffer full!\n", com->num);
    }
#endif

    if (!ring_empty(&com->tx_ring) && !tty->stopped && !tty->hw_stopped) {
        tx_enable(com);
    }

    // enable interrupts and return
    restore_flags(flags);
    return ptr - buf;
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
    room = ring_length(&com->tx_ring) - ring_count(&com->tx_ring);
    restore_flags(flags);

    return room;
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
    if (I_IXOFF(tty)) {
#if CHATTY_COM
        COM_WARN("com%d: IXOFF: tx START_CHAR\n", com->num);
#endif
        com->xchar = START_CHAR(tty);
        tx_enable(com);
    }
    if (C_CRTSCTS(tty)) {
#if CHATTY_COM
        COM_WARN("com%d: rts=1\n", com->num);
#endif
        com->mcr.rts = 1;
    }
    com_out(com, UART_MCR, com->mcr._value);
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
    if (I_IXOFF(tty)) {
#if CHATTY_COM
        COM_WARN("com%d: IXOFF: tx STOP_CHAR\n", com->num);
#endif
        com->xchar = STOP_CHAR(tty);
        tx_enable(com);
    }
    if (C_CRTSCTS(tty)) {
#if CHATTY_COM
        COM_WARN("com%d: rts=0\n", com->num);
#endif
        com->mcr.rts = 0;
    }
    com_out(com, UART_MCR, com->mcr._value);
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
    COM_WARN("com%d: rx XON, starting...\n", com->num);
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
    COM_WARN("com%d: rx XOFF, stopping...\n", com->num);
#endif

    cli_save(flags);
    tx_disable(com);
    restore_flags(flags);
}

// ----------------------------------------------------------------------------
//                          COM Port Interface

static inline uint8_t com_in(struct com *com, uint8_t reg)
{
    assert(com);
    assert(reg <= UART_SCR);

    return inb(com->io_port + reg);
}

static inline void com_out(struct com *com, uint8_t reg, uint8_t data)
{
    assert(com);
    assert(reg <= UART_SCR);

    outb(com->io_port + reg, data);
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
        kprint("com%d: error: unable to set baud rate (div=%Xh)\n",
                com->num, baud_divisor);
        return false;
    }

    // otherwise, we're golden, clear the DLAB bit and exit
    com_out(com, UART_LCR, lcr & ~UART_LCR_DLAB);
    return true;
}

static bool set_mode(struct com *com,
    enum word_length wls, enum parity parity, enum stop_bits stb)
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
        kprint("com%d: error: unable to set line control (lcr=%Xh, lcr_rdbk=%Xh)\n",
            com->num, lcr._value, lcr_rdbk);
        return false;
    }

    return true;
}

static void set_fifo(struct com *com, bool enabled, enum recv_trig depth)
{
    struct fcr fcr;

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

    return copy_to_user(user_info, &result, sizeof(int));
}

static int set_modem_info(struct com *com, const int *user_info)
{
    int info;
    uint32_t flags;

    if (!copy_from_user(&info, user_info, sizeof(int))) {
        return -EFAULT;
    }

    com->mcr._value &= ~(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT1 | UART_MCR_OUT2);
    com->mcr._value |= ((info & TIOCM_DTR) ? UART_MCR_DTR : 0)
                    |  ((info & TIOCM_RTS) ? UART_MCR_RTS : 0)
                    |  ((info & TIOCM_OUT1)? UART_MCR_OUT1: 0)
                    |  ((info & TIOCM_OUT2)? UART_MCR_OUT2: 0);

    cli_save(flags);
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

    return copy_to_user(user_stats, &stats, sizeof(struct serial_stats));
}

static void tx_enable(struct com *com)
{
    if (!com->ier.thre) {
#if CHATTY_COM && PRINT_TX_ENABLE
        COM_WARN("com%d: tx enable\n", com->num);
#endif
        com->ier.thre = 1;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void tx_disable(struct com *com)
{
    if (com->ier.thre) {
#if CHATTY_COM && PRINT_TX_ENABLE
        COM_WARN("com%d: tx disable\n", com->num);
#endif
        com->ier.thre = 0;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void check_modem_status(struct com *com)
{
    com->msr._value = com_in(com, UART_MSR);

#if CHATTY_COM && PRINT_MODEM_STATUS
    if (com->msr._value & 0x0F) {
        COM_WARN("com%d: modem status:%s%s%s%s%s%s%s%s\n", com->num,
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

    // handle CTS/RTS flow control
    if (C_CRTSCTS(com->tty)) {
        if (com->tty->hw_stopped) {
            if (com->msr.cts) {
#if CHATTY_COM
                COM_WARN("com%d: CTS tx start\n", com->num);
#endif
                com->tty->hw_stopped = false;
                tx_disable(com);
            }
        }
        else {
            if (!com->msr.cts) {
#if CHATTY_COM
                COM_WARN("com%d: CTS tx stop\n", com->num);
#endif
                com->tty->hw_stopped = true;
                tx_enable(com);
            }
        }
    }
}

static void check_line_status(struct com *com)
{
    com->lsr._value = com_in(com, UART_LSR);

#if CHATTY_COM && PRINT_LINE_STATUS
    if (com->lsr._value & 0x1E) {
        COM_WARN("com%d: %s%s%s%s\n", com->num,
            com->lsr.oe  ? " overrun error" : "",
            com->lsr.pe  ? " parity error"  : "",
            com->lsr.fe  ? " framing error" : "",
            com->lsr.brk ? " break"   : "");
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
        com->stats.n_tx++;
    }

    // no chars to send or output stopped? disable transmitter
    if (ring_empty(&com->tx_ring) || com->tty->stopped || com->tty->hw_stopped) {
        tx_disable(com);
        return;
    }

    // send chars
    count = XMIT_MAX;
    do {
        c = ring_get(&com->tx_ring);
        com_out(com, UART_TX, c);
        com->stats.n_tx++;
        if (ring_empty(&com->tx_ring)) {
            break;
        }
    } while (--count > 0);

    // nothing left to send? disable transmitter
    if (ring_empty(&com->tx_ring)) {
        tx_disable(com);
        return;
    }
}

static void recv_chars(struct com *com)
{
    char c;
    struct tty *tty = com->tty;
    struct tty_ldisc *ldisc = tty->ldisc;
    int count;

    // was there a timeout?
    if (com->iir.timeout) {
        com->stats.n_timeout++;
#if CHATTY_COM && PRINT_TIMEOUT
        COM_WARN("com%d: timeout!\n", com->num);
#endif
    }

    // receive chars while data ready
    count = RECV_MAX;
    do {
        // accept char and put it in the ldisc
        c = com_in(com, UART_RX);
        ldisc->recv(tty, &c, 1);
        com->stats.n_rx++;

        // read new line status, continue receiving while data is available
        check_line_status(com);
        if (!com->lsr.dr) {
            break;
        }
        // ...or until we've reached the limit
    } while (--count > 0);

#if CHATTY_COM
    if (!count) {
        COM_WARN("com%d: receive max reached!\n", com->num);
    }
#endif
}

static void com_interrupt(struct com *com)
{
    int npass;

    com->iir._value = com_in(com, UART_IIR);
    if (com->iir.no_int) {
        return; // nothing to service!
    }

    // shadow regs
    com->ier._value = com_in(com, UART_IER);
    com->lcr._value = com_in(com, UART_LCR);
    com->mcr._value = com_in(com, UART_MCR);

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
        if (++npass <= INTR_MAX) {
            break;
        }

        // reread for next iteration
        com->iir._value = com_in(com, UART_IIR);
    } while (com->iir.no_int == 0);

#if CHATTY_COM
    if (npass == INTR_MAX) {
        COM_WARN("com%d: max interrupt passes reached!\n", com->num);
    }
#endif
}

#define _do_com_irq(port)   \
do {                        \
    struct com *__c;        \
    __c = get_com(port);    \
    if (__c->open) {        \
        com_interrupt(__c); \
    }                       \
} while (0);

static void com1_irq(int irq, struct iregs *regs)
{
    assert(irq == IRQ_COM1);

    _do_com_irq(COM1);
    _do_com_irq(COM3);
}

static void com2_irq(int irq, struct iregs *regs)
{
    assert(irq == IRQ_COM2);

    _do_com_irq(COM2);
    _do_com_irq(COM4);
}
