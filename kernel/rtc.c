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
#include <fs.h>

#define CHATTY                  0

//
// RTC Register Ports
//
#define PORT_REG_A              0xA     // RTC Register A
#define PORT_REG_B              0xB     // RTC Register B
#define PORT_REG_C              0xC     // RTC Register C
#define PORT_REG_D              0xD     // RTC Register D

//
// Register A: Oscillator Mode
//
#define REG_A_RATE              0x0F    // Periodic Interrupt Rate
#define REB_A_DV                0x70    // Oscillator Mode (010b = enable)
#define REG_A_UIP               0x80    // Update In Progress

//
// Register B: Clock and Interrupt Mode
//
#define REG_B_DSE               0x01    // Daylight Saving Enable
#define REG_B_24H               0x02    // 24-hour Mode: 1 = 24h, 0 = 12h
#define REG_B_DM                0x04    // Data Mode: 1 = binary, 0 = BCD
#define REG_B_SQWE              0x08    // Square Wave Enable
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
// Time Registers
//
#define REG_SECONDS             0x00    // [0-59]
#define REG_SECONDS_ALARM       0x01    // [0-59]
#define REG_MINUTES             0x02    // [0-59]
#define REG_MINUTES_ALARM       0x03    // [0-59]
#define REG_HOURS               0x04    // [0-23], [1-12] (12h mode)
#define REG_HOURS_ALARM         0x05    // [0-23], [1-12] (12h mode)
#define REG_DAYOFWEEK           0x06    // [1-7], unreliable apparently...
#define REG_DATEOFMONTH         0x07    // [1-31]
#define REG_MONTH               0x08    // [1-12]
#define REG_YEAR                0x09    // [0-99]

#define PM_FLAG                 0x80    // PM bit, lives in the hours register (12h mode only)

#define rd_a()                  cmos_read(PORT_REG_A)           // Read A register
#define rd_b()                  cmos_read(PORT_REG_B)           // Read B register
#define rd_c()                  cmos_read(PORT_REG_C)           // Read C register
#define rd_d()                  cmos_read(PORT_REG_D)           // Read D register

#define wr_a(data)              cmos_write(PORT_REG_A, data)    // Write A register
#define wr_b(data)              cmos_write(PORT_REG_B, data)    // Write B register
#define wr_c(data)              cmos_write(PORT_REG_C, data)    // Write C register
#define wr_d(data)              cmos_write(PORT_REG_D, data)    // Write D register

#define bcd2bin(n)              ((((n)>>4)*10)+((n)&0x0F))  // bin = ((bcd / 16) * 10) + (bcd % 16)
#define bin2bcd(n)              ((((n)/10)<<4)+((n)%10))    // bcd = ((bin / 10) * 16) + (bin % 10)

int rtc_open(struct file **file, int flags);
int rtc_close(struct file *file);
int rtc_read(struct file *file, char *buf, size_t count);
int rtc_ioctl(struct file *file, unsigned int num, void *arg);

static void rtc_interrupt(void);

static void set_mode(int mask);
static void clear_mode(int mask);

static unsigned char get_rate(void);
static int set_rate(unsigned char rate);

static void get_time(struct rtc_time *time, bool alarm);
static int set_time(struct rtc_time *time, bool alarm);

struct rtc {
    uint32_t ticks;
    uint32_t a_ticks;
    uint32_t p_ticks;
    uint32_t u_ticks;
};
static struct rtc _rtc; // TODO: make per-process

volatile struct rtc * get_rtc(void)
{
    return &_rtc;
}

static struct file_ops rtc_fops =
{
    .read = rtc_read,
    .write = NULL,
    .open = rtc_open,
    .close = rtc_close,
    .ioctl = rtc_ioctl
};

static struct file rtc_file =
{
    .fops = &rtc_fops,
    .ioctl_code = _IOC_RTC
};

