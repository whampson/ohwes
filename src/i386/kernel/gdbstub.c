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
 * https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html
 *
 * Inspired by https://github.com/mborgerson/gdbstub.
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i386/bitops.h>
#include <i386/debug.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/serial.h>
#include <sys/ohwes.h>

#define GDB_NACK_THRESH     10
#define GDB_BUFLEN          512
#define GDB_IO_POLLCNT      1000000
#define GDB_IO_MAX_TIMEOUT  1000

#define ENABLE_GDB_PRINT    0
#define ENABLE_GDB_ERROR    0
#define PRINT_ACKS          0

#define GDB_RECV_PENDING    (-2)

#if ENABLE_GDB_PRINT
#define GDB_PRINT(...)  pr_info(KLOG_INFO "gdb: " __VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

#if ENABLE_GDB_ERROR
#define GDB_ERROR(...)  pr_alert("gdb: error: " __VA_ARGS__)
#else
#define GDB_ERROR(...)
#endif

enum gdb_i386_regs { // do not change the order!
    GDB_REG_I386_EAX = 0,
    GDB_REG_I386_ECX = 1,
    GDB_REG_I386_EDX = 2,
    GDB_REG_I386_EBX = 3,
    GDB_REG_I386_ESP = 4,
    GDB_REG_I386_EBP = 5,
    GDB_REG_I386_ESI = 6,
    GDB_REG_I386_EDI = 7,
    GDB_REG_I386_EIP = 8,
    GDB_REG_I386_EFLAGS = 9,
    GDB_REG_I386_CS = 10,
    GDB_REG_I386_SS = 11,
    GDB_REG_I386_DS = 12,
    GDB_REG_I386_ES = 13,
    GDB_REG_I386_FS = 14,
    GDB_REG_I386_GS = 15,
    GDB_NUM_I386_REGS,
};
typedef uint32_t i386_reg;

struct gdb_state {
    int signum;                         // break signal number
    i386_reg regs[GDB_NUM_I386_REGS];   // register shadow

    char tx_buf[GDB_BUFLEN];            // last packet transmitted
    size_t tx_len;                      // length of last packet transmitted

    uint32_t ack_count;                 // number of ACKs seen
    uint32_t nack_count;                // number of NACKs seen
    uint32_t error_count;               // number of error packets sent
    uint32_t txpkt_count;               // GDB packets sent
    uint32_t rxpkt_count;               // GDB packets received

    struct com *com;                    // COM port state at time of entry
    int pending_char;                   // pending character to be read
    bool has_pending_char;              // pending_char is valid
    bool interrupt_pending;             // CTRL+C received, deliver on next resume
};

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
static void gdb_report_break(struct gdb_state *state);
static int gdb_recv_ack(struct gdb_state *state);
static int gdb_recv_packet(struct gdb_state *state, char *buf, size_t bufsiz, size_t *len);
static int gdb_send_packet_raw(struct gdb_state *state, const char *buf, size_t len);
static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len);
static int gdb_send_ok(struct gdb_state *state);
static int gdb_send_empty(struct gdb_state *state);
static int gdb_send_signal(struct gdb_state *state, int signal);
static int gdb_send_signal_noack(struct gdb_state *state, int signal);
static int gdb_send_error(struct gdb_state *state, int error);
static int gdb_send_ack(struct gdb_state *state);
static int gdb_send_nack(struct gdb_state *state);
static int gdb_handle_ack(struct gdb_state *state);
static int gdb_handle_nack(struct gdb_state *state);

static void gdb_disable_com_int(struct gdb_state *state);
static void gdb_enable_com_int(struct gdb_state *state);

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

static bool gdb_range_valid(uintptr_t addr, size_t count, bool needs_write);

// basic get/put functions
static int gdb_getc(struct gdb_state *state);
static int gdb_putc(struct gdb_state *state, char c);

extern __noreturn void __hard_reset(void);

#define GDB_PUTC_EOF_CHECK(state, c) \
do { \
    if (gdb_putc(state, c) == EOF) { \
        return EOF; \
    } \
} while (0)

static void gdb_capture(struct gdb_state *state, const struct iregs *regs)
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
}

