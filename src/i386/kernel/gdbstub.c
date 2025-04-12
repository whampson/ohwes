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
#include <stdio.h>
#include <string.h>
#include <i386/gdbstub.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/serial.h>

#define GDB_MAXNACK             10

#define GDB_ENABLE_INTERRUPT    0
#define GDB_ENABLE_DEBUG        1

#if GDB_ENABLE_DEBUG
#define GDB_PRINT(...)  kprint("gdb: " __VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

// initialization (called once at OS startup)
void init_gdb(void);

// serial port interrupt handler
void gdb_serial_interrupt(int irq, struct iregs *regs);

// serial i/o
inline static char com_in(int port);
inline static char com_out(int port, char data);

// basic get/put functions
static char gdb_getc(struct gdb_state *state);
static char gdb_putc(struct gdb_state *state, char c);

// packet i/o
static int gdb_recv_ack(struct gdb_state *state);  // 1 = nack, 0 = ack, EOF = error
static int gdb_recv_packet(struct gdb_state *state, char *buf, size_t bufsiz, size_t *len);
static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len);

// data encoding/decoding
static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len);
static int decode_hex(const char *buf, size_t bufsiz, const void *data, size_t len);

// command handling
static int gdb_cmd_query(struct gdb_state *state);
static int gdb_cmd_read_regs(struct gdb_state *state);
static int gdb_cmd_read_memory(struct gdb_state *state, char *pkt, size_t len);

