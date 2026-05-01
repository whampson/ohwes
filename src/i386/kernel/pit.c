/* =============================================================================
 * Copyright (C) 2023-2024 Wes Hampson. All Rights Reserved.
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
 *         File: i386/kernel/pit.c
 *      Created: October 18, 2025
 *       Author: Wes Hampson
 *
 * PIT timer implementation.
 *
 * One of the PIT-falls of this approach is that the counter will wrap after
 * about 54ms, so any critical section that takes longer than 54ms will lead to
 * timer inaccuracy. Solution: use the HPET or LAPIC, but those require ACPI :)
 * =============================================================================
 */

// http://www.osdever.net/bkerndev/Docs/pit.htm

#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <i386/x86.h>
#include <kernel/irq.h>
#include <sys/ohwes.h>

//
// reference intervals
//

#define PIT_HZ              1193182 // PIT internal/maximum clock frequency, 1.1931818 MHz
#define QUANTUM_MS          20      // approximate interval between timer ticks, in millis

#define MAX_RELOAD          0xFFFF

//
// i/o ports
//

#define PIT_BASE_PORT       0x40
#define PIT_PORT_CHAN0      (PIT_BASE_PORT+0)
#define PIT_PORT_CHAN1      (PIT_BASE_PORT+1)
#define PIT_PORT_CHAN2      (PIT_BASE_PORT+2)
#define PIT_PORT_CFG        (PIT_BASE_PORT+3)
#define PIT_PORT_PCSPK_EN   0x61

#define _TIMER_PORT(chan)   (PIT_BASE_PORT+((chan)&3))

//
// mode register
//

#define _PIT_MODE(chan, access, mode) \
    ((((chan) & 3) << 6) | (((access) & 3) << 4) | (((mode) & 7) << 1) | 0) // 0 = binary, 1 = BCD

#define CHANNEL_0           0
#define CHANNEL_1           1
#define CHANNEL_2           2

#define ACCESS_LATCH        0
#define ACCESS_LOBYTE       1
#define ACCESS_HIBYTE       2
#define ACCESS_LOHI         3

#define MODE_INTERRUPT      0
#define MODE_ONESHOT        1
#define MODE_RATEGEN        2
#define MODE_SQUAREWAVE     3
#define MODE_SWSTROBE       4
#define MODE_HWSTROBE       5

//
// special mode register programming for readback function
//

#define _PIT_RDBK(mode_mask, chan_mask) \
    (0xC0 | (((~(mode_mask)) & 3) << 4) | ((chan_mask) & 3) << 1)

#define RDBK_STATUS         1
#define RDBK_COUNT          2


//
// current timer state
//

enum hw_timer {
    SCHED_TIMER = CHANNEL_0,
    CLOCK_TIMER = CHANNEL_1,
    PCSPK_TIMER = CHANNEL_2,
    NR_TIMERS = 3
};

struct timer_state {
    uint64_t timer_irqs;
    uint64_t pcspk_ticks_remaining;

    uint64_t timer_ticks;
    uint16_t timer_reloads[NR_TIMERS];

    uint64_t last_clock_tick;
};

static struct timer_state _pit = { };
struct timer_state *g_pit = &_pit;

//
// ----------------------------------------------------------------------------
//

void timer_interrupt(int irq, struct iregs *regs);

static inline void pcspk_on(void)
{
    outb(PIT_PORT_PCSPK_EN, inb(PIT_PORT_PCSPK_EN) | 0x03);
}

static inline void pcspk_off(void)
{
    outb(PIT_PORT_PCSPK_EN, inb(PIT_PORT_PCSPK_EN) & ~0x03);
}

static inline uint64_t ticks_to_ms(uint64_t ticks)
{
    return ticks * QUANTUM_MS;
}

static inline uint16_t calculate_divisor(int freq)
{
    int div = div_round(PIT_HZ, freq);
    if (div > UINT16_MAX) {
        div = UINT16_MAX;
    }
    if (div < 1) {
        div = 1;
    }

    return (uint16_t) div;
}

static inline void reload_timer(enum hw_timer timer, uint16_t value)
{
    outb(_TIMER_PORT(timer), value & 0xFF);
    outb(_TIMER_PORT(timer), value >> 8);
    g_pit->timer_reloads[timer] = value;
}

static inline uint8_t read_timer_status(enum hw_timer timer)
{
    outb(PIT_PORT_CFG, _PIT_RDBK(RDBK_STATUS, 1 << timer));
    return inb(_TIMER_PORT(timer));
}

static inline int read_timer(enum hw_timer timer)
{
    uint16_t count;

    outb(PIT_PORT_CFG, _PIT_MODE(timer, ACCESS_LATCH, 0));
    count  = inb(_TIMER_PORT(timer));
    count |= inb(_TIMER_PORT(timer)) << 8;

    return count;
}

void init_timer(void)
{
    uint8_t mode;

    // interrupt timer setup
    mode = _PIT_MODE(CHANNEL_0, ACCESS_LOHI, MODE_RATEGEN);
    outb(PIT_PORT_CFG, mode);
    reload_timer(SCHED_TIMER, calculate_divisor(div_round(1000, QUANTUM_MS)));

    // clock timer setup
    mode = _PIT_MODE(CHANNEL_1, ACCESS_LOHI, MODE_INTERRUPT);
    outb(PIT_PORT_CFG, mode);
    reload_timer(CLOCK_TIMER, MAX_RELOAD);

    // PC speaker setup
    mode = _PIT_MODE(CHANNEL_2, ACCESS_LOHI, MODE_SQUAREWAVE);
    outb(PIT_PORT_CFG, mode);
    reload_timer(PCSPK_TIMER, MAX_RELOAD);

    irq_register(IRQ_TIMER, timer_interrupt);
    irq_unmask(IRQ_TIMER);
}

void timer_interrupt(int irq, struct iregs *regs)
{
    assert(irq == IRQ_TIMER);

    g_pit->timer_irqs++;
    g_pit->last_clock_tick = read_timer(CLOCK_TIMER);

    if (g_pit->pcspk_ticks_remaining) {
        g_pit->pcspk_ticks_remaining--;
        if (!g_pit->pcspk_ticks_remaining) {
            pcspk_off();
        }
    }
}

uint64_t get_uptime(void)   // nanoseconds
{
    volatile uint64_t irqs;
    volatile int64_t clock_ticks;
    uint32_t flags;

    cli_save(flags);
    irqs = g_pit->timer_irqs;
    clock_ticks = g_pit->last_clock_tick - read_timer(CLOCK_TIMER);
    restore_flags(flags);

    if (clock_ticks < 0) {
        clock_ticks += g_pit->timer_reloads[CLOCK_TIMER];
    }
    clock_ticks += (irqs * g_pit->timer_reloads[SCHED_TIMER]);

    // 1/1193182 = 838.095ns, close enough
    return 838 * clock_ticks;
}

void beep(int hz, int ms, bool block)
{
    uint32_t flags;
    uint16_t div;

    // TODO: get rid of this
    //      make it a write to /dev/beep somehow
    //      to block: read from /dev/beep, it will return when beep done

    cli_save(flags);

    div = calculate_divisor(hz);
    reload_timer(PCSPK_TIMER, div);

    g_pit->pcspk_ticks_remaining = div_round(ms, QUANTUM_MS);
    pcspk_on(); // turned off in interrupt handler

    restore_flags(flags);

    while (block && g_pit->pcspk_ticks_remaining > 0);
}