static void gdb_apply(struct gdb_state *state, struct iregs *regs)
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

static volatile uint32_t s_debugging = false;

int gdb_main(struct iregs *regs, bool from_com)
{
    size_t len;
    int status;
    char pkt[GDB_BUFLEN];
    uint32_t flags;
    bool ctrl_c;
    bool frame_start;

    static struct gdb_state s_state;
    struct gdb_state *state = &s_state;

    // reentrancy guard
    //
    // CAUTION: do not perform brain surgery on yourself! this guard only
    // protects against state corruption; it does not guard against live-locks
    // caused by single-steps (int1) or breakpoints (int3). therefore,
    // single-stepping through this code is not supported unless using a
    // different stub!!
    if (test_and_set_bit(&s_debugging, 0)) {
        pr_alert("gdb_main: reentry!\n");
        return 0;
    }
    cli_save(flags);

    // zero state
    status = 0;
    zeromem(state, sizeof(struct gdb_state));   // TODO: preserve statistics

    // sanity check
    if (!regs) {
        pr_fatal("gdb_main: regs are NULL!\n");
        goto gdb_done;  // releases lock, restores flags
    }

    // work-out COM port info
    state->com = get_com(get_com_num(SERIAL_DEBUG_PORT));
    if (!state->com) {
        pr_fatal("gdb_main: com is NULL!\n");
        goto gdb_done;
    }
    if (!state->com->io_port) {
        state->com->io_port = SERIAL_DEBUG_PORT;
    }
    assert(state->com->io_port == SERIAL_DEBUG_PORT);

    // deal with any pending COM chars
    if (from_com) {
        if (com_in(state->com, UART_LSR) & UART_LSR_DR) {
            state->pending_char = com_in(state->com, UART_RX);
            state->has_pending_char = true;
        }
    }
    ctrl_c = (state->has_pending_char && state->pending_char == 0x03);
    frame_start = (state->has_pending_char && state->pending_char == '$');

    // if we got here via COM interrupt and we aren't at a packet start,
    // just exit until we see the start of a frame or CTRL+C
    if (from_com && !frame_start && !ctrl_c) {
        gdb_enable_com_int(state);  // ensure we can receive new characters
        goto gdb_done;              // releases lock
    }

    // capture register state
    gdb_capture(state, regs);

    // disable debug COM interrupts
    // note: reenabled on 'continue' and 'step', early exit due to EOF, and
    //       upon entry to gdb_main with unexpected start character
    gdb_disable_com_int(state);

    // check entry conditions
    if (from_com && ctrl_c) {
        GDB_PRINT("recv: -> CTRL+C (com)\n");
        (void) gdb_getc(state);     // consume CTRL+C (clear pending_char)
        status = gdb_send_signal_noack(state, SIGINT);    // notify GDB host
    }
    else if (from_com && frame_start) {
        status = 0;                 // '$' pending; recv_packet() consumes it
    }
    else {
        gdb_report_break(state);    // breakpoint/single-step/debug-break
        status = 0;                 // proceed into the command loop
    }

    // main command loop
    while (status != EOF) {
        status = gdb_recv_packet(state, pkt, sizeof(pkt), &len);
        if (status == EOF) {
            break;      // timeout; break-out
        }
        else if (status == 0x03) {
            GDB_PRINT("recv: -> CTRL+C (cmd-loop)\n");
            // status = gdb_send_signal_noack(state, SIGINT);
            state->interrupt_pending = true;
            continue;       // latch; deliver on next resume command
            // continue;   // CTRL+C received; stay stopped, wait for GDB's next command
        }
        else if (len == 0) {
            continue;   // empty packet; stay stopped, wait for GDB's next command
        }

        // TODO:
        // 'p/P' - read/write io register?
        // '!' (extended mode) and 'R' - reboot and keep server persistent
        //   or 'qRcmd' and `monitor reboot'

        switch (pkt[0]) {
            case '?':   // query signal state
                status = gdb_send_signal(state, state->signum);
                break;
            case 'k':   // kill - reset
                __hard_reset();
                break;
            case 'g':   // read regs
                status = gdb_read_regs(state);
                break;
            case 'G':   // write regs
                status = gdb_write_regs(state, &pkt[1], len-1);
                break;
            case 'm':   // read memory
                status = gdb_read_mem(state, &pkt[1], len-1);
                break;
            case 'M':   // write memory
                status = gdb_write_mem(state, &pkt[1], len-1);
                break;
            case 's':   // single-step
                if (state->interrupt_pending) {
                    state->interrupt_pending = false;
                    status = gdb_send_signal(state, SIGINT);
                    break;  // stay stopped; deliver pending interrupt
                }
                gdb_step(state);
                goto gdb_cleanup;
            case 'c':   // continue
                if (state->interrupt_pending) {
                    state->interrupt_pending = false;
                    status = gdb_send_signal(state, SIGINT);
                    break;  // stay stopped; deliver pending interrupt
                }
                gdb_continue(state);
                goto gdb_cleanup;
            case 'D':   // detach
                gdb_detach(state);
                goto gdb_cleanup;

            case '+':
                status = gdb_handle_ack(state);
                break;
            case '-':
                status = gdb_handle_nack(state);
                break;
            default:
                status = gdb_send_empty(state);
                break;
        }
    }

    if (status == EOF) {
        GDB_ERROR("EOF!\n");
        status = gdb_send_error(state, GDB_EUNKNOWN);   // communication error
        gdb_enable_com_int(state);  // ensure we can receive new characters

    }

gdb_cleanup:
    gdb_apply(state, regs);     // apply register state
gdb_done:
    clear_bit(&s_debugging, 0);
    restore_flags(flags);
    return status;
}

