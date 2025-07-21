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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i386/bitops.h>
#include <i386/gdbstub.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/serial.h>

#define ENABLE_GDB_PRINT        0
#define ENABLE_GDB_ERROR        0

#define PRINT_ACKS              1
#define NACK_THRESH             10

#if ENABLE_GDB_PRINT
#define GDB_PRINT(...)  kprint("gdb: " __VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

#if ENABLE_GDB_ERROR
#define GDB_ERROR(...)  kprint("\e[1;33mgdb: error: " __VA_ARGS__); kprint("\e[m")
#else
#define GDB_ERROR(...)
#endif

extern void hard_reset(void);

// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Errno-Values.html
enum gdb_errno {
    GDB_EUNKNOWN     = 0,
    GDB_EPERM        = 1,
    GDB_ENOENT       = 2,
    GDB_EINTR        = 4,
    GDB_EBADF        = 9,
    GDB_EACCES       = 13,
    GDB_EFAULT       = 14,
    GDB_EBUSY        = 16,
    GDB_EEXIST       = 17,
    GDB_ENODEV       = 19,
    GDB_ENOTDIR      = 20,
    GDB_EISDIR       = 21,
    GDB_EINVAL       = 22,
    GDB_ENFILE       = 23,
    GDB_EMFILE       = 24,
    GDB_EFBIG        = 27,
    GDB_ENOSPC       = 28,
    GDB_ESPIPE       = 29,
    GDB_EROFS        = 30,
    GDB_ENAMETOOLONG = 91,
};

// packet i/o
static int gdb_recv_ack(struct gdb_state *state);
static int gdb_recv_packet(struct gdb_state *state, char *buf, size_t bufsiz, size_t *len);
static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len);
static int gdb_send_ok_packet(struct gdb_state *state);
static int gdb_send_empty_packet(struct gdb_state *state);
static int gdb_send_signal_packet(struct gdb_state *state, int signal);
static int gdb_send_error_packet(struct gdb_state *state, int error);
static int gdb_send_ack(struct gdb_state *state);
static int gdb_send_nack(struct gdb_state *state);
static int gdb_handle_ack(struct gdb_state *state);
static int gdb_handle_nack(struct gdb_state *state);

// data encoding/decoding
static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len);
static int decode_hex(const char *buf, size_t bufsiz, void *data, size_t len);

// command handling
static void gdb_step(struct gdb_state *state);
static void gdb_continue(struct gdb_state *state);
static void gdb_detach(struct gdb_state *state);
static int gdb_read_regs(struct gdb_state *state);
static int gdb_write_regs(struct gdb_state *state, char *pkt, size_t pktlen);
static int gdb_read_mem(struct gdb_state *state, char *pkt, size_t pktlen);
static int gdb_write_mem(struct gdb_state *state, char *pkt, size_t pktlen);

// basic get/put functions
static char gdb_getc(struct gdb_state *state);
static char gdb_putc(struct gdb_state *state, char c);

void gdb_init(struct gdb_state *state, struct com *com)
{
    zeromem(state, sizeof(struct gdb_state));
    state->com = com;
}

void gdb_capture(struct gdb_state *state, const struct iregs *regs, int signum)
{
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
    state->signum = signum;
}

void gdb_apply(struct gdb_state *state, struct iregs *regs)
{
    regs->ebx = state->regs[GDB_REG_I386_EBX];
    regs->ecx = state->regs[GDB_REG_I386_ECX];
    regs->edx = state->regs[GDB_REG_I386_EDX];
    regs->esi = state->regs[GDB_REG_I386_ESI];
    regs->edi = state->regs[GDB_REG_I386_EDI];
    regs->ebp = state->regs[GDB_REG_I386_EBP];
    regs->eax = state->regs[GDB_REG_I386_EAX];
    regs->ds  = state->regs[GDB_REG_I386_DS ];
    regs->es  = state->regs[GDB_REG_I386_ES ];
    regs->fs  = state->regs[GDB_REG_I386_FS ];
    regs->gs  = state->regs[GDB_REG_I386_GS ];
    regs->eip = state->regs[GDB_REG_I386_EIP];
    regs->cs  = state->regs[GDB_REG_I386_CS ];
    regs->eflags = state->regs[GDB_REG_I386_EFLAGS];
    regs->esp = state->regs[GDB_REG_I386_ESP];
    regs->ss  = state->regs[GDB_REG_I386_SS ];
}

