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
 *         File: i386/kernel/debug.c
 *      Created: April 24, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/cpu.h>
#include <i386/gdbstub.h>
#include <i386/io.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/config.h>
#include <kernel/ohwes.h>
#include <kernel/kernel.h>

void dump_regs(const struct iregs *regs, uint32_t esp, uint16_t ss)
{
    struct eflags flags;

    kprint(
        "eax=%08x ebx=%08x ecx=%08x edx=%08x\n"
        "esp=%08x ebp=%08x esi=%08x edi=%08x\n"
        "eip=%08x cs=%02x ds=%02x es=%02x fs=%02x gs=%02x ss=%02x\n"
        "eflags=%08x [ ",
        regs->eax, regs->ebx, regs->ecx, regs->edx,
        esp, regs->ebp, regs->esi, regs->edi,
        regs->eip, regs->cs, regs->ds, regs->es, regs->fs, regs->gs, ss,
        regs->eflags);

    flags._value = regs->eflags;
    if (flags.id)   kprint("id ");
    if (flags.vip)  kprint("vip ");
    if (flags.vif)  kprint("vif ");
    if (flags.ac)   kprint("ac ");
    if (flags.vm)   kprint("vm ");
    if (flags.rf)   kprint("rf ");
    if (flags.nt)   kprint("nt ");
    kprint("iopl=%d ", flags.iopl);
    if (flags.of)   kprint("of ");
    if (flags.df)   kprint("df ");
    if (flags.intf) kprint("if ");
    if (flags.tf)   kprint("tf ");
    if (flags.sf)   kprint("sf ");
    if (flags.zf)   kprint("zf ");
    if (flags.af)   kprint("af ");
    if (flags.pf)   kprint("pf ");
    if (flags.cf)   kprint("cf ");
    kprint("]\n");
}

__fastcall void handle_breakpoint(struct iregs *regs)
{
    // const char *brk_name = (regs->vec == 3)
    //     ? "BREAKPOINT"
    //     : "HW_BREAKPOINT";

    // kprint("\e[1;33m*** %s BEGIN ***\n", brk_name);
    // dump_regs(regs, esp, ss);
    // kprint("\e[0m");

    struct gdb_state state = { };
    state.signum = GDB_SIGTRAP;
    state.regs[GDB_REG_I386_EBX] = regs->ebx;
    state.regs[GDB_REG_I386_ECX] = regs->ecx;
    state.regs[GDB_REG_I386_EDX] = regs->edx;
    state.regs[GDB_REG_I386_ESI] = regs->esi;
    state.regs[GDB_REG_I386_EDI] = regs->edi;
    state.regs[GDB_REG_I386_EBP] = regs->ebp;
    state.regs[GDB_REG_I386_EAX] = regs->eax;
    state.regs[GDB_REG_I386_DS ] = regs->ds;
    state.regs[GDB_REG_I386_ES ] = regs->es;
    state.regs[GDB_REG_I386_FS ] = regs->fs;
    state.regs[GDB_REG_I386_GS ] = regs->gs;
    state.regs[GDB_REG_I386_EIP] = regs->eip;
    state.regs[GDB_REG_I386_CS ] = regs->cs;
    state.regs[GDB_REG_I386_EFLAGS] = regs->eflags;
    state.regs[GDB_REG_I386_ESP] = regs->esp;
    state.regs[GDB_REG_I386_SS ] = regs->ss;

#if SERIAL_DEBUGGING
    gdb_main(&state);
#else
    for (;;);
#endif

    regs->ebx = state.regs[GDB_REG_I386_EBX];
    regs->ecx = state.regs[GDB_REG_I386_ECX];
    regs->edx = state.regs[GDB_REG_I386_EDX];
    regs->esi = state.regs[GDB_REG_I386_ESI];
    regs->edi = state.regs[GDB_REG_I386_EDI];
    regs->ebp = state.regs[GDB_REG_I386_EBP];
    regs->eax = state.regs[GDB_REG_I386_EAX];
    regs->ds  = state.regs[GDB_REG_I386_DS ];
    regs->es  = state.regs[GDB_REG_I386_ES ];
    regs->fs  = state.regs[GDB_REG_I386_FS ];
    regs->gs  = state.regs[GDB_REG_I386_GS ];
    regs->eip = state.regs[GDB_REG_I386_EIP];
    regs->cs  = state.regs[GDB_REG_I386_CS ];
    regs->eflags = state.regs[GDB_REG_I386_EFLAGS];
    regs->esp = state.regs[GDB_REG_I386_ESP];
    regs->ss  = state.regs[GDB_REG_I386_SS ];

    // kprint("\e[1;33m");
    // dump_regs(regs, esp, ss);
    // kprint("*** %s END ***\e[0m\n", brk_name);
}