static void gdb_step(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] |= EFLAGS_TF;
    gdb_enable_com_int(state);
}

static void gdb_continue(struct gdb_state *state)
{
    state->regs[GDB_REG_I386_EFLAGS] &= ~EFLAGS_TF;
    gdb_enable_com_int(state);
}

static void gdb_detach(struct gdb_state *state)
{
    gdb_send_ok(state);
    gdb_continue(state);
}

static int gdb_read_regs(struct gdb_state *state)
{
    const size_t bufsiz = (sizeof(i386_reg) * GDB_NUM_I386_REGS) * 2;
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

    return gdb_send_ok(state);
}

static int gdb_read_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    uintptr_t addr;
    size_t count;
    char data[GDB_BUFLEN/2] = { };
    char tx_pkt[GDB_BUFLEN] = { };
    char *p;

    errno = 0;
    addr = (uintptr_t) strtoul(pkt, &p, 16);
    if (errno == ERANGE || *p != ',') {
        GDB_ERROR("read_mem: bad packet format (addr)\n");
        return EOF;
    }

    errno = 0;
    count = strtoul(p+1, &p, 16);
    if (errno == ERANGE || *p != '\0') {
        GDB_ERROR("read_mem: bad packet format (count)\n");
        return EOF;
    }
    if (count > sizeof(data)) {
        GDB_ERROR("read_mem: too many bytes requested!\n");
        return EOF;
    }

    if (count == 0 || (addr + count - 1) < addr ||
        !gdb_range_valid(addr, count, /*needs_write=*/false)) {
        return gdb_send_error(state, GDB_EFAULT);
    }

    memcpy(data, (void *) addr, count);
    return gdb_send_packet(state, tx_pkt, encode_hex(tx_pkt,
        sizeof(tx_pkt), data, count));
}

static int gdb_write_mem(struct gdb_state *state, char *pkt, size_t pktlen)
{
    int status;
    uintptr_t addr;
    size_t count;
    char data[GDB_BUFLEN/2] = { };
    char *p;

    errno = 0;
    addr = (uintptr_t) strtoul(pkt, &p, 16);
    if (errno == ERANGE || *p != ',') {
        GDB_ERROR("write_mem: bad packet format (addr)\n");
        return EOF;
    }

    errno = 0;
    count = strtoul(p+1, &p, 16);
    if (errno == ERANGE || *p != ':') {
        GDB_ERROR("write_mem: bad packet format (count)\n");
        return EOF;
    }
    if (count > sizeof(data)) {
        GDB_ERROR("write_mem: too many bytes requested\n");
        return EOF;
    }

    if (count == 0 || (addr + count - 1) < addr ||
        !gdb_range_valid(addr, count, /*needs_write=*/true)) {
        return gdb_send_error(state, GDB_EFAULT);
    }

    status = decode_hex(p+1, (size_t) pktlen - (p+1 - pkt), data, count);
    if (status == EOF) {
        return EOF;     // payload too large
    }

    memcpy((void *) addr, data, count);
    return gdb_send_ok(state);
}