void gdb_main(struct gdb_state *state, struct iregs *regs, int signum)
{
    size_t len;
    int status;
    char pkt[GDB_MAXLEN];
    uint32_t flags;
    bool com_int;
    bool ctrl_c;
    bool packet_start;

    static bool debugging = false;
    if (test_and_set_bit(&debugging, 0)) {
        return;
    }

    cli_save(flags);
    gdb_capture(state, regs, signum);

    status = 0;
    com_int = (state->com->iir.no_int == 0) && (state->pending_char != 0);
    ctrl_c = com_int && (state->pending_char == 0x03);
    packet_start = com_int && (state->pending_char == '$');

    if (!state->com->io_port) {
        state->com->io_port = SERIAL_DEBUG_PORT;
    }
    assert(state->com->io_port == SERIAL_DEBUG_PORT);

    if (ctrl_c) {
        (void) gdb_getc(state); // consume CTRL+C
    }

    if (!com_int || ctrl_c /*|| packet_start*/) {
        status = gdb_send_signal_packet(state, state->signum);  // notify GDB host
    }
    else if (!packet_start) {
        // enable COM interrupts
        com_out(state->com, UART_IER, UART_IER_RDA);
        goto gdb_done;  // ignore interrupt if not packet start
    }

    // disable debug COM interrupts
    com_out(state->com, UART_IER, 0);

    while (status != EOF) {
        status = gdb_recv_packet(state, pkt, sizeof(pkt), &len);
        if (status == EOF) {
            break;
        }
        else if (status == 0x03) {
            GDB_PRINT("handling CTRL+C...\n");
            status = gdb_send_signal_packet(state, SIGINT); // TODO: is this correct?
            break;
        }
        else if (len == 0) {
            continue;
        }

        // TODO:
        // 'p/P' - read/write io register?

        switch (pkt[0]) {
            case '?':
                status = gdb_send_signal_packet(state, state->signum);
                break;

            case 'g':
                status = gdb_read_regs(state);
                break;
            case 'G':
                status = gdb_write_regs(state, &pkt[1], len-1);
                break;

            case 'm':
                status = gdb_read_mem(state, &pkt[1], len-1);
                break;
            case 'M':
                status = gdb_write_mem(state, &pkt[1], len-1);
                break;

            case 's':
                gdb_step(state);
                goto gdb_done;
            case 'c':
                gdb_continue(state);
                goto gdb_done;
            case 'D':
                gdb_detach(state);
                goto gdb_done;

            case '+':
                status = gdb_handle_ack(state);
                break;
            case '-':
                status = gdb_handle_nack(state);
                break;

            default:
                status = gdb_send_empty_packet(state);
                break;
        }
    }

    if (status == EOF) {
        GDB_ERROR("EOF!\n");
        status = gdb_send_error_packet(state, GDB_EUNKNOWN);    // communication error
    }

gdb_done:
    clear_bit(&debugging, 0);
    gdb_apply(state, regs);
    restore_flags(flags);
}

static void gdb_step(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] |= EFLAGS_TF;
}

static void gdb_continue(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] &= ~EFLAGS_TF;

    // enable COM interrupts
    com_out(state->com, UART_IER, UART_IER_RDA);
}

static void gdb_detach(struct gdb_state *state)
{
    gdb_send_ok_packet(state);
    gdb_continue(state);
}

static int gdb_read_regs(struct gdb_state *state)
{
    const size_t bufsiz = (sizeof(gdb_i386_reg) * GDB_NUM_I386_REGS) * 2;
    char buf[bufsiz];

    return gdb_send_packet(state, buf, encode_hex(buf,
        sizeof(buf), state->regs, sizeof(state->regs)));
}