void init_rtc(void)
{
    uint8_t data;
    uint32_t flags;

    //
    // disable interrupts
    //
    cli_save(flags);
    nmi_disable();

    //
    // zero RTC structure
    //
    zeromem(&_rtc, sizeof(struct rtc));

    //
    // flush RTC
    //
    (void) rd_c();

    //
    // sanity checks
    //
    data = rd_d();
    if (!(data & REG_D_VRT)) {
        kprint("rtc: VRT bit not set! Is your CMOS battery dead?\n");
    }

    //
    // enable oscillator
    //
    data = rd_a();
    data |= 0x20;   // bit pattern '010' in "DV" bits
    wr_a(data);

    //
    // configure mode
    //
    data = rd_b();
    data &= ~REG_B_UIE; // disable 'update ended' interrupts
    data &= ~REG_B_AIE; // disable alarm interrupts
    // data &= ~REG_B_PIE; // disable periodic interrupts
    data |= REG_B_PIE;
    data &= ~REG_B_DSE; // disable 'daylight saving enable'
    wr_b(data);

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
    uint8_t reg_c;

    reg_c = rd_c();
    get_rtc()->ticks++;

    if (reg_c & REG_C_AF) {
        get_rtc()->a_ticks++;
    }
    if (reg_c & REG_C_PF) {
        get_rtc()->p_ticks++;
    }
    if (reg_c & REG_C_UF) {
        get_rtc()->u_ticks++;
    }
}

static void set_mode(int mask)
{
    uint8_t data;
    uint32_t flags;

    cli_save(flags);
    data = rd_b();
    data |= mask;
    wr_b(data);
    restore_flags(flags);
}

static void clear_mode(int mask)
{
    uint8_t data;
    uint32_t flags;

    cli_save(flags);
    data = rd_b();
    data &= ~mask;
    wr_b(data);
    restore_flags(flags);
}

static unsigned char get_rate(void)
{
    uint32_t flags;
    uint8_t rate;

    cli_save(flags);
    rate = rd_a() & REG_A_RATE;
    restore_flags(flags);

    return rate;
}

static int set_rate(unsigned char rate)
{
    uint32_t flags;
    uint8_t data;

    if (rate > RTC_RATE_2Hz || rate < RTC_RATE_8192Hz) {
        return -EINVAL;
    }

    cli_save(flags);
    data = rd_a() & ~REG_A_RATE;
    data |= (rate & REG_A_RATE);
    wr_a(data);
    restore_flags(flags);

#if CHATTY
    kprint("rtc: periodic interrupt frequency is now %dHz\n", rate2hz(rate));
#endif

    return 0;
}