static bool gdb_range_valid(uintptr_t addr, size_t count, bool needs_write)
{
    pte_t *pte;
    uintptr_t end = addr + count;

    for (uintptr_t va = addr & PAGE_MASK; va < end; va += PAGE_SIZE) {
        if (!walk_page_table(va, &pte)) {
            return false;
        }
        if (!pte_present(*pte)) {
            return false;
        }
        if (needs_write && !pte_write(*pte)) {
            return false;
        }
    }

    return true;
}

static void gdb_report_break(struct gdb_state *state)
{
    // client-initiated break
    while (true) {
        int status = gdb_send_signal(state, SIGTRAP);
        if (status == 0) {
            return;                 // ACK received, debugger connected
        }
        else if (state->has_pending_char && state->pending_char == '$') {
            return;                 // GDB sent a packet; recv_packet() consumes '$'
        }
        else if (state->has_pending_char && state->pending_char == 0x03) {
            GDB_PRINT("recv: -> CTRL+C (break)\n");
            (void) gdb_getc(state); // consume stale CTRL+C, retry
            // (void) gdb_send_signal_noack(state, SIGINT);
            state->interrupt_pending = true;
            return;
        }
        else if (status == GDB_RECV_PENDING) {
            (void) gdb_getc(state); // consume unexpected char, retry
        }
        // else: timeout (EOF), retry
    }
}

static int gdb_recv_ack(struct gdb_state *state)
{
    int c;

    c = gdb_getc(state);
    if (c == EOF) {
        return EOF; // timeout
    }
    switch (c) {
        case '+': return gdb_handle_ack(state);
        case '-': return gdb_handle_nack(state);
    }

    if (isprint(c)) {
        GDB_PRINT("recv: expecting '+' or '-', got '%c'\n", c);
    }
    else {
        GDB_PRINT("recv: expecting '+' or '-', got \\x%02hhx\n", c);
    }

    state->pending_char = c;    // got something unexpected, cache it
    state->has_pending_char = true;
    return GDB_RECV_PENDING;
}

static int gdb_handle_ack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("recv: -> +\n");
#endif

    state->ack_count++;
    return 0;
}

static int gdb_handle_nack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("recv: -> - (NACK)\n");
#endif

    state->nack_count++;
    if ((state->nack_count % GDB_NACK_THRESH) == 0) {
        GDB_ERROR("received %lu NACKs, what gives??\n", state->nack_count);
    }

    // ignore NACK received without first having sent packet
    if (state->tx_len == 0) {
        return 0;
    }

    // retransmit without waiting for ACK to prevent a ping-pong loop;
    // host's next ACK/NACK handled by next recv call
    return gdb_send_packet_raw(state, state->tx_buf, state->tx_len);
}

