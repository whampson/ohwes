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
 *         File: kernel/chdev/serial.c
 *      Created: December 24, 2024
 *       Author: Wes Hampson
 *
 * 16550 UART driver.
 * =============================================================================
 */

#include <assert.h>
#include <errno.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>
#include <kernel/queue.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

// print debugging info
#define CHATTY_COM          1

// TTY device minor numbers
#define SERIAL_MIN          (NR_CONSOLE+1)
#define SERIAL_MAX          (SERIAL_MIN+NR_SERIAL)

// enable for slower inb/outb operations
#define SLOW_IO             1

// character counts
#define FIFO_DEPTH          16      // hardware FIFO depth
#define RECV_MAX            128     // max chars to receive per interrupt

// check if a COM register returned a bad value
#define err_chk(x)        ((x) == 0 || (x) == 0xFF)

//
// COM port state
//
struct com {
    // port info
    int num;                    // COM port number
    uint16_t io_port;           // I/O base port number
    struct tty *tty;            // TTY

    // flags
    bool valid  : 1;            // port exists and is usable
    bool open   : 1;            // port is currently in use

    // buffers
    struct ring tx_ring;        // output queue
    char _txbuf[FIFO_DEPTH*2];

    // register shadows
    struct iir iir;             // interrupt indicator register
    struct ier ier;             // interrupt enable register
    struct fcr fcr;             // fifo control register
    struct lcr lcr;             // line control register
    struct lsr lsr;             // line status register
    struct mcr mcr;             // modem control register
    struct msr msr;             // modem status register
    uint16_t baud_divisor;      // baud rate divisor

    // statistics
    uint32_t n_overrun;         // overrun error count
    uint32_t n_parity;          // parity error count
    uint32_t n_framing;         // framing error count
    uint32_t n_timeout;         // timeout error count
    uint32_t n_break;           // break interrupt count
};
struct com _com_ports[NR_SERIAL];

// "serial" prefix refers to TTY functions
// "com" prefix refers to UART functions

static void serial_interrupt(int irq_num);
static void com_interrupt(struct com *com);

static uint8_t com_in(struct com *com, uint8_t reg);
static void com_out(struct com *com, uint8_t reg, uint8_t data);

static void shadow(struct com *com);
static bool set_baud(struct com *com, enum baud_rate baud);
static bool set_mode(struct com *com,
    enum word_length wls, enum parity parity, enum stop_bits stb);
static bool set_fifo(struct com *com,
    bool enabled, enum receiver_trigger depth);

static void tx_enable(struct com *com);
static void tx_disable(struct com *com);

static void line_status(struct com *com);
static void modem_status(struct com *com);

static void send_chars(struct com *com);
static void recv_chars(struct com *com);

static struct com * get_com(int num)
{
    if (num <= 0 || num > NR_SERIAL) {
        panic("invalid COM number %d", num);
    }
    return &_com_ports[num - 1];
}

static int tty_get_com(struct tty *tty, struct com **com)
{
    if (!tty || !com) {
        return -EINVAL;
    }

    if (_DEV_MAJ(tty->device) != TTY_MAJOR) {
        return -ENODEV; // not a TTY device
    }

    int index = _DEV_MIN(tty->device);
    if (index < SERIAL_MIN || index > SERIAL_MAX) {
        return -ENXIO;  // TTY device is not a COM
    }

    int com_index = index - SERIAL_MIN + 1;
    *com = get_com(com_index);
    return 0;
}

static int serial_open(struct tty *tty)
{
    uint32_t flags;
    struct com *com;
    int ret;

    ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
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

    // disable all interrupts
    com_out(com, UART_IER, 0);

    // set default baud rate 9600
    if (!set_baud(com, BAUD_9600)) {
        ret = -EIO; goto done;
    }

    // set default mode 8N1 (8 bits, no parity, 1 stop bit)
    if (!set_mode(com, WLS_8, PARITY_NONE, STB_1)) {
        ret = -EIO; goto done;
    }

    // enable FIFOs and set default trigger level (1 byte)
    if (!set_fifo(com, true, RCVR_TRIG_1)) {
        ret = -EIO; goto done;
    }

    // set modem control
    com->mcr._value = 0;
    com->mcr.dtr = 1;   // data terminal ready
    com->mcr.rts = 1;   // request to send
    com->mcr.out2 = 1;  // like setting data carrier detect, I think...
    com_out(com, UART_MCR, com->mcr._value);

    // enable interrupts
    com->ier._value = 0;
    com->ier.rda = 1;   // interrupt when data ready to read
    // com->ier.thre = 1;  // interrupt when ready to send next char
    com->ier.rls = 1;   // interrupt when line status changes
    com->ier.ms = 1;    // interrupt when modem status changes
    com_out(com, UART_IER, com->ier._value);

    // collect final register state
    shadow(com);
    if (err_chk(com->ier._value) || err_chk(com->mcr._value)) {
        ret = -EIO; goto done;
    }

#if CHATTY_COM
    kprint("com%d: opened, port=%Xh div=%d lcr=%02Xh mcr=%02Xh fcr=%02Xh ier=%02Xh\n",
        com->num, (int) com->io_port,
        (int) com->baud_divisor,
        (int) com->lcr._value, (int) com->mcr._value,
        (int) com->fcr._value, (int) com->ier._value);
#endif

    tty->driver_data = com;
    com->tty = tty;
    com->open = true;

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

    tty->driver_data = NULL;
    com->tty = NULL;
    com->open = false;
    return 0;
}

