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
 *         File: kernel/serial.c
 *      Created: August 11, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <boot.h>
#include <ohwes.h>
#include <io.h>
#include <device.h>
#include <irq.h>
#include <kernel.h>
#include <console.h>
#include <stdint.h>
#include <errno.h>
#include <queue.h>
#include <tty.h>

//
// TODO: ditch the 'COM' nomenclature?
//  what could we use in favor? serial? ttyS? uart? rs232? rs?
//

#define COM_BUFFER_SIZE     16
#define LOOPBACK_TEST       0
#define DEBUG_SERIAL        1

//
// Physical Serial Ports
//
enum {
    COM1 = 1,
    COM2,
    COM3,
    COM4,
};
static_assert(COM4 == NR_SERIAL, "NR_SERIAL");

//
// COM (Serial) Base IO Ports
//
#define COM1_PORT           0x3F8
#define COM2_PORT           0x2F8
#define COM3_PORT           0x3E8
#define COM4_PORT           0x2E8

//
// COM Port Baud Rates
//
// The integer value of each enum value may be used to program the baud rate
// divisor register.
//
enum baud_rate {
    BAUD_115200 = 1,
    BAUD_57600  = 2,
    BAUD_38400  = 3,
    BAUD_28800  = 4,    // nonstandard
    BAUD_23040  = 5,    // nonstandard
    BAUD_19200  = 6,
    BAUD_14400  = 8,
    BAUD_12800  = 9,    // nonstandard
    BAUD_11520  = 10,   // nonstandard
    BAUD_9600   = 12,
    BAUD_7680   = 15,   // nonstandard
    BAUD_7200   = 16,
    BAUD_6400   = 18,   // nonstandard
    BAUD_5760   = 20,   // nonstandard
    BAUD_4800   = 24,
    BAUD_2400   = 48,
    BAUD_1800   = 64,
    BAUD_1200   = 96,
    BAUD_600    = 192,
    BAUD_300    = 384,
    BAUD_150    = 768,
    BAUD_134_5  = 857,
    BAUD_110    = 1047,
    BAUD_75     = 1536,
    BAUD_50     = 2304,
};

//
// COM Port Register Offsets
//
#define COM_REG_RX          0       // Receive (LCR_DLA=0)
#define COM_REG_TX          0       // Transmit (LCR_DLA=0)
#define COM_REG_IER         1       // Interrupt Enable (LCR_DLA=0)
#define COM_REG_DLL         0       // Baud Rate Divisor LSB (LCR_DLA=1)
#define COM_REG_DLM         1       // Baud Rate Divisor MSB (LCR_DLA=1)
#define COM_REG_IIR         2       // (Read) Interrupt Identification
#define COM_REG_FCR         2       // (Write) FIFO Control
#define COM_REG_LCR         3       // Line Control
#define COM_REG_MCR         4       // Modem Control
#define COM_REG_LSR         5       // Line Status
#define COM_REG_MSR         6       // Modem Status
#define COM_REG_SCR         7       // Scratch Register

//
// Interrupt Enable Register Masks
//
// The value of this register determines under which scenarios to raise an
// interrupt.
//
#define IER_RXREADY         0x01    // Ready to Receive (Timeout if FCR_ENABLE=1)
#define IER_TXREADY         0x02    // Ready to Send
#define IER_LSR             0x04    // Receiver Line Status
#define IER_MSR             0x08    // Modem Status