static int gdb_recv_packet(struct gdb_state *state,
    char *buf, size_t bufsiz, size_t *len)
{
    // packet formats:
    //   $packet-data#checksum
    //   $sequence-id:packet-data#checksum
    // sequence-id should never appear in packets transmitted by GDB host

    int c;
    size_t length;
    uint8_t cksum, tx_cksum;
    char cksum_buf[2];
    int status;
    int timeout_count;

    assert(buf && len);

    #define HANDLE_TIMEOUT(x) \
    do { \
        if (++(x) >= GDB_IO_MAX_TIMEOUT) { \
            GDB_ERROR("recv: host disconnected (timeout)\n"); \
            return EOF; \
        } \
    } while (0)

    while (true) {
        length = 0; cksum = 0;
        timeout_count = 0;

        // read 'til we find packet start
        while (true) {
            c = gdb_getc(state);
            if (c == EOF) {
                HANDLE_TIMEOUT(timeout_count);
                continue;
            }
            timeout_count = 0;

            if (c == 0x03) {
                GDB_PRINT("recv: -> CTRL+C (recv-start)\n");
                return 0x03;    // Ctrl+C
            }
            else if (c == '+') {
                gdb_handle_ack(state);
                continue;       // stray ACK
            }
            else if (c == '-') {
                state->nack_count++;
                continue;       // stray NACK, ignore
            }
            else if (c == '$') {
                break;          // packet start
            }
            else {              // ignore everything else 'til we see packet start
                if (isprint(c)) {
                    GDB_PRINT("recv: expecting '$', got '%c'\n", c);
                }
                else {
                    GDB_PRINT("recv: expecting '$', got \\x%02hhx\n", c);
                }
            }
        }

        // read-in packet data
        while (1) {
            c = gdb_getc(state);
            if (c == EOF) {
                HANDLE_TIMEOUT(timeout_count);
                continue;
            }
            timeout_count = 0;

            if (c == 0x03) {
                GDB_PRINT("recv: -> CTRL+C (recv-data)\n");
                return 0x03;    // CTRL+C
            }
            else if (c == '#') {
                break;          // checksum start
            }

            if (length >= (bufsiz-1) || length >= (GDB_BUFLEN-1)) {
                GDB_ERROR("recv: packet buffer overflow!\n");
                return EOF;
            }
            buf[(length)++] = c;
            cksum += c;
        }
        buf[length] = '\0';

        // read-in transmitted checksum
        int i = 0;
        while (i < 2) {
            c = gdb_getc(state);
            if (c == EOF) {
                HANDLE_TIMEOUT(timeout_count);
                continue;
            }
            timeout_count = 0;

            if (c == 0x03) {
                GDB_PRINT("recv: -> CTRL+C (recv-cksum)\n");
                return 0x03;    // Ctrl+C
            }
            cksum_buf[i++] = c;
        }
        status = decode_hex(cksum_buf, sizeof(cksum_buf), &tx_cksum, sizeof(tx_cksum));
        if (status == EOF) {
            (void) gdb_send_nack(state);    // send NACK and retry
            continue;
        }

        GDB_PRINT("recv: -> $%.*s#%02x\n", (int) length, buf, cksum);
        state->rxpkt_count++;

        // verify checksum
        if (cksum != tx_cksum) {
            GDB_ERROR("recv: checksum: expecting %02hhx, got %02hhx\n", tx_cksum, cksum);
            (void) gdb_send_nack(state);
            continue;   // retry, wait for host retransmit
        }

        *len = length;
        return gdb_send_ack(state);
    }

    #undef HANDLE_TIMEOUT
}

static int gdb_send_packet_raw(struct gdb_state *state, const char *buf, size_t len)
{
    uint8_t cksum;
    char cksum_buf[2];
    char frame[GDB_BUFLEN + 4]; // '$' + data + '#' + 2 cksum bytes
    size_t frame_len;
    int status;
    int i;

    if (!buf && len > 0) {
        GDB_ERROR("send: user provided NULL buffer!\n");
        return EOF;
    }

    if (len > (GDB_BUFLEN-1)) {
        GDB_ERROR("send: packet buffer overflow!\n");
        return EOF;
    }

    frame[0] = '$';
    for (cksum = 0, i = 0; i < len; i++) {
        frame[1 + i] = buf[i];
        cksum += (uint8_t) buf[i];
    }

    status = encode_hex(cksum_buf, sizeof(cksum_buf), &cksum, 1);
    if (status == EOF) {
        return EOF;
    }

    frame[1 + len] = '#';
    frame[2 + len] = cksum_buf[0];
    frame[3 + len] = cksum_buf[1];
    frame_len = 4 + len;

    // transmit whole frame
    for (i = 0; i < frame_len; i++) {
        GDB_PUTC_EOF_CHECK(state, frame[i]);
    }

    // commit only after full success
    if (len > 0) {
        memcpy(state->tx_buf, buf, len);
    }
    state->tx_buf[len] = '\0';
    state->tx_len = len;

    GDB_PRINT("send: <- $%.*s#%c%c\n",
        (int) state->tx_len, state->tx_buf, cksum_buf[0], cksum_buf[1]);

    state->txpkt_count++;
    return 0;
}

