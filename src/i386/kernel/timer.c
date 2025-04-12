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
    uint64_t sys_timer;
    uint64_t ticks;
    uint32_t pcspk_ticks;
    uint32_t sleep_ticks;
    int quantum_ms;
};

static struct pit_state _pit = {};
struct pit_state * get_pit(void)
{
    return &_pit;
}

#ifdef DEBUG
extern void _crash_key_proc(int irq, struct iregs *regs);
#endif

static uint16_t calculate_divisor(int freq);
void timer_interrupt(int irq, struct iregs *regs);

void init_timer(void)
{
    uint8_t mode;
    uint16_t div;
    int freq;
    struct pit_state *pit;

    pit = get_pit();
    zeromem(pit, sizeof(struct pit_state));
    pit->quantum_ms = QUANTUM_MS;

    freq = div_round(1000, pit->quantum_ms);
    div = calculate_divisor(freq);

    mode = PIT_CFG_CHANNEL_0 | PIT_CFG_MODE_RATEGEN | PIT_CFG_ACCESS_LOHI;
    assert(mode == 0x34);
    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN0, div & 0xFF);
    outb(PIT_PORT_CHAN0, (div >> 8) & 0xFF);

    irq_register(IRQ_TIMER, timer_interrupt);

#ifdef DEBUG        // CTRL+ALT+FN to crash kernel
    irq_register(IRQ_TIMER, _crash_key_proc);
#endif

    irq_unmask(IRQ_TIMER);
    pic_unmask(IRQ_TIMER);
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

void timer_sleep(int millis)
{
    uint32_t flags;
    volatile struct pit_state *pit;

    cli_save(flags);

    pit = get_pit();
    if (pit->quantum_ms) {
        pit->sleep_ticks = div_round(millis, pit->quantum_ms);
        // kprint("timer: sleeping for %dms (%d ticks)\n", millis, pit.sleep_ticks);
    }

    __sti();
    while (pit->sleep_ticks) { }
    __cli();

    restore_flags(flags);
}

void pcspk_on(void)
{
    uint8_t data;

    data = inb(0x61);
    data |= 0x03;
    outb(0x61, data);
}

void pcspk_off(void)
{
    uint8_t data;

    data = inb(0x61);
    data &= ~0x03;
    outb(0x61, data);
}

void pcspk_beep(int freq, int millis)
{
    uint32_t flags;
    uint8_t mode;
    uint16_t div;
    struct pit_state *pit;

    cli_save(flags);

    div = calculate_divisor(freq);
    mode = PIT_CFG_CHANNEL_2 | PIT_CFG_MODE_SQUAREWAVE | PIT_CFG_ACCESS_LOHI;
    assert(mode == 0xB6);

    outb(PIT_PORT_CFG, mode);
    outb(PIT_PORT_CHAN2, div & 0xFF);
    outb(PIT_PORT_CHAN2, (div >> 8) & 0xFF);

    pit = get_pit();
    if (pit->quantum_ms) {
        pit->pcspk_ticks = div_round(millis, pit->quantum_ms);
        pcspk_on(); // turned off in interrupt handler
    }

    restore_flags(flags);
}

void timer_interrupt(int irq, struct iregs *regs)
{
    volatile struct pit_state *pit;
    assert(irq == IRQ_TIMER);

    pit = get_pit();
    pit->ticks++;       // TODO: read tick count from PIT?

    if (pit->pcspk_ticks) {
        pit->pcspk_ticks--;
        if (!pit->pcspk_ticks) {
            pcspk_off();
        }
    }

    if (pit->sleep_ticks) {
        pit->sleep_ticks--;
    }
}
