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
 *         File: i386/kernel/gdbstub.c
 *      Created: April 5, 2025
 *       Author: Wes Hampson
 *
 * Support debugging over serial port with GDB.
 * Inspired by https://github.com/mborgerson/gdbstub.
 * =============================================================================
 */

#include <ctype.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/serial.h>

#define GDB_EOF         (-1)

#define DEBUG_STUB      1
#if DEBUG_STUB
#define GDB_PRINT(...)  kprint(__VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

static char gdb_getc(void);
static void gdb_putc(char c);

inline static char com_in(int port);
inline static void com_out(int port, char data);

static int gdb_recv_packet(char *buf, size_t *len, size_t maxlen);

void init_gdb(void)
{
    if (reserve_io_range(DEBUG_PORT, 8, "serial_debug") < 0) {
        kprint("gdb: unable to reserve I/O ports for serial debugging!\n");
    }

    struct lcr lcr = {
        .word_length = WLS_8,       // 8 data bits
        .stop_bits   = STB_1,       // 1 stop bit
        .parity      = PARITY_NONE, // no parity
    };
    struct mcr mcr = {
        .dtr        = 1,            // data terminal ready
        .rts        = 1,            // request to send
        .out2       = 1,            // carrier detect
    };
    uint16_t baud   = DEBUG_BAUD;

    com_out(UART_IER, 0);           // disable interrupts because we're polling
    com_out(UART_MCR, mcr._value);
    com_out(UART_LCR, UART_LCR_DLAB);
    com_out(UART_DLM, baud >> 8);
    com_out(UART_DLL, baud & 0xFF);
    com_out(UART_LCR, lcr._value);
    com_out(UART_FCR, 0);           // disable the FIFO

    // TODO: need an interrupt handler to receive
    // CTRL+C sent by GDB for break!!

    (void) com_in(UART_LSR);
    (void) com_in(UART_MSR);
    (void) com_in(UART_IIR);
}

void gdb_main(struct iregs *regs)
{
    kprint("*** %s ***\nRegisters:\n", (regs->vec_num == 3) ? "BREAKPOINT" : "DEBUG_BREAK");
    kprint("   eax: %08x\n", regs->eax);
    kprint("   ebx: %08x\n", regs->ebx);
    kprint("   ecx: %08x\n", regs->ecx);
    kprint("   edx: %08x\n", regs->edx);
    kprint("   esi: %08x\n", regs->esi);
    kprint("   edi: %08x\n", regs->edi);
    kprint("   ebp: %08x\n", regs->ebp);
    kprint("   esp: %08x\n", regs->esp);
    kprint("   eip: %08x\n", regs->eip);
    kprint("eflags: %08x\n", regs->eflags);
    kprint("    cs: %04x\n", regs->cs);
    kprint("    ds: %04x\n", regs->ds);
    kprint("    es: %04x\n", regs->es);
    kprint("    fs: %04x\n", regs->fs);
    kprint("    gs: %04x\n", regs->gs);

    char buf[1024];
    size_t len;

    while (gdb_recv_packet(buf, &len, sizeof(buf)) != GDB_EOF);
    gdb_putc('-');
}

static int gdb_recv_packet(char *buf, size_t *len, size_t maxlen)
{
    // packet formats:
    //   $packet-data#checksum
    //   $sequence-id:packet-data#checksum
    // sequence-id should never appear in packets transmitted by GDB

    char c;
    uint8_t cksum, tx_cksum;

    // read 'til we find packet start
    while (1) {
        c = gdb_getc();
        if (c == GDB_EOF) {
            return GDB_EOF;
        }
        if (c == '$') {
            break;      // packet start
        }
        // GDB_PRINT("gdb: warn: expecting '$', got '%c' (\\x%02x)\n", c, c);
    }

    // read-in packet data
    *len = 0; cksum = 0;
    while (1) {
        c = gdb_getc();
        if (c == GDB_EOF) {
            return GDB_EOF;
        }
        if (c == '#') {
            break;      // checksum start
        }
        if (*len >= maxlen) {
            GDB_PRINT("gdb: error: packet buffer overflow!\n");
            return GDB_EOF;
        }
        buf[(*len)++] = c;
        cksum += c;
    }

    tx_cksum = 0;
    for (int i = 0; i < 2; i++) {
        c = gdb_getc();
        if (c == GDB_EOF) {
            return GDB_EOF;
        }
        tx_cksum <<= 4;
        if (isdigit(c)) {
            tx_cksum += (c - '0');
        }
        else if (isxdigit(c)) {
            tx_cksum += (toupper(c) - 'A') + 0xA;
        }
    }

    GDB_PRINT("gdb: -> $%.*s#%02x\n", *len, buf, cksum);
    if (cksum != tx_cksum) {
        GDB_PRINT("gdb: error: bad checksum! (exp=%02x, got=%02x)\n", tx_cksum, cksum);
        gdb_putc('-');  // NACK
    }

    return 0;
}

static char gdb_getc(void)
{
    while ((com_in(UART_LSR) & UART_LSR_DR) == 0);
    return com_in(UART_RX);
}

static void gdb_putc(char c)
{
    while ((com_in(UART_LSR) & UART_LSR_THRE) == 0);
    com_out(UART_TX, c);
}

inline static char com_in(int port)
{
    return inb(DEBUG_PORT + port);
}

inline static void com_out(int port, char data)
{
    outb(DEBUG_PORT + port, data);
}
