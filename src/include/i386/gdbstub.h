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
 *         File: include/kernel/gdbstub.h
 *      Created: April 10, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __GDBSTUB_H
#define __GDBSTUB_H

#include <stddef.h>
#include <stdint.h>
#include <i386/interrupt.h>

#define GDB_MAXLEN          512

typedef uint32_t            gdb_i386_reg;

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

enum gdb_signals {
    GDB_SIGINT  = 2,
    GDB_SIGTRAP = 5,
    GDB_SIGEMT  = 7,
};

struct gdb_state {
    int signum;                             // break signal number
    gdb_i386_reg regs[GDB_NUM_I386_REGS];   // register shadow
    char tx_buf[GDB_MAXLEN];                // last packet transmitted
    size_t tx_len;                          // length of last packet transmitted
    size_t nack_count;                      // number of NACKs seen in a row
};

// initialize state struct
int gdb_init_state(struct gdb_state *state, int signum, const struct iregs *regs);

// main debugging loop
int gdb_main(struct gdb_state *state);

#endif // __GDBSTUB_H