static void get_time(struct rtc_time *time, bool alarm)
{
    uint32_t flags;
    uint8_t regb;
    bool pm;

    cli_save(flags);

    // zero time struct
    zeromem(time, sizeof(struct rtc_time));

    // spin until update-in-progress bit goes low
    spin(rd_a() & REG_A_UIP);       // TODO: TIMEOUT!!!! don't deadlock the kernel ;)

    // disable RTC updates so we can safely read CMOS RAM
    regb = rd_b();
    regb |= REG_B_SET;
    wr_b(regb);

    // read time bits
    if (alarm) {
        time->tm_sec = cmos_read(REG_SECONDS_ALARM);
        time->tm_min = cmos_read(REG_MINUTES_ALARM);
        time->tm_hour = cmos_read(REG_HOURS_ALARM);
    }
    else {
        time->tm_sec = cmos_read(REG_SECONDS);
        time->tm_min = cmos_read(REG_MINUTES);
        time->tm_hour = cmos_read(REG_HOURS);
        time->tm_mday = cmos_read(REG_DATEOFMONTH);
        time->tm_mon = cmos_read(REG_MONTH);
        time->tm_year = cmos_read(REG_YEAR);
    }

    // re-enable updates
    regb &= ~REG_B_SET;
    wr_b(regb);

#if CHATTY
    if (alarm) {
        kprint("rtc: get_time: cmos alarm is %02d:%02d:%02d (hex: %02x:%02x:%02x)\n",
            time->tm_hour, time->tm_min, time->tm_sec,
            time->tm_hour, time->tm_min, time->tm_sec);
    }
    else {
        kprint("rtc: get_time: cmos time is %02d/%02d/%02d %02d:%02d:%02d (hex: %02x/%02x/%02x %02x:%02x:%02x)\n",
            time->tm_mon, time->tm_mday, time->tm_year,
            time->tm_hour, time->tm_min, time->tm_sec,
            time->tm_mon, time->tm_mday, time->tm_year,
            time->tm_hour, time->tm_min, time->tm_sec);
    }
#endif

    // RTC using 12h time?
    // if so, convert to 24h; PM is indicated in bit 7 of the hour;
    // do this before BCD conversion
    pm = false;
    if (!(regb & REG_B_24H)) {
#if CHATTY
        kprint("rtc: get_time: time is in 12h format\n");
#endif
        if ((time->tm_hour & PM_FLAG)) {
            time->tm_hour &= ~PM_FLAG;
            pm = true;
        }
    }

    // RTC time formatted in BCD?
    // some hardware doesn't seem to honor this bit if we manually set it,
    // so just read it as-is and convert to binary if necessary
    if (!(regb & REG_B_DM)) {
#if CHATTY
        kprint("rtc: get_time: time is in BCD\n");
#endif
        time->tm_sec = bcd2bin(time->tm_sec);
        time->tm_min = bcd2bin(time->tm_min);
        time->tm_hour = bcd2bin(time->tm_hour);
        time->tm_mday = bcd2bin(time->tm_mday);
        time->tm_mon = bcd2bin(time->tm_mon);
        time->tm_year = bcd2bin(time->tm_year);
    }

    // if RTC is using using 12h time, convert to 24h
    if (!(regb & REG_B_24H)) {
        if (pm && time->tm_hour < 12) {
            time->tm_hour += 12;    // PM: [1-12] -> [12-23]
        }
        else if (!pm && time->tm_hour == 12) {
            time->tm_hour = 0;      // AM: [1-12] -> [0-11]
        }
    }

    // account for Y2K;
    // if the year is >= 90, it's assumed to mean 19YY
    // therefore 00-89 = 20XX
    if (time->tm_year < 90) {
        time->tm_year += 100;   // tm_year is years since 1900
    }

    // convert the month
    time->tm_mon -= 1;          // tm_mon is 0-11

    restore_flags(flags);
}

static int set_time(struct rtc_time *time, bool alarm)
{
    bool pm;
    uint32_t flags;
    uint8_t regb;

    if (time->tm_sec < 0 || time->tm_sec > 59 ||
        time->tm_min < 0 || time->tm_min > 59 ||
        time->tm_hour < 0 || time->tm_hour > 23 ||
        (!alarm && (time->tm_mday < 1 || time->tm_mday > 31)) ||
        (!alarm && (time->tm_mon < 0 || time->tm_mon > 11)) ||
        (!alarm && (time->tm_year < 90 || time->tm_year > 189)))
    {
        return -EINVAL;
    }

    cli_save(flags);

    // read B register to get RTC state
    regb = rd_b();

    // adjust time for RTC ranges
    time->tm_mon += 1;              // tm_mon is 0-11, RTC CMOS is 1-12

    // handle Y2K
    if (time->tm_year >= 100) {     // tm_year: years since 1900
        time->tm_year -= 100;       // RTC CMOS: 0-89 = 20YY, 90-99 = 19YY
    }

    // RTC using 12h time?
    // if so, convert time to 12h and keep track of PM bit
    pm = false;
    if (!(regb & REG_B_24H)) {
        pm = (time->tm_hour >= 12);
        if (time->tm_hour > 12) {
            time->tm_hour -= 12;        // [13-23] -> [1-12] PM
        }
        else if (time->tm_hour == 0) {
            time->tm_hour = 12;         // [12] -> [12] PM
        }                               // [0-11] -> [1-12] AM
    }

    // RTC using BCD?
    // if so, convert to BCD
    if (!(regb & REG_B_DM)) {
        time->tm_year = bin2bcd(time->tm_year);
        time->tm_mon = bin2bcd(time->tm_mon);
        time->tm_mday = bin2bcd(time->tm_mday);
        time->tm_hour = bin2bcd(time->tm_hour);
        time->tm_min = bin2bcd(time->tm_min);
        time->tm_sec = bin2bcd(time->tm_sec);
    }

    // set the PM flag after BCD conversion
    if (pm) {
        time->tm_hour |= PM_FLAG;
    }

    // spin until update-in-progress bit goes low
    spin(rd_a() & REG_A_UIP);   // TODO: TIMEOUT!!!

    // disable RTC updates so we can safely write CMOS RAM
    regb = rd_b();
    regb |= REG_B_SET;
    wr_b(regb);

    // write the time to CMOS RAM
    if (alarm) {
        cmos_write(REG_HOURS_ALARM, time->tm_hour);
        cmos_write(REG_MINUTES_ALARM, time->tm_min);
        cmos_write(REG_SECONDS_ALARM, time->tm_sec);
    }
    else {
        cmos_write(REG_YEAR, time->tm_year);
        cmos_write(REG_MONTH, time->tm_mon);
        cmos_write(REG_DATEOFMONTH, time->tm_mday);
        cmos_write(REG_HOURS, time->tm_hour);
        cmos_write(REG_MINUTES, time->tm_min);
        cmos_write(REG_SECONDS, time->tm_sec);
    }

#if CHATTY
    if (alarm) {
        kprint("rtc: set_time: cmos alarm set to %02d:%02d:%02d (hex: %02x:%02x:%02x)\n",
            time->tm_hour, time->tm_min, time->tm_sec,
            time->tm_hour, time->tm_min, time->tm_sec);
    }
    else {
        kprint("rtc: set_time: cmos time set to %02d/%02d/%02d %02d:%02d:%02d (hex: %02x/%02x/%02x %02x:%02x:%02x)\n",
            time->tm_mon, time->tm_mday, time->tm_year,
            time->tm_hour, time->tm_min, time->tm_sec,
            time->tm_mon, time->tm_mday, time->tm_year,
            time->tm_hour, time->tm_min, time->tm_sec);
    }
#endif

    // re-enable updates
    regb &= ~REG_B_SET;
    wr_b(regb);

    restore_flags(flags);
    return 0;
}

