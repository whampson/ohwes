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

#include <assert.h>
#include <ohwes.h>
#include <errno.h>
#include <io.h>
#include <x86.h>
#include <irq.h>
#include <rtc.h>

#define PARANOID

//
// RTC Register Ports
//
#define RTC_PORT_REG_A          0xA     // RTC Register A
#define RTC_PORT_REG_B          0xB     // RTC Register B
#define RTC_PORT_REG_C          0xC     // RTC Register C
#define RTC_PORT_REG_D          0xD     // RTC Register D

//
// Register A: Oscillator Mode
//
#define REG_A_RATE              0x0F    // Periodic Interrupt Rate
#define REB_A_DV                0x70    // Oscillator Mode
#define REG_A_UIP               0x80    // Update In Progress

//
// Register B: Clock and Interrupt Mode
//
#define REG_B_DSE               0x01    // Daylight Savings Enable
#define REG_B_24H               0x02    // 24-hour Mode: 1 = 24h, 0 = 12h
#define REG_B_DM                0x04    // Data Mode: 1 = binary, 0 = BCD
#define REB_B_SQWE              0x08    // Square Wave Enable
#define REG_B_UIE               0x10    // Update Ended Interrupt Enable
#define REG_B_AIE               0x20    // Alarm Interrupt Enable
#define REG_B_PIE               0x40    // Periodic Interrupt Enable
#define REG_B_SET               0x80    // Disable Updates

//
// Register C: Interrupt Status
//
#define REG_C_UF                0x10    // Update Ended Interrupt Flag
#define REG_C_AF                0x20    // Alarm Interrupt Flag
#define REG_C_PF                0x40    // Periodic Interrupt Flag
#define REG_C_IRQF              0x80    // IRQ sent to CPU

//
// Register D: RAM Status
//
#define REG_D_VRT               0x80    // Valid RAM and Time (battery alive)

//
// Periodic Interrupt Rates
//
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

#define rate2hz(r)              (32768 >> ((r) - 1))

#define rd_a()                  cmos_read(RTC_PORT_REG_A)
#define rd_b()                  cmos_read(RTC_PORT_REG_B)
#define rd_c()                  cmos_read(RTC_PORT_REG_C)
#define rd_d()                  cmos_read(RTC_PORT_REG_D)

#define wr_a(data)              cmos_write(RTC_PORT_REG_A, data)
#define wr_b(data)              cmos_write(RTC_PORT_REG_B, data)
#define wr_c(data)              cmos_write(RTC_PORT_REG_C, data)
#define wr_d(data)              cmos_write(RTC_PORT_REG_D, data)

static struct tm tm_now;

static void rtc_interrupt(void);
static void update_time(void);

void init_rtc(void)
{
    uint8_t data;
    uint8_t rate;
    uint32_t flags;

    //
    // disable interrupts
    //
    cli_save(flags);
    nmi_disable();

    //
    // flush RTC
    //
    (void) rd_c();

    //
    // set rate
    //
    rate = RTC_RATE_8192Hz; // fastest rate, TODO: freq-divide per process
    data = rd_a();
    kprint("rtc: initial rate = %d Hz\n", rate2hz(data & REG_A_RATE));
    wr_a((data & ~REG_A_RATE) | rate);
#ifdef PARANOID
    // readback
    data = rd_a();
    kprint("rtc: current rate = %d Hz\n", rate2hz(data & REG_A_RATE));
#endif

    //
    // configure mode
    //
    data = rd_b();
    data |= REG_B_PIE;  // enable periodic interrupts
    data |= REG_B_24H;  // 24-hour time (not 12-hour)
    data |= REG_B_DM;   // binary (not BCD)
    data |= REG_B_UIE;  // enable 'update ended' interrupts
    data &= ~REG_B_AIE; // disable alarm interrupts
    data &= ~REG_B_DSE; // disable 'daylight saving enable'
    wr_b(data);
#ifdef PARANOID
    // readback
    data = rd_b();
    if (!(data & REG_B_PIE)) {
        panic("rtc: failed to enable periodic interrupts");
    }
    if (!(data & REG_B_24H)) {
        panic("rtc: failed to enable 24h mode");
    }
    if (!(data & REG_B_DM)) {
        panic("rtc: failed to enable binary data mode");
    }
    if (data & REG_B_AIE) {
        panic("rtc: failed to disable alarm interrupts");
    }
    if (data & REG_B_DSE) {
        panic("rtc: failed to disable Daylight Saving mode");
    }
#endif

    //
    // sanity checks
    //
    data = rd_d();
    if (!(data & REG_D_VRT)) {
        kprint("rtc: VRT bit not set! Is your CMOS battery dead?\n");
    }

    //
    // get the current time of day
    //
    zeromem(&tm_now, sizeof(struct tm));
    update_time();

    //
    // register IRQ handler
    //
    irq_register(IRQ_RTC, rtc_interrupt);
    irq_unmask(IRQ_RTC);

    //
    // restore interrupt state
    //
    nmi_enable();
    restore_flags(flags);
}

static void rtc_interrupt(void)
{
    uint8_t reg_b;
    uint8_t reg_c;

    reg_c = rd_c();
    reg_b = rd_b();
    (void) reg_b;
    (void) reg_c;

    // might as well update the time...
    if (reg_c & REG_C_UF) {
        update_time();
    }
}

static void update_time(void)
{
    uint8_t data;
    struct tm tmp;

    // disable RTC updates so we can safely read CMOS RAM
    data = rd_b();
    wr_b(data | REG_B_SET);

    // grab the time bits
    tmp.tm_sec = cmos_read(0x00);
    tmp.tm_min = cmos_read(0x02);
    tmp.tm_hour = cmos_read(0x04);
    tmp.tm_mday = cmos_read(0x07);
    tmp.tm_mon = cmos_read(0x08) - 1;
    tmp.tm_year = cmos_read(0x09);
    tmp.tm_wday = -1;   // unrelabie on RTC, apparently...
    tmp.tm_yday = -1;   // not available from RTC
    tmp.tm_isdst = -1;  // not available from RTC

    if (tmp.tm_year < 90) {
        tmp.tm_year += 2000;
    }
    else {
        tmp.tm_year += 1900;
    }

    if (tmp.tm_sec != tm_now.tm_sec ||
        tmp.tm_min != tm_now.tm_min ||
        tmp.tm_hour != tm_now.tm_hour ||
        tmp.tm_mday != tm_now.tm_mday ||
        tmp.tm_mon != tm_now.tm_mon ||
        tmp.tm_year != tm_now.tm_year)
    {
        tm_now = tmp;
        kprint("\e[s\e[25;61H%02d/%02d/%04d %02d:%02d:%02d\e[u",
            tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_year,
            tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    }

    // re-enable updates
    data &= ~REG_B_SET;
    wr_b(data);
}

int rtc_gettime(struct tm *tm)
{
    uint32_t flags;

    if (tm == NULL) {
        return -EINVAL;
    }

    cli_save(flags);
    memcpy(tm, &tm_now, sizeof(struct tm));
    restore_flags(flags);

    return 0;
}