static int gdb_write_regs(struct gdb_state *state, char *pkt, size_t pktlen)
{
    int status;

    status = decode_hex(pkt, pktlen, state->regs, sizeof(state->regs));
    if (status == EOF) {
        return EOF;     // payload too large
    }

    return gdb_send_ok_packet(state);
}

static int gdb_read_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    void *addr;
    size_t count;
    char  data[GDB_MAXLEN/2];
    char tx_pkt[GDB_MAXLEN];
    char *p;

    pkt[pktlen] = '\0';    // no NUL protection built into strtol

    addr = (void *) strtol(pkt, &p, 16);
    if (*p == '\0') {
        GDB_ERROR("gdb_read_mem: bad packet format\n");
        return EOF;
    }

    count = strtol(p+1, NULL, 16);
    if (count > sizeof(data)) {
        GDB_ERROR("gdb_read_mem: too many bytes requested!\n");
        return EOF;
    }

    if (!virt_addr_valid(addr) || !virt_addr_valid(addr + count)) {
        return gdb_send_error_packet(state, GDB_EFAULT);
    }

    for (int i = 0; i < count; i++) {
        data[i] = *(((char *) addr) + i);
    }

    return gdb_send_packet(state, tx_pkt, encode_hex(tx_pkt,
        sizeof(tx_pkt), data, count));
}

static int gdb_write_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    int status;
    void *addr;
    size_t count;
    char data[GDB_MAXLEN/2] = { };
    char *p;

    pkt[pktlen] = '\0';    // no NUL protection built into strtol

    addr = (void *) strtol(pkt, &p, 16);
    if (*p != ',') {
        GDB_ERROR("gdb_write_mem: bad packet format\n");
        return EOF;
    }

    count = strtol(p+1, &p, 16);
    if (*p != ':') {
        GDB_ERROR("gdb_write_mem: bad packet format\n");
        return EOF;
    }
    if (count > sizeof(data)) {
        GDB_ERROR("gdb_write_mem: too many bytes requested\n");
        return EOF;
    }

    status = decode_hex(p+1, (size_t) pktlen - (p+1 - pkt), data, count);
    if (status == EOF) {
        return EOF;     // payload too large
    }

    if (!virt_addr_valid(addr) || !virt_addr_valid(addr + count)) {
        return gdb_send_error_packet(state, GDB_EFAULT);
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
            return gdb_handle_ack(state);
        case '-':   // NACK
            return gdb_handle_nack(state);
    }

    if (isprint(c)) {
        GDB_PRINT("expecting '+' or '-', got '%c'\n", c);
    }
    else {
        GDB_PRINT("expecting '+' or '-', got \\x%02hhx\n", c);
    }

    state->pending_char = c;
    return c;
}

static int gdb_handle_ack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("-> +\n");
#endif

    state->ack_count++;
    return 0;
}

static int gdb_handle_nack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("-> - (NACK)\n");
#endif

    state->nack_count++;
    if ((state->nack_count % NACK_THRESH) == 0) {
        GDB_ERROR("received %d NACKs, what gives??\n", state->nack_count);
    }

    // retransmit...
    return gdb_send_packet(state, state->tx_buf, state->tx_len);
}

