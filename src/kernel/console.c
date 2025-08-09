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
 *         File: kernel/console.c
 *      Created: April 28, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/fs.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>
#include <kernel/vga.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/queue.h>

#define KPRINT_MAX          1024

static int _log_start = 0;
static int _log_size = 0;
static char *_kernel_log = (char *) __klog;

struct console *g_consoles = NULL;
extern bool g_kb_initialized;

void register_console(struct console *cons)
{
    const char *log_ptr;
    struct console *currcons;

    assert(cons);

    if (!has_console()) {
        g_consoles = cons;
        cons->next = NULL;
    }
    else {
        currcons = g_consoles;
        if (cons == currcons) {
            return; // already registered
        }

        while (currcons->next) {
            currcons = currcons->next;
            if (currcons == cons) {
                return; // already registered
            }
        }

        currcons->next = cons;
        cons->next = NULL;
    }

    if (cons->setup) {
        cons->setup(cons);
    }
    if (cons->write) {
        log_ptr = &_kernel_log[_log_start];
        if (_log_size < KERNEL_LOG_SIZE) {
            cons->write(cons, log_ptr, _log_size);
        }
        else {
            cons->write(cons, log_ptr, KERNEL_LOG_SIZE - _log_start);
            cons->write(cons, _kernel_log, _log_start);
        }
    }
}

void unregister_console(struct console *cons)
{
    struct console *currcons;
    struct console *prevcons;

    assert(cons);

    if (g_consoles == cons) {
        g_consoles = g_consoles->next;
        cons->next = NULL;
        return;
    }

    currcons = g_consoles;
    while (currcons->next) {
        prevcons = currcons;
        currcons = currcons->next;
        if (currcons == cons) {
            prevcons->next = currcons->next;
            currcons = NULL;
            break;
        }
    }
}

bool has_console()
{
    return g_consoles != NULL;
}

static void register_default_console(void)
{
    extern struct console vt_console;
    register_console(&vt_console);
}

// print a message to the console(s) and kernel log
int console_write(const char *buf, size_t count)
{
    const char *p;
    const char *line;
    int linefeed;
    struct console *cons;

#if EARLY_PRINT
    // ensure a console is registered
    if (!has_console()) {
        register_default_console();
    }
#endif

    if (count > KPRINT_MAX) {
        count = KPRINT_MAX;
    }

    p = buf;
    linefeed = 0;
    while ((p - buf) < count && *p != '\0') {
        // add to the log 'til we see a linefeed or hit the end
        for (line = p; (p - buf) < count && *p != '\0'; p++) {
#if E9_HACK
            outb(0xE9, *p);
#endif
            _kernel_log[(_log_start + _log_size) % KERNEL_LOG_SIZE] = *p;
            if (_log_size < KERNEL_LOG_SIZE) {
                _log_size++;
            }
            else {
                _log_start += 1;
                _log_start %= KERNEL_LOG_SIZE;
            }
            linefeed = (*p == '\n');
            if (linefeed) {
                break;
            }
        }

        // write line to console
        cons = g_consoles;
        while (cons) {
            if (cons->write) {
                cons->write(cons, line, (p - line) + linefeed);
            }
            cons = cons->next;
        }

        if (linefeed) {
            linefeed = 0;
            p++;
        }
    }

    return (p - buf);
}

int console_getc(void)
{
    if (!has_console() && !g_consoles->getc) {
        return '\0';
    }

    return g_consoles->getc(g_consoles);
}

int _vkprint(const char *fmt, va_list args)
{
    size_t count;
    char buf[KPRINT_MAX+1] = { };

    count = vsnprintf(buf, KPRINT_MAX, fmt, args);
    return console_write(buf, count);
}

int kprint(const char *fmt, ...)
{
    va_list args;
    size_t count;

    va_start(args, fmt);
    count =_vkprint(fmt, args);
    va_end(args);

    return count;
}

