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

#define PIT_PORT_CHAN0              0x40
#define PIT_PORT_CHAN1              0x41
#define PIT_PORT_CHAN2              0x42
#define PIT_PORT_CFG                0x43

#define PIT_REFCLK                  1193182     // PIT internal/maximum clock frequency, 1.1931818 MHz

#define PIT_CFG_CHANNEL_0           (0<<6)
#define PIT_CFG_CHANNEL_1           (1<<6)
#define PIT_CFG_CHANNEL_2           (2<<6)
#define PIT_CFG_CHANNEL_READBACK    (3<<6)
#define PIT_CFG_ACCESS_LATCH        (0<<4)
#define PIT_CFG_ACCESS_LOBYTE       (1<<4)
#define PIT_CFG_ACCESS_HIBYTE       (2<<4)
#define PIT_CFG_ACCESS_LOHI         (3<<4)
#define PIT_CFG_MODE_INTERRUPT      (0<<1)
#define PIT_CFG_MODE_ONESHOT        (1<<1)
#define PIT_CFG_MODE_RATEGEN        (2<<1)
#define PIT_CFG_MODE_SQUAREWAVE     (3<<1)
#define PIT_CFG_MODE_SWSTROBE       (4<<1)
#define PIT_CFG_MODE_HWSTROBE       (5<<1)

#define QUANTUM_MS                  20          // millis between timer interrupts

struct pit_state {
    uint64_t ticks;
    uint64_t pcspk_ticks;
    uint64_t sleep_ticks;
};
extern bool g_timer_initialized;

static struct pit_state _pit = { };
struct pit_state *g_pit = &_pit;

static uint16_t calculate_divisor(int freq);
void timer_interrupt(int irq, struct iregs *regs);

void init_timer(void)
{
    uint8_t mode;
    uint16_t div;
    int freq;
    freq = div_round(1000, QUANTUM_MS);
    div = calculate_divisor(freq);

    mode = PIT_CFG_CHANNEL_0 | PIT_CFG_MODE_RATEGEN | PIT_CFG_ACCESS_LOHI;
    assert(mode == 0x34);
    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN0, div & 0xFF);
    outb(PIT_PORT_CHAN0, (div >> 8) & 0xFF);

    irq_register(IRQ_TIMER, timer_interrupt);

    irq_unmask(IRQ_TIMER);
    g_timer_initialized = true;
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

static void pcspk_on(void)
{
    uint8_t data;
    data = inb(0x61);
    data |= 0x03;
    outb(0x61, data);
}

static void pcspk_off(void)
{
    uint8_t data;
    data = inb(0x61);
    data &= ~0x03;
    outb(0x61, data);
}

void beep(int hz, int ms)
{
    uint32_t flags;
    uint8_t mode;
    uint16_t div;

    cli_save(flags);

    div = calculate_divisor(hz);
    mode = PIT_CFG_CHANNEL_2 | PIT_CFG_MODE_SQUAREWAVE | PIT_CFG_ACCESS_LOHI;
    assert(mode == 0xB6);

    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN2, div & 0xFF);
    outb(PIT_PORT_CHAN2, (div >> 8) & 0xFF);

    g_pit->pcspk_ticks = div_round(ms, QUANTUM_MS);
    pcspk_on(); // turned off in interrupt handler

    restore_flags(flags);
}

void timer_interrupt(int irq, struct iregs *regs)
{
    assert(irq == IRQ_TIMER);

    g_pit->ticks++;     // TODO: read tick count from PIT?
                        // TODO: do timer ticks and speaker ticks trigger on the same pulse?

    if (g_pit->pcspk_ticks) {
        g_pit->pcspk_ticks--;
        if (!g_pit->pcspk_ticks) {
            pcspk_off();
        }
    }
}