struct ier {
    union {
        struct {
            uint8_t rx_ready     : 1;
            uint8_t tx_ready     : 1;
            uint8_t line_status  : 1;
            uint8_t modem_status : 1;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct ier) == 1, "sizeof(struct ier)");

//
// Interrupt Identification Register Masks
//
// This is a read-only register that indicates whether an interrupt is pending
// and the interrupt source (priority). It also indicates whether the UART is
// in FIFO mode.
//
#define IIR_PENDING         0x01    // Interrupt Pending (0 = Pending)
#define IIR_PRIORITY        0x0E    // Interrupt Priority (0 = Lowest)
#define IIR_FIFO            0xC0    // FIFOs Enabled

struct iir {
    union {
        struct {
            uint8_t pending       : 1;
            uint8_t priority      : 2;
            uint8_t timeout       : 1;
            uint8_t               : 2;
            uint8_t fifos_enabled : 2;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct iir) == 1, "sizeof(struct iir)");

//
// Interrupt Priority Levels
//
enum interrupt_priority {
    PRIORITY_MODEM,                 // Modem Status (Lowest)
    PRIORITY_TXREADY,               // Ready to Send
    PRIORITY_RXREADY,               // Ready to Receive (Timeout if FCR_ENABLE=1)
    PRIORITY_LINE,                  // Line Status (Highest)
};

//
// FIFO Control Register Masks
//
// This is a write-only register that controls the transmitter and receiver
// FIFOs.
//
#define FCR_ENABLE          0x01    // FIFOs Enabled
#define FCR_RXRESET         0x02    // Clear Receiver FIFO
#define FCR_TXRESET         0x04    // Clear Transmitter FIFO
#define FCR_DMA             0x08    // DMA Mode Select
#define FCR_TRIGGER         0xC0    // Receiver Trigger Select

struct fcr {
    union {
        struct {
            uint8_t enable   : 1;
            uint8_t rx_reset : 1;
            uint8_t tx_reset : 1;
            uint8_t dma      : 1;
            uint8_t          : 2;
            uint8_t depth    : 2;
        };
        uint8_t _value;
    };

};
static_assert(sizeof(struct fcr) == 1, "sizeof(struct fcr)");

//
// Receiver Interrupt Trigger Levels
//
enum fifo_depth {
    FIFODEPTH_1,                    // 1 Byte Received
    FIFODEPTH_4,                    // 4 Bytes Received
    FIFODEPTH_8,                    // 8 Bytes Received
    FIFODEPTH_14,                   // 14 Bytes Received
};

//
// Line Control Register Masks
//
// This register specifies the format of the transmitted and received data. It
// also provides access to the Divisor Line Access Bit, which enables the baud
// rate to be set.
//
#define LCR_WLS             0x03    // Word Length Select
#define LCR_STB             0x04    // Stop Bits
#define LCR_PEN             0x08    // Parity Enable
#define LCR_EPS             0x10    // Even Parity Select
#define LCR_STP             0x20    // Stick Parity
#define LCR_BRK             0x40    // Break
#define LCR_DLA             0x80    // Divisor Line Access

struct lcr {
    union {
        struct {
            uint8_t word_length : 2;
            uint8_t stop_bits   : 1;
            uint8_t parity      : 3;
            uint8_t brk         : 1;
            uint8_t dla         : 1;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct lcr) == 1, "sizeof(struct lcr)");

//
// Data Word Lengths
//
enum word_length_select {
    DATABITS_5,
    DATABITS_6,
    DATABITS_7,
    DATABITS_8,
};

enum stop_bits {
    STOPBITS_1,
    STOPBITS_2, // 1.5 or 2 depending on word length
};

//
// Parity Modes
//
// This is a combined value of LCR bits 3-5.
//
enum parity_select {
    PARITY_NONE     = 0,
    PARITY_ODD      = LCR_PEN,
    PARITY_EVEN     = LCR_PEN | LCR_EPS,
    PARITY_MARK     = LCR_PEN | LCR_STP,
    PARITY_SPACE    = LCR_PEN | LCR_EPS | LCR_STP,
};

//
// Line Status Register Masks
//
// This register provides data transfer status information.
//
#define LSR_DR              0x01    // Data Ready
#define LSR_OE              0x02    // Overrun Error (RX too slow)
#define LSR_PE              0x04    // Parity Error (Incorrect Parity)
#define LSR_FE              0x08    // Framing Error (Invalid Stop Bit)
#define LSR_BI              0x10    // Break Interrupt
#define LSR_THRE            0x20    // Transmitter Holding Register Empty
#define LSR_TEMT            0x40    // Transmitter Empty
#define LSR_FIFO            0x80    // Receiver FIFO Error

struct lsr {
    union {
        struct {
            uint8_t data_ready : 1;
            uint8_t overrun_error : 1;
            uint8_t parity_error : 1;
            uint8_t framing_error : 1;
            uint8_t break_interrupt : 1;
            uint8_t tx_ready : 1;
            uint8_t tx_idle : 1;
            uint8_t fifo_error : 1;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct lsr) == 1, "sizeof(struct lsr)");
//
// Modem Control Register Masks
//
// This register controls the modem (or peripheral device) interface. The
// Auxiliary Output can be used to delineate between multiple serial ports
// sharing the same IRQ line.
//
#define MCR_DTR             0x01    // Data Terminal Ready
#define MCR_RTS             0x02    // Request to Send
#define MCR_OUT             0x0C    // Auxiliary Output
#define MCR_LOOP            0x10    // Loopback Enable

struct mcr {
    union {
        struct {
            uint8_t data_terminal_ready : 1;
            uint8_t request_to_send     : 1;
            uint8_t auxiliary_out       : 2;
            uint8_t loop                : 1;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct mcr) == 1, "sizeof(struct mcr)");

//
// Modem Status Register Masks
//
// This register provdes information about the current state of the control
// lines from the modem (or peripheral device).
//
#define MSR_DCTS            0x01    // Delta Clear to Send
#define MSR_DDSR            0x02    // Delta Data Set Ready
#define MSR_TERI            0x04    // Trailing Edge Ring Indicator
#define MSR_DDCD            0x08    // Delta Data Carrier Detect
#define MSR_CTS             0x10    // Clear to Send
#define MSR_DSR             0x20    // Data Set Ready
#define MSR_RI              0x40    // Ring Indicator
#define MSR_DCD             0x80    // Data Carrier Detect

struct msr {
    union {
        struct {
            uint8_t delta_clear_to_send : 1;
            uint8_t delta_data_set_ready : 1;
            uint8_t trailing_edge_ring_indicator : 1;
            uint8_t delta_carrier_detect : 1;
            uint8_t clear_to_send : 1;
            uint8_t data_set_ready : 1;
            uint8_t ring_indicator : 1;
            uint8_t carrier_detect : 1;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct msr) == 1, "sizeof(struct msr)");

// ----------------------------------------------------------------------------

static uint16_t s_comports[] = {
    COM1, COM2, COM3, COM4,
};
static_assert(countof(s_comports) == NR_SERIAL, "countof(s_comports)");

struct com_port {
    bool open;                  // port is in use
    uint8_t num;                // port number
    uint16_t io_port;           // I/O base port number

