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
 *         File: include/kernel/serial.h
 *      Created: December 23, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __SERIAL_H
#define __SERIAL_H

#include <assert.h>
#include <stdint.h>
#include <kernel/config.h>

//
// UART Base IO Port Numbers
//
#define COM1_PORT           0x3F8
#define COM2_PORT           0x2F8
#define COM3_PORT           0x3E8
#define COM4_PORT           0x2E8
// could go up to 8... but 4 will do!

//
// UART IO Port Registers
//
#define UART_RX             0       // Receive Buffer Register (Read-Only) (DLAB=0)
#define UART_TX             0       // Transmit Holding Register (Write-Only) (DLAB=0)
#define UART_DLL            0       // Baud Rate Divisor LSB (DLAB=1)
#define UART_DLM            1       // Baud Rate Divisor MSB (DLAB=1)
#define UART_IER            1       // Interrupt Enable Register
#define UART_IIR            2       // Interrupt Identification Register (Read-Only)
#define UART_FCR            2       // FIFO Control Register (Write-Only)
#define UART_LCR            3       // Line Control Register
#define UART_MCR            4       // Modem Control Register
#define UART_LSR            5       // Line Status Register
#define UART_MSR            6       // Modem Status Register
#define UART_SCR            7       // Scratch Register

//
// Interrupt Enable Register
//
// The value of this register determines under which scenarios to raise an
// interrupt.
//
#define UART_IER_RDA        0x01    // Enable 'Received Data Available' Interrupt
#define UART_IER_THRE       0x02    // Enable 'Transmitter Holding Register Empty' Interrupt
#define UART_IER_RLS        0x04    // Enable 'Receiver Line Status' Interrupt
#define UART_IER_MS         0x08    // Enable 'Modem Status' Interrupt

//
// Interrupt Identification Register (Read-Only)
//
// This is a read-only register that indicates whether an interrupt is pending
// and the interrupt source (priority). It also indicates whether the UART is
// in FIFO mode.
//
#define UART_IIR_NO_INT     0x01    // Interrupt Pending (0 = Pending)
#define UART_IIR_ID         0x06    // Interrupt Priority ID (0 = Lowest)
#define UART_IIR_TIMEOUT    0x08    // Receiver Timeout

//
// Interrupt Priority Levels
//
enum priority {
    ID_MS,                          // Modem Status (Lowest)
    ID_THRE,                        // Transmitter Holding Register Empty
    ID_RDA,                         // Received Data Available
    ID_RLS                          // Receiver Line Status (Highest)
};

//
// FIFO Control Register
//
// This is a write-only register that controls the transmitter and receiver
// FIFOs.
//
#define UART_FCR_EN         0x01    // FIFO Enable
#define UART_FCR_RESET_RCVR 0x02    // Receiver FIFO Reset
#define UART_FCR_RESET_XMIT 0x04    // Transmitter FIFO Reset
#define UART_FCR_DMA        0x08    // DMA Mode Select
#define UART_FCR_RCVR_TRIG  0xC0    // Receiver Interrupt Trigger

//
// Receiver Interrupt Trigger Levels
//
enum recv_trig {
    RCVR_TRIG_1,                    // Interrupt when 1 byte received
    RCVR_TRIG_4,                    // Interrupt when 4 bytes received
    RCVR_TRIG_8,                    // Interrupt when 8 bytes received
    RCVR_TRIG_14                    // Interrupt when 14 bytes received
};

//
// Line Control Register
//
// This register specifies the format of the transmitted and received data. It
// also provides access to the Divisor Line Access Bit, which enables the baud
// rate to be set via registers 0 and 1.
//
#define UART_LCR_WLS        0x03    // Word Length Select
#define UART_LCS_STB        0x04    // Stop Bit Select
#define UART_LCR_PEN        0x08    // Parity Enable
#define UART_LCR_EPS        0x10    // Even Parity Select
#define UART_LCR_STK        0x20    // Stick Parity
#define UART_LCR_BRK        0x40    // Set Break
#define UART_LCR_DLAB       0x80    // Divisor Latch Access

//
// Word Length Select Values
//
enum word_length {
    WLS_5,                          // 5 bits per character
    WLS_6,                          // 6 bits per character
    WLS_7,                          // 7 bits per character
    WLS_8                           // 8 bits per character
};

//
// Stop But Select Values
//
enum stop_bits {
    STB_1,                          // 1 stop bit
    STB_2,                          // 1.5 or 2 stop bits (1.5 when WSL_5 is used)
};

//
// Parity Modes
//
// This is a combined value of LCR bits [5:3].
//
enum parity {
    PARITY_NONE     = 0,
    PARITY_ODD      = UART_LCR_PEN,
    PARITY_EVEN     = UART_LCR_PEN | UART_LCR_EPS,
    PARITY_MARK     = UART_LCR_PEN | UART_LCR_STK,
    PARITY_SPACE    = UART_LCR_PEN | UART_LCR_EPS | UART_LCR_STK,
};

//
// Line Status Register
//
// This register provides data transfer status information.
//
#define UART_LSR_DR         0x01    // Data Ready
#define UART_LSR_OE         0x02    // Overrun Error
#define UART_LSR_PE         0x04    // Parity Error
#define UART_LSR_FE         0x08    // Framing Error
#define UART_LSR_BI         0x10    // Break Interrupt
#define UART_LSR_THRE       0x20    // Transmitter Holding Register Empty
#define UART_LSR_TEMT       0x40    // Transmitter Empty
#define UART_LSR_FIFO       0x80    // Error in FIFO

