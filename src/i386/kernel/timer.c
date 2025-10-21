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
 *         File: src/i386/kernel/timer.c
 *      Created: March 6, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

// http://www.osdever.net/bkerndev/Docs/pit.htm

#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <i386/x86.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>

//
// reference intervals
//

#define PIT_REFCLK          1193182 // PIT internal/maximum clock frequency, 1.1931818 MHz
#define QUANTUM_MS          20      // approximate interval between timer ticks, in millis

//
// i/o ports
//

#define PIT_PORT_CHAN0      0x40
#define PIT_PORT_CHAN1      0x41
#define PIT_PORT_CHAN2      0x42
#define PIT_PORT_CFG        0x43
#define PIT_PORT_PCSPK      0x61

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

#define RDBK_CHANNEL_0      (1 << (CHANNEL_0))
#define RDBK_CHANNEL_1      (1 << (CHANNEL_1))
#define RDBK_CHANNEL_2      (1 << (CHANNEL_2))


//
// current timer state
//

struct pit_state {
    uint64_t timer_ticks;
    uint64_t pcspk_ticks;

    uint32_t clock_count;
};

static struct pit_state _pit = { };
struct pit_state *g_pit = &_pit;

//
// ----------------------------------------------------------------------------
//

void timer_interrupt(int irq, struct iregs *regs);

static void pcspk_on(void)
{
    outb(PIT_PORT_PCSPK, inb(PIT_PORT_PCSPK) | 0x03);
}

static void pcspk_off(void)
{
    outb(PIT_PORT_PCSPK, inb(PIT_PORT_PCSPK) & ~0x03);
}

static uint64_t ticks_to_ms(uint64_t ticks)
{
    return ticks * QUANTUM_MS;
}

static uint16_t calculate_divisor(int freq)
{
    int div = div_round(PIT_REFCLK, freq);
    if (div > UINT16_MAX) {
        div = UINT16_MAX;
    }
    if (div < 1) {
        div = 1;
    }

    return (uint16_t) div;
}

void init_timer(void)
{
    uint8_t mode;
    uint16_t div;

    // interrupt timer setup
    mode = _PIT_MODE(CHANNEL_0, ACCESS_LOHI, MODE_RATEGEN);
    outb(PIT_PORT_CFG, mode);
    div = calculate_divisor(div_round(1000, QUANTUM_MS));
    outb(PIT_PORT_CHAN0, div & 0xFF);
    outb(PIT_PORT_CHAN0, (div >> 8) & 0xFF);

    // clock timer setup
    mode = _PIT_MODE(CHANNEL_1, ACCESS_LOHI, MODE_INTERRUPT);
    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN1, 0xFF);
    outb(PIT_PORT_CHAN1, 0xFF);
    g_pit->clock_count = 0xFFFF;

    // PC speaker setup
    mode = _PIT_MODE(CHANNEL_2, ACCESS_LOHI, MODE_SQUAREWAVE);
    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN2, 0xFF);
    outb(PIT_PORT_CHAN2, 0xFF);

    irq_register(IRQ_TIMER, timer_interrupt);
    irq_unmask(IRQ_TIMER);
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
    outb(PIT_PORT_CHAN2, div & 0xFF);
    outb(PIT_PORT_CHAN2, (div >> 8) & 0xFF);

    g_pit->pcspk_ticks = div_round(ms, QUANTUM_MS);
    pcspk_on(); // turned off in interrupt handler

    restore_flags(flags);

    while (block && g_pit->pcspk_ticks > 0);
}

static void update_clock(void)
{
    uint8_t status;
    uint16_t count;
    int diff;

    outb(PIT_PORT_CFG, _PIT_RDBK(RDBK_COUNT | RDBK_STATUS, RDBK_CHANNEL_1));
    status = inb(PIT_PORT_CHAN1);
    count  = inb(PIT_PORT_CHAN1);
    count |= inb(PIT_PORT_CHAN1) << 8;

    diff = 0xFFFF - count;
    if (status & 0x80) {    // counter wrapped!
        diff += 0xFFFF;     // should not wrap as long as quantum is <54ms, roughly
    }

    outb(PIT_PORT_CHAN1, 0xFF);
    outb(PIT_PORT_CHAN1, 0xFF);
    g_pit->clock_count = diff;
}

void timer_interrupt(int irq, struct iregs *regs)
{
    assert(irq == IRQ_TIMER);

    g_pit->timer_ticks++;

    if (g_pit->pcspk_ticks) {
        g_pit->pcspk_ticks--;
        if (!g_pit->pcspk_ticks) {
            pcspk_off();
        }
    }

    update_clock();
}

uint64_t get_uptime(void)   // microseconds
{
    uint64_t time_us;
    uint32_t flags;

    cli_save(flags);
    update_clock();
    // 1/1193182Hz = 0.83805911us per clock tick, ~5us every 6 ticks
    time_us  = (g_pit->clock_count / 6) * 5;
    time_us += (g_pit->timer_ticks * QUANTUM_MS) * 1000;
    restore_flags(flags);

    // TODO: need a way to count seconds elapsed if interrupts are off..
    // perhaps use the RTC?

    return time_us;
}
