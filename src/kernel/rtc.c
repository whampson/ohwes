/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/rtc.c
 *      Created: March 31, 2024
 *       Author: Wes Hampson
 *
 * =============================================================================
 */

#include <ohwes.h>
#include <io.h>
#include <x86.h>
#include <irq.h>

#define RTC_STATUS_REG_A        0xA
#define RTC_STATUS_REG_B        0xB
#define RTC_STATUS_REG_C        0xC

#define RTC_RATE_OFF            0
#define RTC_RATE_2Hz            0xF
#define RTC_RATE_4Hz            0xE
#define RTC_RATE_8Hz            0xD
#define RTC_RATE_16Hz           0xC
#define RTC_RATE_32Hz           0xB
#define RTC_RATE_64Hz           0xA
#define RTC_RATE_128Hz          0x9
#define RTC_RATE_256Hz          0x8
#define RTC_RATE_512Hz          0x7
#define RTC_RATE_1024Hz         0x6
#define RTC_RATE_2048Hz         0x5
#define RTC_RATE_4096Hz         0x4
#define RTC_RATE_8192Hz         0x3

#define rate_to_hz(r)           (32768 >> ((r) - 1))

static void rtc_interrupt(void);
static uint8_t flush_rtc(void);

void init_rtc(void)
{
    uint8_t data;
    uint8_t rate;
    uint32_t flags;

    cli_save(flags);
    nmi_disable();

    (void) flush_rtc();

    // set rate
    rate = RTC_RATE_8192Hz; // fastest rate, TODO: freq-divide per process
    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_A);
    data = inb_delay(CMOS_DATA_PORT);
    kprint("rtc: initial rate = %d Hz\n", rate_to_hz(data & 0xF));
    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_A);
    outb_delay(CMOS_DATA_PORT, (data & 0xF0) | rate);
    // readback
    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_A);
    data = inb_delay(CMOS_DATA_PORT);
    kprint("rtc: current rate = %d Hz\n", rate_to_hz(data & 0xF));

    // enable interrupts
    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_B);
    data = inb_delay(CMOS_DATA_PORT);
    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_B);
    outb_delay(CMOS_DATA_PORT, 0x40 | data);        // 0x40 = Periodic Interrupts Enable

    // register IRQ handler
    irq_register(IRQ_RTC, rtc_interrupt);
    irq_unmask(IRQ_RTC);

    nmi_enable();
    restore_flags(flags);
}

static void rtc_interrupt(void)
{
    // kprint("!");
    (void) flush_rtc();
}

static uint8_t flush_rtc(void)
{
    uint8_t data;

    outb_delay(CMOS_INDEX_PORT, RTC_STATUS_REG_C);
    data = inb_delay(CMOS_DATA_PORT);

    return data;
}