    struct ring oq;       // --> to device
    char _obuf[COM_BUFFER_SIZE];

    // register shadows
    struct iir iir;             // interrupt indicator register
    struct ier ier;             // interrupt enable register
    struct fcr fcr;             // fifo control register
    struct lcr lcr;             // line control register
    struct mcr mcr;             // modem control register
    struct lsr lsr;             // line status register
    struct msr msr;             // modem status register
    uint16_t baud_divisor;      // baud rate divisor
};

struct com_port g_com[NR_SERIAL];

static void init_com_port(int port);
static struct com_port * get_com(int port);
static int com_open(struct com_port *com);
static int com_close(struct com_port *com);
static uint8_t com_read(struct com_port *com, uint8_t reg);
static void com_write(struct com_port *com, uint8_t reg, uint8_t data);
static void com_flush(struct com_port *com);
static void com_interrupt(struct com_port *com);

static void serial_interrupt(int irq_num);

static int serial_open(struct tty *);
static int serial_close(struct tty *);
static int serial_write(struct tty *, const char *buf, size_t count);
static int serial_ioctl(struct tty *, unsigned int cmd, unsigned long arg);
static size_t serial_write_room(struct tty *);

struct tty_driver serial_driver = {
    .name = "serial",
    .open = serial_open,
    .close = serial_close,
    .ioctl = serial_ioctl,
    .write = serial_write,
    .write_room = serial_write_room,
};

static int tty_get_com(struct tty *tty, struct com_port **com)
{
    if (!tty || !com) {
        return -EINVAL;
    }

    if (tty->major != TTYS_MAJOR) {
        return -ENXIO;
    }
    if (tty->index < 0 || tty->index >= NR_SERIAL) {
        return -ENXIO;
    }

    *com = get_com(tty->index + 1);  // TODO: consistent indexing
    return 0;
}

static int serial_open(struct tty *tty)
{
    struct com_port *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    return com_open(com);
}

static int serial_close(struct tty *tty)
{
    struct com_port *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    return com_close(com);
}

static int serial_write(struct tty *tty, const char *buf, size_t count)
{
    struct com_port *com;

    int ret = tty_get_com(tty, &com);
    if (ret < 0) {
        return ret;
    }

    int nwritten = 0;
    for (int i = 0; i < count; i++) {
        if (ring_full(&com->oq)) {
            com_flush(com);
        }
        if (!ring_full(&com->oq)) {
            ring_put(&com->oq, buf[i]);
            nwritten++;
        }
    }

    com_flush(com);
    return nwritten;
}

static int serial_ioctl(
    struct tty *tty, unsigned int cmd, unsigned long arg)
{
    return -ENOSYS;
}

static size_t serial_write_room(struct tty *tty)
{
    return -ENOSYS;
}

// ----------------------------------------------------------------------------

void init_serial(void)
{
    for (int i = 0; i < countof(s_comports); i++) {
        init_com_port(s_comports[i]);
    }

    irq_register(IRQ_COM1, serial_interrupt);
    irq_unmask(IRQ_COM1);

    irq_register(IRQ_COM2, serial_interrupt);
    irq_unmask(IRQ_COM2);
}

static void init_com_port(int port)
{
    struct com_port *com = get_com(port);
    zeromem(com, sizeof(struct com_port));

    com->num = port;
    switch (port) {
        case COM1: com->io_port = COM1_PORT; break;
        case COM2: com->io_port = COM2_PORT; break;
        case COM3: com->io_port = COM3_PORT; break;
        case COM4: com->io_port = COM4_PORT; break;
        default: panic("invalid COM com%d", port);
    }
}

static struct com_port * get_com(int port)
{
    panic_assert(port > 0 && port <= NR_SERIAL);
    return &g_com[port - 1];
}

static int com_open(struct com_port *com)
{
    assert(com != NULL);

    // check if port exists
    com->ier._value = com_read(com, COM_REG_IER);
    if (com->ier._value == 0xFF) {
        // 0xFF = port not available
        return -EIO;
    }

    // initialize output buffer
    ring_init(&com->oq, com->_obuf, COM_BUFFER_SIZE);

    // disable all interrupts
    com_write(com, COM_REG_IER, 0);

    // set baud rate
    com->baud_divisor = BAUD_9600;
    com_write(com, COM_REG_LCR, LCR_DLA);
    com_write(com, COM_REG_DLL, com->baud_divisor & 0xFF);
    com_write(com, COM_REG_DLM, com->baud_divisor >> 8);
    com->baud_divisor = com_read(com, COM_REG_DLL);
    com->baud_divisor |= com_read(com, COM_REG_DLM) << 8;

    if (com->baud_divisor == 0 || com->baud_divisor == 0xFF) {
        // invalid/bad COM port
        return -EIO;
    }

    // enable and clear FIFOs, trigger at 1 byte
    com->fcr.enable = 1;
    com->fcr.rx_reset = 1;
    com->fcr.tx_reset = 1;
    com->fcr.depth = FIFODEPTH_1;
    com_write(com, COM_REG_FCR, com->fcr._value);
    com->fcr._value = com_read(com, COM_REG_FCR);

    // set word length, parity, and stop bits
    com->lcr.word_length = DATABITS_8;
    com->lcr.parity = PARITY_NONE;
    com->lcr.stop_bits = STOPBITS_1;
    com_write(com, COM_REG_LCR, com->lcr._value);
    com->lcr._value = com_read(com, COM_REG_LCR);

    // configure modem
    com->mcr.data_terminal_ready = 1;
    com->mcr.request_to_send = 1;
    com->mcr.auxiliary_out = 2;     // 2 = enable IRQ

#if LOOPBACK_TEST   // TODO: does not work for some reason...
    com->mcr.loop = 1;
    com_write(com, COM_REG_MCR, com->mcr._value);
    com_write(com, COM_REG_TX, 0x18);
    int ndelay = 1000;
    do {
        if (!ndelay) {
            kprint("com%d: loop: timeout\n", com->num);
            return -EIO;
        }
        if (com_read(com, COM_REG_RX) != 0x18) {
            continue;
        }
    } while (ndelay--);
    com->mcr.loop = 0;
#endif

    com_write(com, COM_REG_MCR, com->mcr._value);
    com->mcr._value = com_read(com, COM_REG_MCR);

    // enable interrupts
    com->ier.tx_ready = 1;
    com->ier.rx_ready = 1;
    com->ier.line_status = 1;
    com->ier.modem_status = 1;
    com_write(com, COM_REG_IER, com->ier._value);
    com->ier._value = com_read(com, COM_REG_IER);

    // print port info
    kprint("com%d: port=%Xh div=%d fcr=%02Xh lcr=%02Xh mcr=%02Xh ier=%02Xh\n",
        com->num, (int) com->io_port,
        (int) com->baud_divisor, (int) com->fcr._value,
        (int) com->lcr._value, (int) com->mcr._value,
        (int) com->ier._value);

    com->open = true;
    return 0;
}

static int com_close(struct com_port *com)
{
    com->open = false;
    return 0;
}

static uint8_t com_read(struct com_port *com, uint8_t reg)
{
    assert(com != NULL);
    if (reg > COM_REG_SCR) {
        panic("COM register %d invalid", reg);
    }

    return inb(com->io_port + reg);
}

static void com_write(struct com_port *com, uint8_t reg, uint8_t data)
{
    assert(com != NULL);
    if (reg > COM_REG_SCR) {
        panic("COM register %d invalid", reg);
    }

    outb(com->io_port + reg, data);
}

static void com_flush(struct com_port *com)
{
    // while (!com->lsr.tx_ready) {
    //     // spin...
    // }

    while (com->lsr.tx_ready && !ring_empty(&com->oq)) {
        char c = ring_get(&com->oq);
        com_write(com, COM_REG_TX, c);
        com->lsr._value = com_read(com, COM_REG_LSR);
    }
}

static void com_interrupt(struct com_port *com)
{
    char data;

    assert(com != NULL);
    if (!com->open) {
        return;
    }

    do {
        com->iir._value = com_read(com, COM_REG_IIR);
        com->lsr._value = com_read(com, COM_REG_LSR);
        com->msr._value = com_read(com, COM_REG_MSR);

        switch (com->iir.priority) {
            case PRIORITY_RXREADY:
#if DEBUG_SERIAL
                kprint("com%d: rx_ready\n", com->num);
#endif
                assert(com->lsr.data_ready);
                while (com->lsr.data_ready) {
                    data = com_read(com, COM_REG_RX);
                    // TODO: put into line discipline
                    write_console(get_console(com->num), &data, 1);
                    com->lsr._value = com_read(com, COM_REG_LSR);
                }
                break;
            case PRIORITY_TXREADY:
#if DEBUG_SERIAL
                kprint("com%d: tx_ready\n", com->num);
#endif
                assert(com->lsr.tx_ready);
                com_flush(com);
                break;
            case PRIORITY_LINE:
#if DEBUG_SERIAL
                if (com->lsr.overrun_error) {
                    kprint("\e[1;31mcom%d: overrun error\e[0m\n", com->num);
                }
                if (com->lsr.parity_error) {
                    kprint("\e[1;31mcom%d: parity error\e[0m\n", com->num);
                }
                if (com->lsr.framing_error) {
                    kprint("\e[1;31mcom%d: framing error\e[0m\n", com->num);
                }
                if (com->lsr.fifo_error) {
                    kprint("\e[1;31mcom%d: fifo error\e[0m\n", com->num);
                }
                if (com->lsr._value & (LSR_BI | LSR_DR | LSR_TEMT | LSR_THRE)) {
                    kprint("com%d: line status:%s%s%s%s\n", com->num,
                        com->lsr.break_interrupt ? " break" : "",
                        com->lsr.data_ready ? " data_ready" : "",
                        com->lsr.tx_ready ? " tx_ready" : "",
                        com->lsr.tx_idle ? " tx_idle" : "");
                }
#endif
                break;
            case PRIORITY_MODEM:

#if DEBUG_SERIAL
                if (com->msr._value) {
                    kprint("com%d: modem status:%s%s%s%s%s%s%s%s\n", com->num,
                            com->msr.clear_to_send ? " cts" : "",
                            com->msr.data_set_ready ? " dsr" : "",
                            com->msr.ring_indicator ? " ri" : "",
                            com->msr.carrier_detect ? " dcd" : "",
                            com->msr.delta_clear_to_send ? " dcts" : "",
                            com->msr.delta_data_set_ready ? " ddsr" : "",
                            com->msr.trailing_edge_ring_indicator ? " teri" : "",
                            com->msr.delta_carrier_detect ? " ddcd" : "");
                }
#endif
                break;
        }
        com->iir._value = com_read(com, COM_REG_IIR);
    } while (com->iir.pending == 0);
}

static void serial_interrupt(int irq_num)
{
    assert(irq_num == IRQ_COM1 || irq_num == IRQ_COM2);
    struct com_port *com;

    for (int i = 0; i < NR_SERIAL; i++) {
        com = get_com(s_comports[i]);
        com->iir._value = com_read(com, COM_REG_IIR);
        if (com->iir.pending == 0) {     // active low
            com_interrupt(com);
        }
    }
}