//
// Modem Control Register Masks
//
// This register controls the modem (or peripheral device) interface.
//
#define UART_MCR_DTR        0x01    // Data Terminal Ready
#define UART_MCR_RTS        0x02    // Request to Send
#define UART_MCR_OUT1       0x04    // Aux Output #1 (Ring Indicator)
#define UART_MCR_OUT2       0x08    // Aux Output #2 (Data Carrier Detect)
#define UART_MCR_LOOP       0x10    // Loopback Test

//
// Modem Status Register
//
// This register provdes information about the current state of the control
// lines from the modem (or peripheral device).
//
#define UART_MSR_DCTS       0x01    // Delta Clear to Send
#define UART_MSR_DDSR       0x02    // Delta Data Set Ready
#define UART_MSR_TERI       0x04    // Trailing Edge Ring Indicator
#define UART_MSR_DDCD       0x08    // Delta Data Carrier Detect
#define UART_MSR_CTS        0x10    // Clear to Send
#define UART_MSR_DSR        0x20    // Data Set Ready
#define UART_MSR_RI         0x40    // Ring Indicator
#define UART_MSR_DCD        0x80    // Data Carrier Detect

//
// COM Port Baud Rates
//
// The integer value of each enum value may be used to program the
// Baud Rate Divisor register.
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
    BAUD_200    = 576,
    BAUD_150    = 768,
    BAUD_134_5  = 857,
    BAUD_110    = 1047,
    BAUD_75     = 1536,
    BAUD_50     = 2304,
};

//
// ----------------------------------------------------------------------------
// UART registers in struct form.
//

//
// Interrupt Enable Register
//
struct ier {
    union {
        struct {
            uint8_t rda         : 1;    // Enable 'Received Data Available' Interrupt
            uint8_t thre        : 1;    // Enable 'Transmitter Holding Register Empty' Interrupt
            uint8_t rls         : 1;    // Enable 'Receiver Line Status' Interrupt
            uint8_t ms          : 1;    // Enable 'Modem Status' Interrupt
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct ier) == 1, "sizeof(struct ier)");

//
// Interrupt Identification Register
//
struct iir {
    union {
        struct {
            uint8_t no_int      : 1;    // Interrupt pending when 0
            uint8_t id          : 2;    // Interrupt priority ID; see priority enum
            uint8_t timeout     : 1;    // In FIFO mode, indicates a timeout interrupt is pending
            uint8_t             : 2;    // (unused)
            uint8_t fifo_en     : 2;    // Both bits are set to indicate when FIFOs are enabled
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct iir) == 1, "sizeof(struct iir)");

//
// FIFO Control Register
//
struct fcr {
    union {
        struct {
            uint8_t enable      : 1;    // Enable FIFOs
            uint8_t rx_reset    : 1;    // Clear receiver FIFO
            uint8_t tx_reset    : 1;    // Clear transmitter FIFO
            uint8_t dma         : 1;    // Enable DMA mode
            uint8_t             : 2;    // (reserved)
            uint8_t trig        : 2;    // FIFO depth; see recv_trig enum
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct fcr) == 1, "sizeof(struct fcr)");

//
// Line Control Register
//
struct lcr {
    union {
        struct {
            uint8_t word_length : 2;    // Word Length Select; see word_length enum
            uint8_t stop_bits   : 1;    // Stop Bit Select; see stop_bits enum
            uint8_t parity      : 3;    // Parity Select; see parity enum
            uint8_t break_cntl  : 1;    // Transmit Break
            uint8_t dlab        : 1;    // Divisor Latch Access
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct lcr) == 1, "sizeof(struct lcr)");

//
// Modem Control Register
//
struct mcr {
    union {
        struct {
            uint8_t dtr         : 1;    // Data Terminal Ready
            uint8_t rts         : 1;    // Request To Send
            uint8_t out1        : 1;    // Auxilary Output 1
            uint8_t out2        : 1;    // Auxilary Output 2
            uint8_t loop        : 1;    // Loopback Test
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct mcr) == 1, "sizeof(struct mcr)");

//
// Line Status Register
//
struct lsr {
    union {
        struct {
            uint8_t dr          : 1;    // Received data ready to be fetched
            uint8_t oe          : 1;    // Receive buffer overrun
            uint8_t pe          : 1;    // Parity error detected in received data
            uint8_t fe          : 1;    // Invalid stop bit in received data
            uint8_t brk         : 1;    // Raised when data input is held low for longer than one word
            uint8_t thre        : 1;    // Transmitter holding register empty; ready to transmit
            uint8_t temt        : 1;    // Transmitter idle; nothing is transmitting
            uint8_t fifo        : 1;    // In FIFO mode, indicates an error on one of the chars in the FIFO
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct lsr) == 1, "sizeof(struct lsr)");

//
// Modem Status Register
//
struct msr {
    union {
        struct {
            uint8_t dcts        : 1;    // Delta Clear to Send
            uint8_t ddsr        : 1;    // Delta Data Set Ready
            uint8_t teri        : 1;    // Trailing Edge Ring Indicator
            uint8_t ddcd        : 1;    // Delta Data Carrier Detect
            uint8_t cts         : 1;    // Clear To Send
            uint8_t dsr         : 1;    // Data Set Ready
            uint8_t ri          : 1;    // Ring Indicator
            uint8_t dcd         : 1;    // Data Carrier Detect
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct msr) == 1, "sizeof(struct msr)");

#endif // __SERIAL_H