void init_gdb(void)
{
    if (reserve_io_range(SERIAL_DEBUG_PORT, 8, "serial_debug") < 0) {
        kprint("unable to reserve I/O ports for serial debugging!\n");
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
    struct ier ier = {
        .rda        = 1,            // data ready
        .rls        = 1,            // line status interrupt
        .ms         = 1,            // modem status interrupt
    };
    uint16_t baud   = SERIAL_DEBUG_BAUD;

    com_out(UART_IER, ier._value);
    com_out(UART_MCR, mcr._value);
    com_out(UART_LCR, UART_LCR_DLAB);
    com_out(UART_DLM, baud >> 8);
    com_out(UART_DLL, baud & 0xFF);
    com_out(UART_LCR, lcr._value);
    com_out(UART_FCR, 0);           // disable the FIFO

    (void) com_in(UART_LSR);
    (void) com_in(UART_MSR);
    (void) com_in(UART_IIR);

#if GDB_ENABLE_INTERRUPT
    irq_register(IRQ_COM1, gdb_serial_interrupt);
#endif
}

int gdb_init_state(struct gdb_state *state,
    int signum, const struct iregs *regs)
{
    if (!state || !regs) {
        return EOF;
    }

    zeromem(state, sizeof(struct gdb_state));
    state->signum = signum;
    state->regs[GDB_REG_I386_EBX] = regs->ebx;
    state->regs[GDB_REG_I386_ECX] = regs->ecx;
    state->regs[GDB_REG_I386_EDX] = regs->edx;
    state->regs[GDB_REG_I386_ESI] = regs->esi;
    state->regs[GDB_REG_I386_EDI] = regs->edi;
    state->regs[GDB_REG_I386_EBP] = regs->ebp;
    state->regs[GDB_REG_I386_EAX] = regs->eax;
    state->regs[GDB_REG_I386_DS ] = regs->ds;
    state->regs[GDB_REG_I386_ES ] = regs->es;
    state->regs[GDB_REG_I386_FS ] = regs->fs;
    state->regs[GDB_REG_I386_GS ] = regs->gs;
    state->regs[GDB_REG_I386_EIP] = regs->eip;
    state->regs[GDB_REG_I386_CS ] = regs->cs;
    state->regs[GDB_REG_I386_EFLAGS] = regs->eflags;
    state->regs[GDB_REG_I386_ESP] = regs->esp;
    state->regs[GDB_REG_I386_SS ] = regs->ss;
    return 0;
}

int gdb_main(struct gdb_state *state)
{
    char pkt[GDB_MAXLEN];
    size_t len;
    int status;

    bool loop_forever = true;
    do {
        status = gdb_recv_packet(state, pkt, sizeof(pkt), &len);
        if (status == EOF) {
            break;
        }
        if (len == 0) {
            continue;
        }

        switch (pkt[0]) {
            case '?':
                status = gdb_cmd_query(state);
                break;

            case 'g':
                status = gdb_cmd_read_regs(state);
                break;

            case 'm':
                status = gdb_cmd_read_memory(state, &pkt[1], len-1);
                break;

            default:
                status = gdb_send_packet(state, NULL, 0);
                break;
        }
    } while (status != EOF || loop_forever);

    return status;
}

static int gdb_cmd_query(struct gdb_state *state)
{
    char buf[8];
    size_t len;

    buf[0] = 'S';
    len = encode_hex(buf+1, sizeof(buf)-1, &state->signum, 1);
    return gdb_send_packet(state, buf, len+1);
}

static int gdb_cmd_read_regs(struct gdb_state *state)
{
    const size_t bufsiz = (sizeof(gdb_i386_reg) * GDB_NUM_I386_REGS) * 2;
    char buf[bufsiz];

    return gdb_send_packet(state, buf, encode_hex(buf,
        sizeof(buf), state->regs, sizeof(state->regs)));
}

static int gdb_cmd_read_memory(struct gdb_state *state, char *pkt, size_t len)
{
    int sep;
    GDB_PRINT("pkt = '%.*s'\n", len, pkt);

    sep = -1;
    for (int i = 0; i < len; i++) {
        if (pkt[i] == ',') {
            sep = i;
            break;
        }
    }

    if (sep <= 0) {
        // no memory address! TODO: tx error packet?
        return EOF;
    }

    // TODO: need strtol to get addr,count

    GDB_PRINT("comma = %d\n", sep);

    return 0;
}

static int gdb_recv_ack(struct gdb_state *state)
{
    char c;

    c = gdb_getc(state);
    switch (c) {
        case '+':   // ACK
            break;
        case '-':   // NACK
            GDB_PRINT("NACK\n");
            if (++state->nack_count >= GDB_MAXNACK) {
                GDB_PRINT("error: received %d NACKs, giving up...\n",
                    state->nack_count);
                return EOF;
            }
            // retransmit...
            return gdb_send_packet(state, state->tx_buf, state->tx_len);
        default:
            GDB_PRINT("error: bad packet response '%c'\n", c);
            break;
    }

    return 0;
}

static int gdb_recv_packet(struct gdb_state *state,
    char *buf, size_t bufsiz, size_t *len)
{
    // packet formats:
    //   $packet-data#checksum
    //   $sequence-id:packet-data#checksum
    // sequence-id should never appear in packets transmitted by GDB

    char c;
    uint8_t cksum, tx_cksum;
    char cksum_buf[2];
    int status;

    if (!buf || !len) {
        GDB_PRINT("error: user provided NULL buffer!\n");
        return EOF;
    }

    // read 'til we find packet start
    while (1) {
        c = gdb_getc(state);
        if (c == EOF) {
            return EOF;
        }
        if (c == '$') {
            break;      // packet start
        }
        GDB_PRINT("expecting '$', got '%c' (\\x%02x)\n", c, c);
    }

    // read-in packet data
    *len = 0; cksum = 0;
    while (1) {
        c = gdb_getc(state);
        if (c == EOF) {
            return EOF;
        }
        if (c == '#') {
            break;      // checksum start
        }
        if (*len >= (bufsiz-1) || *len >= (GDB_MAXLEN-1)) {
            GDB_PRINT("error: recv packet buffer overflow!\n");
            return EOF;
        }
        if (buf) {
            buf[(*len)++] = c;
        }
        cksum += c;
    }

    buf[*len] = '\0';

    // read-in transmitted checksum
    for (int i = 0; i < 2; i++) {
        c = gdb_getc(state);
        if (c == EOF) {
            return EOF;
        }
        cksum_buf[i] = c;
    }
    status = decode_hex(cksum_buf, sizeof(cksum_buf), &tx_cksum, sizeof(tx_cksum));
    if (status == EOF) {
        return EOF;
    }

    GDB_PRINT("-> $%.*s#%02x\n", *len, buf, cksum);

    // verify checksum
    if (cksum != tx_cksum) {
        GDB_PRINT("cksum: expecting %02x, got %02x\n", tx_cksum, cksum);
        gdb_putc(state, '-');  // NACK
        return 1;
    }

    gdb_putc(state, '+');
    return 0;
}

static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len)
{
    uint8_t cksum;
    char cksum_buf[2];
    int i;

    if (!buf && len > 0) {
        GDB_PRINT("error: user provided NULL buffer!\n");
        return EOF;
    }

    if (len > (GDB_MAXLEN-1)) {
        GDB_PRINT("error: send packet buffer overflow!\n");
        return EOF;
    }

    gdb_putc(state, '$');

    cksum = 0; i = 0;
    while (i < len && buf) {
        state->tx_buf[i] = buf[i];
        cksum += gdb_putc(state, state->tx_buf[i++]);
    }
    state->tx_buf[i] = '\0';
    state->tx_len = i;

    encode_hex(cksum_buf, sizeof(cksum_buf), &cksum, 1);

    gdb_putc(state, '#');
    gdb_putc(state, cksum_buf[0]);
    gdb_putc(state, cksum_buf[1]);

    GDB_PRINT("<- $%.*s#%c%c\n",
        state->tx_len, state->tx_buf, cksum_buf[0], cksum_buf[1]);

    return gdb_recv_ack(state);
}