void __noreturn panic(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    kprint("\n\e[1;31mpanic: "); _vkprint(fmt, args); kprint("\e[0m");
    va_end(args);

    irq_disable();
    irq_setmask(IRQ_MASKALL);

#if SERIAL_DEBUGGING
    if (SERIAL_DEBUG_PORT == COM1_PORT || SERIAL_DEBUG_PORT == COM3_PORT) {
        irq_unmask(IRQ_COM1);
    }
    else {
        irq_unmask(IRQ_COM2);
    }
#endif

    irq_unmask(IRQ_TIMER);
    if (g_kb_initialized) {
        irq_unmask(IRQ_KEYBOARD);
    }

    irq_enable();
    __int3();

    for (;;);
}

#if 0
static void print_log(void)    // TODO: procfs for this
{
    for (int i = 0; i < _log_size; i++) {
        // printf("%c", _kernel_log[(_log_start + i) % _log_size]);
    }
}

static void print_consoles(void)   // TODO: procfs for this
{
    struct console *cons;

    cons = g_consoles;
    while (cons) {
        // printf("%s ", cons->name);
        cons = cons->next;
    }
    // printf("\n");
}
#endif

void print_boot_info(struct boot_info *boot)
{
    int nfloppies = boot->hwflags.has_diskette_drive;
    if (nfloppies) {
        nfloppies += boot->hwflags.num_other_diskette_drives;
    }

    int nserial = boot->hwflags.num_serial_ports;
    int nparallel = boot->hwflags.num_parallel_ports;
    bool gameport = boot->hwflags.has_gameport;
    bool mouse = boot->hwflags.has_ps2mouse;
    uint32_t ebda_size = 0xA0000 - boot->ebda_base;

    kprint("bios-boot: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("bios-boot: A20 mode is %s\n",
        (boot->a20_method == A20_KEYBOARD) ? "A20_KEYBOARD" :
        (boot->a20_method == A20_PORT92) ? "A20_PORT92" :
        (boot->a20_method == A20_BIOS) ? "A20_BIOS" :
        "A20_NONE");
    kprint("bios-boot: %s PS/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("bios-boot: video mode is %02Xh\n", boot->vga_mode & 0x7F);
    if (boot->ebda_base) kprint("bios-boot: EBDA=%08X,%Xh\n", boot->ebda_base, ebda_size);
}

static void print_page_info(uint32_t va, const struct pginfo *page)
{
    uint32_t pa = page->pfn << PAGE_SHIFT;
    uint32_t plimit = pa + PAGE_SIZE - 1;
    uint32_t vlimit = va + PAGE_SIZE - 1;
    if (page->pde) {
        if (page->ps) {
            plimit = pa + PGDIR_SIZE - 1;
        }
        vlimit = va + PGDIR_SIZE - 1;
    }

    //           va-vlimit -> pa-plimit k/M/T rw u/s a/d g wt nc
    kprint("  v(%08X-%08X) -> p(%08X-%08X) %c %-2s %c %c %c %s%s\n",
        va, vlimit, pa, plimit,
        page->pde ? (page->ps ? 'M' : 'T') : 'k',   // (k) small page, (M) large page, (T) page table
        page->rw ? "rw" : "r",                      // read/write
        page->us ? 'u' : 's',                       // user/supervisor
        page->a ? (page->d ? 'd' : 'a') : ' ',      // accessed/dirty
        page->g ? 'g' : ' ',                        // global
        page->pwt ? "wt " : "  ",                   // write-through
        page->pcd ? "nc " : "  ");                  // no-cache
}

void print_page_mappings(void)
{
    struct pginfo *pgdir = (struct pginfo *) get_pgdir();
    struct pginfo *pgtbl;
    struct pginfo *page;
    uint32_t va;

    for (int i = 0; i < PDE_COUNT; i++) {
        page = &pgdir[i];
        if (!page->p) {
            continue;
        }

        va = i << PGDIR_SHIFT;
        print_page_info(va, page);

        if (page->pde && page->ps) {
            continue;   // large
        }

        pgtbl = (struct pginfo *) KERNEL_ADDR(page->pfn << PAGE_SHIFT);
        for (int j = 0; j < PTE_COUNT; j++) {
            page = &pgtbl[j];
            if (!page->p) {
                continue;
            }
            va = (i << PGDIR_SHIFT) | (j << PAGE_SHIFT);
            print_page_info(va, page);
        }
    }
}
