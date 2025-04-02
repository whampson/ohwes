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
 *         File: src/include/kernel/rtc.h
 *      Created: March 31, 2024
 *       Author: Wes Hampson
 *
 * Dallas Semiconductor DS12887 Real Time Clock
 * =============================================================================
 */

#ifndef __RTC_H
#define __RTC_H

#include <kernel/ioctl.h>

//
// RTC time
//
// Mirrors 'struct tm' in C standard library.
//
struct rtc_time {
    int tm_sec;     // seconds after the minute - [0, 60] (allows leapsecond)
    int tm_min;     // minutes after the hour - [0, 59]
    int tm_hour;    // hours since midnight - [0, 23]
    int tm_mday;    // day of the month - [1, 31]
    int tm_mon;     // months since January - [0, 11]
    int tm_year;    // years since 1900
    int tm_wday;    // (unused) days since Sunday - [0, 6]
    int tm_yday;    // (unused) days since January 1 - [0, 365]
    int tm_isdst;   // (unused) DST flag: >0 = DST in effect, 0 = no DST, <0 = no data
};

//
// Periodic Interrupt Rates
//
#define RTC_RATE_OFF    0
#define RTC_RATE_8192Hz 0x3
#define RTC_RATE_4096Hz 0x4
#define RTC_RATE_2048Hz 0x5
#define RTC_RATE_1024Hz 0x6
#define RTC_RATE_512Hz  0x7
#define RTC_RATE_256Hz  0x8
#define RTC_RATE_128Hz  0x9
#define RTC_RATE_64Hz   0xA
#define RTC_RATE_32Hz   0xB
#define RTC_RATE_16Hz   0xC
#define RTC_RATE_8Hz    0xD
#define RTC_RATE_4Hz    0xE
#define RTC_RATE_2Hz    0xF

#define rate2hz(r)      (32768 >> ((r) - 1))

#endif // __RTC_H
