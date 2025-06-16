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
#include <stdlib.h>
#include <string.h>
#include <i386/gdbstub.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/serial.h>

#define GDB_MAXNACK             10
#define GDB_ENABLE_DEBUG        0

#if GDB_ENABLE_DEBUG
#define GDB_PRINT(...)  kprint("gdb: " __VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

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
static int gdb_send_ok_packet(struct gdb_state *state);
static int gdb_send_signal_packet(struct gdb_state *state, int signal);
static int gdb_send_error_packet(struct gdb_state *state, int error);

// data encoding/decoding
static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len);
static int decode_hex(const char *buf, size_t bufsiz, const void *data, size_t len);

// command handling
static int gdb_cmd_step(struct gdb_state *state);
static int gdb_cmd_continue(struct gdb_state *state);
static int gdb_cmd_query(struct gdb_state *state);
static int gdb_cmd_read_regs(struct gdb_state *state);
static int gdb_cmd_write_regs(struct gdb_state *state, char *pkt, size_t pktlen);
static int gdb_cmd_read_mem(struct gdb_state *state, char *pkt, size_t pktlen);
static int gdb_cmd_write_mem(struct gdb_state *state, char *pkt, size_t pktlen);

__initmem bool g_debug_port_available = false;

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

void gdb_main(struct gdb_state *state)
{
    char pkt[GDB_MAXLEN];
    size_t len;
    int status;

    if (!g_debug_port_available) {
        panic("serial debugging unavailable!");
        return;
    }

    status = gdb_cmd_query(state);
    if (status == EOF) {
        goto error;
    }

    while (true) {
        status = gdb_recv_packet(state, pkt, sizeof(pkt), &len);
        if (status == EOF) {
            GDB_PRINT("gdb_recv_packet() returned EOF, exiting...\n");
            break;
        }
        if (len == 0) {
            continue;
        }

        switch (pkt[0]) {
            case '?':   // query halt state
                status = gdb_cmd_query(state);
                break;

            case 'g':   // read regs
                status = gdb_cmd_read_regs(state);
                break;

            case 'G':   // write regs
                status = gdb_cmd_write_regs(state, &pkt[1], len-1);
                break;

            case 'm':   // read memory
                status = gdb_cmd_read_mem(state, &pkt[1], len-1);
                break;

            case 'M':    // write memory
                status = gdb_cmd_write_mem(state, &pkt[1], len-1);
                break;

            case 'c':   // continue
                gdb_cmd_continue(state);
                return;

            case 's':   // step
                gdb_cmd_step(state);
                return;

            default:
                status = gdb_send_packet(state, NULL, 0);
                break;
        }

    error:
        if (status == EOF) {
            gdb_send_error_packet(state, 0);
        }
    }
}

static int gdb_cmd_step(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] |= EFLAGS_TF;
    return 0;
}

static int gdb_cmd_continue(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] &= ~EFLAGS_TF;
    return 0;
}

static int gdb_cmd_query(struct gdb_state *state)
{
    return gdb_send_signal_packet(state, state->signum);
}

static int gdb_cmd_read_regs(struct gdb_state *state)
{
    const size_t bufsiz = (sizeof(gdb_i386_reg) * GDB_NUM_I386_REGS) * 2;
    char buf[bufsiz];

    return gdb_send_packet(state, buf, encode_hex(buf,
        sizeof(buf), state->regs, sizeof(state->regs)));
}

static int gdb_cmd_write_regs(struct gdb_state *state, char *pkt, size_t pktlen)
{
    int status;

    status = decode_hex(pkt, pktlen, state->regs, sizeof(state->regs));
    if (status == EOF) {
        return gdb_send_error_packet(state, 0);
    }

    return gdb_send_ok_packet(state);
}

static int gdb_cmd_read_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    void *addr;
    size_t count;
    char  data[GDB_MAXLEN/2];
    char tx_pkt[GDB_MAXLEN];
    char *p;

    pkt[pktlen] = '\0';    // no NUL protection built into strtol

    addr = (void *) strtol(pkt, &p, 16);
    if (*p == '\0') {
        return EOF;     // invalid packet format
    }

    count = strtol(p+1, NULL, 16);
    if (count > sizeof(data)) {
        return EOF;     // too many bytes requested
    }

    for (int i = 0; i < count; i++) {
        data[i] = *(((char *) addr) + i);
    }

    return gdb_send_packet(state, tx_pkt, encode_hex(tx_pkt,
        sizeof(tx_pkt), data, count));
}

static int gdb_cmd_write_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    int status;
    void *addr;
    size_t count;
    char  data[GDB_MAXLEN/2];
    char *p;

    pkt[pktlen] = '\0';    // no NUL protection built into strtol

    addr = (void *) strtol(pkt, &p, 16);
    if (*p != ',') {
        return EOF;     // invalid packet format
    }

    count = strtol(p+1, &p, 16);
    if (*p != ':') {
        return EOF;     // invalid packet format
    }
    if (count > sizeof(data)) {
        return EOF;     // too many bytes requested
    }

    status = decode_hex(p+1, (size_t) pktlen - (p+1 - pkt), data, count);
    if (status == EOF) {
        return EOF;
    }

    for (int i = 0; i < count; i++) {
        *(((char *) addr) + i) = data[i];
    }
    return gdb_send_ok_packet(state);
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
            if (isprint(c)) {
                GDB_PRINT("error: bad packet response '%c'\n", c);
            }
            else {
                GDB_PRINT("error: bad packet response \\x%02hhx\n", c);
            }
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
        if (isprint(c)) {
            GDB_PRINT("expecting '$', got '%c'\n", c);
        }
        else {
            GDB_PRINT("expecting '$', got \\x%02hhx\n", c);
        }
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
        GDB_PRINT("cksum: expecting %02hhx, got %02hhx\n", tx_cksum, cksum);
        gdb_putc(state, '-');  // NACK
        return EOF;
    }

    gdb_putc(state, '+');
    return 0;
}

static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len)
{
    uint8_t cksum;
    char cksum_buf[2];
    int status;
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

    status = encode_hex(cksum_buf, sizeof(cksum_buf), &cksum, 1);
    if (status == EOF) {
        return EOF;
    }

    gdb_putc(state, '#');
    gdb_putc(state, cksum_buf[0]);
    gdb_putc(state, cksum_buf[1]);

    GDB_PRINT("<- $%.*s#%c%c\n",
        state->tx_len, state->tx_buf, cksum_buf[0], cksum_buf[1]);

    return gdb_recv_ack(state);
}

static int gdb_send_ok_packet(struct gdb_state *state)
{
    return gdb_send_packet(state, "OK", 2);
}

static int gdb_send_signal_packet(struct gdb_state *state, int signal)
{
    char buf[8];
    size_t len;
    int status;

    buf[0] = 'S';
    status = encode_hex(buf+1, sizeof(buf)-1, &signal, 1);
    if (status == EOF) {
        return EOF;
    }

    len = status + 1;
    return gdb_send_packet(state, buf, len);
}

static int gdb_send_error_packet(struct gdb_state *state, int error)
{
    char buf[8];
    size_t len;
    int status;

    buf[0] = 'E';
    status = encode_hex(buf+1, sizeof(buf)-1, &error, 1);
    if (status == EOF) {
        return EOF;
    }

    len = status + 1;
    return gdb_send_packet(state, buf, len);
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