int rtc_open(struct file **file, int flags)
{
    (void) flags;

    *file = &rtc_file;
    return 0;
}

int rtc_close(struct file *file)
{
    return 0;
}

int rtc_read(struct file *file, char *buf, size_t count)
{
    uint32_t flags;
    uint32_t tick;

    if (count < sizeof(uint32_t)) {
        return -EINVAL;
    }

    // TODO: encode interrupt type into returned value
    // TODO: tick 'count/sizeof(uint32_t)' times?

    // get current tick count
    cli_save(flags);
    tick = get_rtc()->ticks;
    sti();

    // spin until another tick happens
    spin(tick == get_rtc()->ticks);
    cli();

    // capture new tick count
    tick = get_rtc()->ticks;
    memcpy(buf, &tick, sizeof(uint32_t));

    restore_flags(flags);
    return sizeof(uint32_t);
}

int rtc_ioctl(struct file *file, unsigned int num, void *arg)
{
    int ret;
    unsigned char rate;
    struct rtc_time time;

    (void) file;

    ret = 0;
    switch (num) {
        case RTC_ALARM_DISABLE: clear_mode(REG_B_AIE); break;
        case RTC_ALARM_ENABLE: set_mode(REG_B_AIE); break;
        case RTC_IRQP_DISABLE: clear_mode(REG_B_PIE); break;
        case RTC_IRQP_ENABLE: set_mode(REG_B_PIE); break;
        case RTC_UPDATE_DISABLE: clear_mode(REG_B_UIE); break;
        case RTC_UPDATE_ENABLE: set_mode(REG_B_UIE); break;

        case RTC_IRQP_GET:
            rate = get_rate();
            copy_to_user(arg, &rate, sizeof(unsigned char));
            break;

        case RTC_IRQP_SET:
            copy_from_user(&rate, arg, sizeof(unsigned char));
            ret = set_rate(rate);
            break;

        case RTC_TIME_GET: __fallthrough;
        case RTC_ALARM_GET:
            get_time(&time, num == RTC_ALARM_GET);
            copy_to_user(arg, &time, sizeof(struct rtc_time));
            break;

        case RTC_TIME_SET: __fallthrough;
        case RTC_ALARM_SET:
            copy_from_user(&time, arg, sizeof(struct rtc_time));
            ret = set_time(&time, num == RTC_ALARM_SET);
            break;

        default:
            ret = -ENOTTY;
            break;
    }

    return ret;
}