static int gdb_recv_packet(struct gdb_state *state,
    char *buf, size_t bufsiz, size_t *len)
{
    // packet formats:
    //   $packet-data#checksum
    //   $sequence-id:packet-data#checksum
    // sequence-id should never appear in packets transmitted by GDB

    char c;
    size_t length;
    uint8_t cksum, tx_cksum;
    char cksum_buf[2];
    int status;

    assert(buf && len);
    length = 0; cksum = 0;

    // read 'til we find packet start
    while (1) {
        c = gdb_getc(state);
        if (c == EOF) {
            GDB_ERROR("EOF while waiting for packet\n");
            return EOF;     // communication error
        }
        else if (c == 0x03) {
            return 0x03;    // Ctrl+C
        }
        else if (c == '+') {
            gdb_handle_ack(state);
            continue;       // stray ACK
        }
        else if (c == '-') {
            gdb_handle_nack(state);
            continue;       // stray NACK
        }
        else if (c == '$') {
            break;          // packet start
        }
        else if (c == 0) {
            continue;       // timeout
        }
        else {              // ignore everything else 'til we see packet start
            if (isprint(c)) {
                GDB_PRINT("expecting '$', got '%c'\n", c);
            }
            else {
                GDB_PRINT("expecting '$', got \\x%02hhx\n", c);
            }
        }
    }

    // read-in packet data
    while (1) {
        c = gdb_getc(state);
        if (c == EOF) {
            GDB_ERROR("EOF while reading packet data\n");
            return EOF;
        }
        else if (c == '#') {
            break;      // checksum start
        }
        else if (c == 0) {
            continue;   // timeout
        }
        if (length >= (bufsiz-1) || length >= (GDB_MAXLEN-1)) {
            GDB_ERROR("recv packet buffer overflow!\n");
            return EOF;
        }
        if (buf) {
            buf[(length)++] = c;
        }
        cksum += c;
    }
    buf[length] = '\0';

    // read-in transmitted checksum
    for (int i = 0; i < 2; i++) {
        c = gdb_getc(state);
        if (c == EOF) {
            GDB_ERROR("EOF while reading packet checksum\n");
            return EOF;
        }
        else if (c == 0) {
            continue;   // timeout
        }
        cksum_buf[i] = c;
    }
    status = decode_hex(cksum_buf, sizeof(cksum_buf), &tx_cksum, sizeof(tx_cksum));
    if (status == EOF) {
        return EOF;
    }

    GDB_PRINT("-> $%.*s#%02x\n", *len, buf, cksum);
    state->rxpkt_count++;

    // verify checksum
    if (cksum != tx_cksum) {
        GDB_ERROR("checksum: expecting %02hhx, got %02hhx\n", tx_cksum, cksum);
        return gdb_send_nack(state);
    }

    *len = length;
    return gdb_send_ack(state);
}

static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len)
{
    uint8_t cksum;
    char cksum_buf[2];
    int status;
    int i;

    if (!buf && len > 0) {
        GDB_ERROR("user provided NULL buffer!\n");
        return EOF;
    }

    if (len > (GDB_MAXLEN-1)) {
        GDB_ERROR("send packet buffer overflow!\n");
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

    state->txpkt_count++;
    return gdb_recv_ack(state);
}

static int gdb_send_ok_packet(struct gdb_state *state)
{
    return gdb_send_packet(state, "OK", 2);
}

static int gdb_send_empty_packet(struct gdb_state *state)
{
    return gdb_send_packet(state, NULL, 0);
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

static int gdb_send_ack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("<- +\n");
#endif

    gdb_putc(state, '+');
    return 0;
}

static int gdb_send_nack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("<- -\n");
#endif

    gdb_putc(state, '-');
    return 0;
}

static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len)
{
    uint8_t c;

    assert(buf || len);

    if ((len*2) > bufsiz) {
        GDB_ERROR("encode_hex: payload too large!\n");
        return EOF;
    }

    for (int i = 0; i < len; i++) {
        c = ((uint8_t *) data)[i];
        *buf++ = toxdigit(c >> 4);
        *buf++ = toxdigit(c & 0xF);
    }

    return len * 2;
}

static int decode_hex(const char *buf, size_t bufsiz, void *data, size_t len)
{
    uint8_t *p;
    char c;

    assert(buf && data);

    if ((len*2) > bufsiz) {
        GDB_ERROR("decode_hex: payload too large!\n");
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
    char c;
    int timeout;

    if (state->pending_char) {
        c = state->pending_char;
        state->pending_char = 0;
        return c;
    }

    c = 0;
    timeout = 1000000;  // TODO: make this a real timer
    while (timeout > 0 && (com_in(state->com, UART_LSR) & UART_LSR_DR) == 0) {
        timeout--;
    }
    if (timeout > 0) {
        c = com_in(state->com, UART_RX);
    }

    return c;
}

static char gdb_putc(struct gdb_state *state, char c)
{
    while ((com_in(state->com, UART_LSR) & UART_LSR_THRE) == 0);
    com_out(state->com, UART_TX, c);
    return c;
}