static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len)
{
    uint8_t c;

    if (!buf || !data || (len*2) > bufsiz) {
        return EOF;
    }

    for (int i = 0; i < len; i++) {
        c = ((uint8_t *) data)[i];
        *buf++ = toxdigit(c >> 4);
        *buf++ = toxdigit(c & 0xF);
    }

    return len * 2;
}

static int decode_hex(const char *buf, size_t bufsiz, const void *data, size_t len)
{
    uint8_t *p;
    char c;

    if (!buf || !data || (len*2) > bufsiz) {
        return EOF;
    }

    p = (uint8_t *) data;
    for (int i = 0; i < len; i++, p++) {
        *p = 0;
        for (int j = 0; j < 2; j++) {
            c = *buf++;
            *p <<= 4;
            if (isdigit(c)) {
                *p += (c - '0');
            }
            else if (isxdigit(c)) {
                *p += (tolower(c) - 'a') + 0xA;
            }
        }
    }

    return 0;
}

static char gdb_getc(struct gdb_state *state)
{
    // if (state->rx_char != 0) { // get char received by interrupt first
    //     char c = state->rx_char;
    //     state->rx_char = 0;
    //     return c;
    // }

    while ((com_in(UART_LSR) & UART_LSR_DR) == 0);
    return com_in(UART_RX);
}

static char gdb_putc(struct gdb_state *state, char c)
{
    (void) state;

    while ((com_in(UART_LSR) & UART_LSR_THRE) == 0);
    com_out(UART_TX, c);
    return c;
}

inline static char com_in(int port)
{
    return inb(SERIAL_DEBUG_PORT + port);
}

inline static char com_out(int port, char data)
{
    outb(SERIAL_DEBUG_PORT + port, data);
    return data;
}

#if GDB_ENABLE_INTERRUPT
void gdb_serial_interrupt(int irq, struct iregs *regs)
{
    int pass;

    struct iir iir;
    iir._value = com_in(UART_IIR);
    if (iir.no_int) {
        return;
    }

    pass = 0;
    do {
        struct lsr lsr;
        lsr._value = com_in(UART_LSR);
        if (lsr._value & 0x1E) {
            GDB_PRINT("com: lsr: %s%s%s%s%s\n",
                (lsr.dr)  ? "ready "   : "",
                (lsr.oe)  ? "overrun " : "",
                (lsr.pe)  ? "parity "  : "",
                (lsr.fe)  ? "frame "   : "",
                (lsr.brk) ? "break "   : "");
        }

        uint8_t msr = com_in(UART_MSR);
        if (msr & UART_MSR_ANY_DELTA) {
            GDB_PRINT("msr: %s%s%s%s%s%s%s%s\n",
                (msr & UART_MSR_DCTS) ? "dcts " : "",
                (msr & UART_MSR_DDSR) ? "ddsr " : "",
                (msr & UART_MSR_TERI) ? "teri " : "",
                (msr & UART_MSR_DDCD) ? "ddcd " : "",
                (msr & UART_MSR_CTS)  ? "cts "  : "",
                (msr & UART_MSR_DSR)  ? "dsr "  : "",
                (msr & UART_MSR_RI)   ? "ri "   : "",
                (msr & UART_MSR_DCD)  ? "dcd "  : "");
        }

        if (iir.id == ID_RDA || iir.timeout || lsr.dr) {
            char c = com_in(UART_RX);
            if (c == 3) {
                struct gdb_state state;
                gdb_init_state(&state, GDB_SIGINT, regs);
                gdb_main(&state);
            }
        }

        if (pass >= 16) {
            break;
        }

        iir._value = com_in(UART_IIR);
    } while (iir.no_int == 0);
}
#endif
