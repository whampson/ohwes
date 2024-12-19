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
 *         File: rtc_test.c
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <ioctl.h>
#include <panic.h>
#include <rtc.h>
#include <syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TODO: make this a program on disk

#define CHECK(x)            \
({                          \
    int __ret = (x);        \
    if (__ret < 0) {        \
        panic(#x, errno);   \
    }                       \
    __ret;                  \
})

#define WAIT_TIME           3   // sec

static void add_seconds(struct rtc_time *time, int seconds)
{
    time->tm_sec += seconds;
    if (time->tm_sec >= 60) {
        time->tm_min += time->tm_sec / 60;
        time->tm_sec %= 60;

    }
    if (time->tm_min >= 60) {
        time->tm_hour += time->tm_min / 60;
        time->tm_min %= 60;
    }
    if (time->tm_hour >= 24) {
        time->tm_hour %= 24;
    }
    // not doing the date lol sorry
}

static void print_datetime(struct rtc_time *dt)
{
    printf("%02d/%02d/%04d %02d:%02d:%02d", // american format >:)
        dt->tm_mon + 1, dt->tm_mday, dt->tm_year + 1900,
        dt->tm_hour, dt->tm_min, dt->tm_sec);
}

static void print_time(struct rtc_time *tm)
{
    printf("%02d:%02d:%02d",
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void rtc_test(void)
{
    uint32_t rtc_data[1];
    struct rtc_time time;
    struct rtc_time orig_time;
    int fd;
    int rate;
    int ret;

    printf("rtc_test:\n");
    errno = 0;

    //
    // open RTC
    //
    fd = open("/dev/rtc", 0);
    assert(fd > 0);

    //
    // disable all interrupt types
    //
    CHECK(ioctl(fd, RTC_UPDATE_DISABLE, NULL));
    CHECK(ioctl(fd, RTC_IRQP_DISABLE, NULL));
    CHECK(ioctl(fd, RTC_ALARM_DISABLE, NULL));

    //
    // enable update interrupts (1 per sec)
    //
    CHECK(ioctl(fd, RTC_UPDATE_ENABLE, NULL));

    //
    // wait for n seconds, 1 interrupt per second
    //
    printf("waiting %d seconds using clock update interrupts...\n", WAIT_TIME);
    for (int i = 0; i < WAIT_TIME; i++) {
        ret = CHECK(read(fd, rtc_data, sizeof(rtc_data)));   // should block for 1s...
        assert(ret == sizeof(uint32_t));
        printf("!");
    }
    printf("\n");

    //
    // disable update interrupts
    //
    CHECK(ioctl(fd, RTC_UPDATE_DISABLE, NULL));

    //
    // set periodic rate and enable periodic interrupts
    //
    rate = RTC_RATE_4Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_4Hz);
    CHECK(ioctl(fd, RTC_IRQP_ENABLE, NULL));

    //
    // wait approx n seconds, 'rate' interrupts per second
    //
    printf("waiting %d seconds using periodic interrupts at %d Hz...\n", WAIT_TIME, rate2hz(rate));
    for (int i = 0; i < WAIT_TIME * rate2hz(rate); i++) {
        ret = CHECK(read(fd, rtc_data, sizeof(rtc_data)));  // TODO: what should the data be...?
        assert(ret == sizeof(uint32_t));
        printf("%d ", i + 1);
    }
    printf("\n");

    //
    // set rate to highest possible and disable periodic interrupts
    //
    rate = RTC_RATE_8192Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_8192Hz);
    CHECK(ioctl(fd, RTC_IRQP_DISABLE, NULL));

    //
    // get current time
    //
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("current date and time is ");
    print_datetime(&time);
    printf("\n");

    //
    // set alarm to n seconds ahead of current time, handle rollover
    //
    add_seconds(&time, WAIT_TIME);
    CHECK(ioctl(fd, RTC_ALARM_SET, &time));
    CHECK(ioctl(fd, RTC_ALARM_GET, &time));
    printf("alarm set to ring %d seconds from now at ", WAIT_TIME);
    print_time(&time);
    printf("\n");

    //
    // enable alarm interrupts
    //
    CHECK(ioctl(fd, RTC_ALARM_ENABLE, NULL));

    //
    // wait for alarm
    //
    printf("waiting for alarm to ring...\n");
    CHECK(read(fd, rtc_data, sizeof(rtc_data)));

    //
    // disable alarm interrupts
    //
    CHECK(ioctl(fd, RTC_ALARM_DISABLE, NULL));

    //
    // get time after alarm
    //
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("alarm rang at "); print_time(&time); printf("\n");

    //
    // set the time and read it back
    //
    orig_time = time;
    add_seconds(&time, 31337);
    CHECK(ioctl(fd, RTC_TIME_SET, &time));
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("time temporarily set to "); print_time(&time); printf("\n");

    //
    // wait for a bit 'cause why not? let's add a cool spinner!
    //
    rate = RTC_RATE_4Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    CHECK(ioctl(fd, RTC_IRQP_ENABLE, NULL));
    for (int i = 0; i < WAIT_TIME * rate2hz(rate); i++) {
        ret = CHECK(read(fd, rtc_data, sizeof(rtc_data)));
        assert(ret == sizeof(uint32_t));

        char *spinnybits = "-\\|/";
        printf("\r%c", spinnybits[i % 4]);
    }
    printf("\r");

    //
    // restore time, set periodic rate
    //
    add_seconds(&orig_time, WAIT_TIME);
    CHECK(ioctl(fd, RTC_TIME_SET, &orig_time));
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("time restored to "); print_datetime(&time); printf("\n");
    rate = RTC_RATE_8192Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));

    //
    // close RTC
    //
    ret = close(fd);
    assert(ret == 0);
}
