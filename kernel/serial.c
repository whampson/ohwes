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
#include <kernel.h>
#include <stdint.h>

//
// Physical Serial Ports
//
enum com_port {
    COM1 = 1,
    COM2,
    COM3,
    COM4,
    COM5,
    COM6,
    COM7,
    COM8,
};

//
// COM (Serial) Base IO Ports
//
#define COM1_PORT           0x3F8
#define COM2_PORT           0x2F8
#define COM3_PORT           0x3E8
#define COM4_PORT           0x2E8
#define COM5_PORT           0x5F8
#define COM6_PORT           0x4F8
#define COM7_PORT           0x5E8
#define COM8_PORT           0x4E8

//
// COM Port Register Offsets
//
#define COM_REG_RXTX        0       // Receive/Transmit (LCR_DLA=0)
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

//
// Interrupt Identification Register Masks
//
// This is a read-only register that indicates whether an interrupt is pending
// and the interrupt source (priority). It also indicates whether the UART is
// in FIFO mode.
//
#define IIR_PENDING         0x01    // Interrupt Pending (0 = Pending)
#define IIR_STATE           0x0E    // Interrupt Priority (0 = Lowest)
#define IIR_FIFO            0xC0    // FIFOs Enabled

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

//
// Receiver Interrupt Trigger Levels
//
enum trigger_level {
    TRIGGER_1_BYTE,                 // 1 Byte Received
    TRIGGER_4_BYTES,                // 4 Bytes Received
    TRIGGER_8_BYTES,                // 8 Bytes Received
    TRIGGER_14_BYTES,               // 14 Bytes Received
};

//
// Line Control Register Masks
//
// This register specifies the format of the transmitted and received data. It
// also provides access to the Divisor Line Access Bit, which enables the baud
// rate to be set.
//
#define LCR_WLS             0x03    // Word Length Select
#define LCR_STB             0x04    // Stop Bit
#define LCR_PEN             0x08    // Parity Enable
#define LCR_EPS             0x10    // Even Parity Select
#define LCR_STP             0x20    // Stick Parity
#define LCR_BRK             0x40    // Break
#define LCR_DLA             0x80    // Divisor Line Access

//
// Data Word Lengths
//
enum word_length_select {
    WLS_5_BITS,
    WLS_6_BITS,
    WLS_7_BITS,
    WLS_8_BITS,
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

// ----------------------------------------------------------------------------

struct serial_info {
    uint8_t com_ports;      // bitmap, LSB = COM1
};

struct serial_info _serial;
struct serial_info *g_serial = &_serial;

static uint16_t get_io_port(enum com_port port)
{
    uint16_t io_port = 0;
    switch (port) {
        case COM1: io_port = COM1_PORT; break;
        case COM2: io_port = COM2_PORT; break;
        case COM3: io_port = COM3_PORT; break;
        case COM4: io_port = COM4_PORT; break;
        case COM5: io_port = COM5_PORT; break;
        case COM6: io_port = COM6_PORT; break;
        case COM7: io_port = COM7_PORT; break;
        case COM8: io_port = COM8_PORT; break;
        default: panic("invalid COM port (COM%d)", port);
    }

    return io_port;
}

static uint8_t com_read(enum com_port port, uint8_t reg)
{
    if (reg > COM_REG_SCR) {
        panic("invalid COM register (%d)", reg);
    }

    return inb(get_io_port(port) + reg);
}

static void com_write(enum com_port port, uint8_t reg, uint8_t data)
{
    if (reg > COM_REG_SCR) {
        panic("invalid COM register (%d)", reg);
    }

    outb(get_io_port(port) + reg, data);
}

void init_com_port(enum com_port port)
{
    // disable all interrupts
    com_write(port, COM_REG_IER, 0);

    // set baud rate (9600)
    com_write(port, COM_REG_LCR, LCR_DLA);
    com_write(port, COM_REG_DLL, 0x03);
    com_write(port, COM_REG_DLM, 0x00);

    // 8-N-1
    com_write(port, COM_REG_LCR, 0x03);

    // enable and clear FIFOs, trigger at 1 byte
    com_write(port, COM_REG_FCR, 0xC7);

    // put the UART into loopback mode
    com_read(port, COM_REG_MCR);
    com_write(port, COM_REG_MCR, MCR_LOOP);

    // test UART
    //      TODO: does not seem to work on bochs!!
    com_write(port, COM_REG_RXTX, 0xA5);
    if (com_read(port, COM_REG_RXTX) != 0xA5) {
        return; // no COM port here!
    }

    boot_kprint("COM%d at %Xh\n", port, get_io_port(port));
    g_serial->com_ports |= (1 << (port - 1));

    // disable loopback mode
    com_write(port, COM_REG_MCR, 0x0F);

    // write a byte to the COM port
    com_write(port, COM_REG_RXTX, 'A');
}

void init_serial(void)
{
    zeromem(g_serial, sizeof(struct serial_info));

    uint16_t com_ports[] = {
        COM1, COM2, COM3, COM4,
        COM5, COM6, COM7, COM8,
    };

    for (int i = 0; i < countof(com_ports); i++) {
        init_com_port(com_ports[i]);
    }
}