static int gdb_send_packet(struct gdb_state *state, const char *buf, size_t len)
{
    int status = gdb_send_packet_raw(state, buf, len);
    if (status == EOF) {
        return EOF;
    }
    return gdb_recv_ack(state);
}

static int gdb_send_ok(struct gdb_state *state)
{
    return gdb_send_packet(state, "OK", 2);
}

static int gdb_send_empty(struct gdb_state *state)
{
    return gdb_send_packet(state, NULL, 0);
}

static int gdb_send_signal(struct gdb_state *state, int signal)
{
    char buf[8];
    uint8_t sig = (uint8_t) signal;
    size_t len;
    int status;

    buf[0] = 'S';
    status = encode_hex(buf+1, sizeof(buf)-1, &sig, 1);
    if (status == EOF) {
        return EOF;
    }

    len = status + 1;
    state->signum = sig;
    return gdb_send_packet(state, buf, len);
}

static int gdb_send_signal_noack(struct gdb_state *state, int signal)
{
    char buf[8];
    uint8_t sig = (uint8_t) signal;
    size_t len;
    int status;

    buf[0] = 'S';
    status = encode_hex(buf+1, sizeof(buf)-1, &sig, 1);
    if (status == EOF) {
        return EOF;
    }

    len = status + 1;
    state->signum = sig;
    return gdb_send_packet_raw(state, buf, len);
}

static int gdb_send_error(struct gdb_state *state, int error)
{
    char buf[8];
    uint8_t err = (uint8_t) error;
    size_t len;
    int status;

    buf[0] = 'E';
    status = encode_hex(buf+1, sizeof(buf)-1, &err, 1);
    if (status == EOF) {
        return EOF;
    }
    state->error_count++;

    len = status + 1;
    return gdb_send_packet(state, buf, len);
}

static int gdb_send_ack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("send: <- +\n");
#endif

    GDB_PUTC_EOF_CHECK(state, '+');
    return 0;
}

static int gdb_send_nack(struct gdb_state *state)
{
#if PRINT_ACKS
    GDB_PRINT("send: <- -\n");
#endif

    GDB_PUTC_EOF_CHECK(state, '-');
    return 0;
}

static int encode_hex(char *buf, size_t bufsiz, const void *data, size_t len)
{
    uint8_t c;

    assert(buf || len == 0);

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
            else {
                return EOF; // invalid hex digit
            }
        }
    }

    return 0;
}

static int gdb_getc(struct gdb_state *state)
{
    int c;
    int timeout;

    if (state->has_pending_char) {
        c = state->pending_char;
        state->has_pending_char = false;
        return c;
    }

    c = 0;
    timeout = GDB_IO_POLLCNT;  // TODO: real timer?
    while (timeout > 0 && (com_in(state->com, UART_LSR) & UART_LSR_DR) == 0) {
        timeout--;
    }
    if (timeout > 0) {
        return com_in(state->com, UART_RX);
    }

    return EOF;     // timeout
}

static int gdb_putc(struct gdb_state *state, char c)
{
    int timeout = GDB_IO_POLLCNT;  // TODO: real timer?
    while (timeout > 0 && (com_in(state->com, UART_LSR) & UART_LSR_THRE) == 0) {
        timeout--;
    }
    if (timeout > 0) {
        com_out(state->com, UART_TX, c);
        return c;
    }

    return EOF;     // timeout
}

static void gdb_disable_com_int(struct gdb_state *state)
{
    state->com->ier._value = com_in(state->com, UART_IER);
    state->com->ier._value &= ~UART_IER_RDA;
    com_out(state->com, UART_IER, state->com->ier._value);
}

static void gdb_enable_com_int(struct gdb_state *state)
{
    state->com->ier._value = com_in(state->com, UART_IER);
    state->com->ier._value |= UART_IER_RDA;
    com_out(state->com, UART_IER, state->com->ier._value);
}