static int serial_ioctl(struct tty *tty, unsigned int cmd, unsigned long arg)
{
    // TODO
    return -ENOSYS;
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
    if (!ring_empty(&com->tx_ring)) {
        tx_enable(com);
    }
    restore_flags(flags);
}

static int serial_write(struct tty *tty, const char *buf, size_t count)
{
    uint32_t flags;
    struct com *com;
    size_t room;
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
    ret = 0;
    while (count > 0) {
        ring_put(&com->tx_ring, *buf);
        buf++; count--; ret++;
    }

    // enable transmitter if there are chars to transmit
    if (!ring_empty(&com->tx_ring)) {
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
    room = ring_length(&com->tx_ring) - ring_count(&com->tx_ring);
    restore_flags(flags);

    return room;
}


struct tty_driver serial_driver = {
    .name = "ttyS",
    .major = TTY_MAJOR,
    .minor_start = SERIAL_MIN,
    .count = NR_SERIAL,
    .open = serial_open,
    .close = serial_close,
    .ioctl = serial_ioctl,
    .flush = serial_flush,
    .write = serial_write,
    .write_room = serial_write_room,
    // .throttle = serial_throttle,
    // .unthrottle = serial_unthrottle,
    // .start = serial_start,
    // .stop = serial_stop
};

void init_serial(void)
{
    struct com *com;

    if (tty_register_driver(&serial_driver)) {
        panic("unable to register serial driver!");
    }

    for (int i = 1; i <= NR_SERIAL; i++) {
        // locate and init com struct
        com = get_com(i);
        zeromem(com, sizeof(struct com));
        com->num = i;
        switch (i) {
            case 1: com->io_port = COM1_PORT; break;
            case 2: com->io_port = COM2_PORT; break;
            case 3: com->io_port = COM3_PORT; break;
            case 4: com->io_port = COM4_PORT; break;
            default: panic("assign port for COM%d!", i);
        }

        // collect initial register state
        shadow(com);
        if (com->ier._value == 0xFF) {
            continue;
        }

        // sanity check: try storing a value in scratch reg
        com_out(com, UART_SCR, 0);
        com_out(com, UART_SCR, 0x55);
        if (com_in(com, UART_SCR) != 0x55) {
            kprint("com%d: error: probe failed\n", com->num);
            continue;
        }

        com->valid = true;
        kprint("com%d: detected on port %Xh\n", com->num, com->io_port);
    }

    irq_register(IRQ_COM1, serial_interrupt);
    irq_unmask(IRQ_COM1);

    irq_register(IRQ_COM2, serial_interrupt);
    irq_unmask(IRQ_COM2);
}

static uint8_t com_in(struct com *com, uint8_t reg)
{
    assert(com);

    uint8_t data;
    if (reg > UART_SCR) {
        panic("invalid COM register %d", reg);
    }

#if SLOW_IO
    data = inb_delay(com->io_port + reg);
#else
    data = inb(com->io_port + reg);
#endif

    return data;
}

static void com_out(struct com *com, uint8_t reg, uint8_t data)
{
    assert(com);

    if (reg > UART_SCR) {
        panic("invalid COM register %d", reg);
    }

#if SLOW_IO
    outb_delay(com->io_port + reg, data);
#else
    outb(com->io_port + reg, data);
#endif
}

static void shadow(struct com *com)
{
    com->iir._value = com_in(com, UART_IIR);
    com->ier._value = com_in(com, UART_IER);
    com->fcr._value = com_in(com, UART_FCR);
    com->lcr._value = com_in(com, UART_LCR);
    com->lsr._value = com_in(com, UART_LSR);
    com->mcr._value = com_in(com, UART_MCR);
    com->msr._value = com_in(com, UART_MSR);
}

static bool set_baud(struct com *com, enum baud_rate baud)
{
    uint8_t div_lo, div_hi;
    uint8_t lcr;

    // calculate divisor
    div_lo = baud & 0xFF;
    div_hi = (baud >> 8) & 0xFF;

    // set DLAB=1 so we can access the divisor regs
    lcr = com_in(com, UART_LCR);
    com_out(com, UART_LCR, lcr | UART_LCR_DLAB);

    // set the divisor and readback
    com_out(com, UART_DLL, div_lo);
    com_out(com, UART_DLM, div_hi);
    com->baud_divisor = com_in(com, UART_DLL);
    com->baud_divisor |= com_in(com, UART_DLM) << 8;

    // if readback failed, we might have a bad COM port
    if (err_chk(com->baud_divisor) ) {
        kprint("com%d: error: unable to set baud rate (div=%Xh)\n", com->num, baud);
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
    if (err_chk(lcr_rdbk) || lcr_rdbk != lcr._value) {
        kprint("com%d: error: unable to set line control (lcr=%Xh, lcr_rdbk=%Xh)\n",
            com->num, lcr._value, lcr_rdbk);
        return false;
    }

    return true;
}

static bool set_fifo(struct com *com,
    bool enabled, enum receiver_trigger depth)
{
    struct fcr fcr;
    uint8_t fcr_rdbk;

    // program FIFO control register
    fcr._value = 0;
    fcr.enable = enabled;
    if (enabled) {
        fcr.rx_reset = 1;
        fcr.tx_reset = 1;
        fcr.trig = depth;
    }
    com_out(com, UART_FCR, fcr._value);

    // readback for sanity
    fcr_rdbk = com_in(com, UART_FCR);
    if (err_chk(fcr_rdbk)) {
        kprint("com%d: error: unable to set FIFO mode (fcr=%Xh, fcr_rdbk=%Xh)\n",
            com->num, fcr._value, fcr_rdbk);
        return false;
    }

    return true;
}

static void tx_enable(struct com *com)
{
    assert(com);
    if (!com->ier.thre) {
        com->ier.thre = 1;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void tx_disable(struct com *com)
{
    assert(com);
    if (com->ier.thre) {
        com->ier.thre = 0;
        com_out(com, UART_IER, com->ier._value);
    }
}

static void modem_status(struct com *com)
{
#if CHATTY_COM
    if (com->msr._value) {
        kprint("com%d: modem status:%s%s%s%s%s%s%s%s\n", com->num,
            com->msr.cts  ? " cts"  : "",
            com->msr.dsr  ? " dsr"  : "",
            com->msr.dcd  ? " dcd"  : "",
            com->msr.ri   ? " ri"   : "",
            com->msr.dcts ? " dcts" : "",
            com->msr.ddsr ? " ddsr" : "",
            com->msr.ddcd ? " ddcd" : "",
            com->msr.teri ? " teri" : "");
    }
#endif
}

static void line_status(struct com *com)
{
    if (com->lsr.oe) {
        com->n_overrun++;
    }
    if (com->lsr.pe) {
        com->n_parity++;
    }
    if (com->lsr.fe) {
        com->n_framing++;
    }
    if (com->lsr.brk) {
        com->n_break++;
    }

#if CHATTY_COM
    if (com->lsr._value & 0x1E) {
        kprint("com%d: line status:%s%s%s%s\n",
            com->lsr.oe  ? " overrun" : "",
            com->lsr.pe  ? " parity"  : "",
            com->lsr.fe  ? " framing" : "",
            com->lsr.brk ? " break"   : "");
    }
#endif
}

static void send_chars(struct com *com)
{
    int count;

    if (ring_empty(&com->tx_ring)) {
        tx_disable(com);
        return;
    }

    count = FIFO_DEPTH;
    while (count > 0) {
        com_out(com, UART_TX, ring_get(&com->tx_ring));
        if (ring_empty(&com->tx_ring)) {
            break;
        }
    }

    if (ring_empty(&com->tx_ring)) {
        tx_disable(com);
    }
}

static void recv_chars(struct com *com)
{
    char c;
    struct tty *tty = com->tty;
    struct tty_ldisc *ldisc = tty->ldisc;
    size_t count;

    // was there a timeout?
    if (com->iir.timeout) {
        com->n_timeout++;
#if CHATTY_CO
        kprint("com%d: timeout!\n", com->num);
#endif
    }

    // receive chars
    count = RECV_MAX;
    do {
        // read-in character
        c = com_in(com, UART_RX);
        ldisc->recv(tty, &c, 1);

        // check line status
        line_status(com);
        com->lsr._value = com_in(com, UART_LSR);
    } while (com->lsr.dr && --count);

    // check line status one last time
    line_status(com);
}

static void com_interrupt(struct com *com)
{
    switch (com->iir.id) {
        case ID_RLS:
            line_status(com);
            break;
        case ID_RDA:
            recv_chars(com);
            break;
        case ID_THRE:
            send_chars(com);
            break;
        case ID_MS:
            modem_status(com);
            break;
    }
}

static void serial_interrupt(int irq_num)
{
    // common interrupt handler for all COM ports

    assert(irq_num == IRQ_COM1 || irq_num == IRQ_COM2);
    struct com *com;

    for (int i = 1; i <= NR_SERIAL; i++) {
        com = get_com(i);
        shadow(com);
        if (!com->iir.no_int) {
            com_interrupt(com);
        }
    }
}
