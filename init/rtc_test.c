#include <errno.h>
#include <ioctl.h>
#include <panic.h>
#include <rtc.h>
#include <syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x)            \
({                          \
    int __ret = (x);        \
    if (__ret < 0) {        \
        panic(#x, errno);   \
    }                       \
    __ret;                  \
})

#define WAIT_TIME   3   // sec

void print_datetime(struct rtc_time *dt)
{
    printf("%02d/%02d/%04d %02d:%02d:%02d",
        dt->tm_mon + 1, dt->tm_mday, dt->tm_year + 1900,
        dt->tm_hour, dt->tm_min, dt->tm_sec);
}

void print_time(struct rtc_time *tm)
{
    printf("%02d:%02d:%02d",
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void rtc_test(void)
{
    uint32_t data[3];
    struct rtc_time time;
    int fd;
    int rate;
    int ret;

    errno = 0;

    //
    // open RTC
    //
    fd = open("/dev/rtc", 0);
    assert(fd > 0);

    //
    // enable update interrupts (1/sec)
    //
    CHECK(ioctl(fd, RTC_UIE_ENABLE, NULL));

    //
    // wait for n seconds, 1 interrupt per second
    //
    for (int i = 0; i < WAIT_TIME; i++) {
        CHECK(read(fd, &data, sizeof(data)));
        printf("!");
    }
    printf("\n");

    //
    // disable update interrupts
    //
    CHECK(ioctl(fd, RTC_UIE_DISABLE, NULL));

    //
    // set periodic rate to 2HZ and enable periodic interrupts
    //
    rate = RTC_RATE_2Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_2Hz);
    CHECK(ioctl(fd, RTC_PIE_ENABLE, NULL));

    //
    // wait approx n seconds, 2 interrupts per second
    //
    for (int i = 0; i < WAIT_TIME * rate_to_hz(rate); i++) {
        ret = CHECK(read(fd, (char *) data, sizeof(data)));
        assert(ret == sizeof(uint32_t));
        printf("%d ", data[0]);
    }
    printf("\n");

    //
    // set rate to highest possible and disable periodic interrupts
    //
    rate = RTC_RATE_8192Hz;
    CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_8192Hz);
    CHECK(ioctl(fd, RTC_PIE_DISABLE, NULL));

    //
    // get current time
    //
    printf("RTC_TIME_GET\n");
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("current time is "); print_datetime(&time); printf("\n");

    //
    // set alarm to n seconds ahead of current time, handle rollover
    //
    time.tm_sec += WAIT_TIME;
    if (time.tm_sec >= 60) {
        time.tm_sec %= 60;
        time.tm_min += 1;
    }
    if (time.tm_min >= 60) {
        time.tm_min %= 60;
        time.tm_hour += 1;
    }
    if (time.tm_hour >= 24) {
        time.tm_hour %= 24;
    }
    printf("RTC_ALARM_SET\n");
    CHECK(ioctl(fd, RTC_ALARM_SET, &time));

    printf("RTC_ALARM_GET\n");
    CHECK(ioctl(fd, RTC_ALARM_GET, &time));
    printf("alarm is set to ring at "); print_time(&time); printf("\n");

    //
    // enable alarm interrupts
    //
    CHECK(ioctl(fd, RTC_AIE_ENABLE, NULL));

    //
    // wait for alarm
    //
    printf("waiting for alarm to ring...\n");
    CHECK(read(fd, &data, sizeof(data)));

    //
    // get time after alarm
    //
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("alarm rang at "); print_time(&time); printf("\n");

    //
    // disable alarm interrupts
    //
    CHECK(ioctl(fd, RTC_AIE_DISABLE, NULL));

    time.tm_min = 45;
    printf("RTC_TIME_SET\n");
    CHECK(ioctl(fd, RTC_TIME_SET, &time));
    CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("current time is "); print_datetime(&time); printf("\n");

    //
    // close RTC
    //
    ret = close(fd);
    assert(ret == 0);
}
