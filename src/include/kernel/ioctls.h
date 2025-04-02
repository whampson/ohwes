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
 *         File: include/kernel/ioctls.h
 *      Created: April 2, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __IOCTLS_H
#define __IOCTLS_H

#include <kernel/ioctl.h>
#include <kernel/termios.h>

//
// IOCTL class codes
//
#define _IOC_TTY            'T'     // TTY IOCTL code
#define _IOC_RTC            'R'     // RTC IOCTL code

//
// TTY IOCTL functions
//
#define TCGETS              _IOCTL_R(_IOC_TTY,0x01,struct termios *)            // Set termios
#define TCSETS              _IOCTL_W(_IOC_TTY,0x02,const struct termios *)      // Get termios
#define TIOCMGET            _IOCTL_R(_IOC_TTY,0x03,int *)                       // Get modem status bits
#define TIOCMSET            _IOCTL_W(_IOC_TTY,0x04,const int *)                 // Set modem status bits

//
// RTC IOCTL functions
//
#define RTC_IRQP_ENABLE     _IOCTL  (_IOC_RTC,0x01)                   // Periodic Interrupt Enable
#define RTC_IRQP_DISABLE    _IOCTL  (_IOC_RTC,0x02)                   // Periodic Interrupt Disable
#define RTC_IRQP_GET        _IOCTL_R(_IOC_RTC,0x03,char)              // Get Periodic Interrupt Rate
#define RTC_IRQP_SET        _IOCTL_W(_IOC_RTC,0x04,char)              // Set Periodic Interrupt Rate
#define RTC_UPDATE_ENABLE   _IOCTL  (_IOC_RTC,0x05)                   // Time Update Interrupt Enable
#define RTC_UPDATE_DISABLE  _IOCTL  (_IOC_RTC,0x06)                   // Time Update Interrupt Disable
#define RTC_TIME_GET        _IOCTL_R(_IOC_RTC,0x07,struct rtc_time)   // Get RTC Time
#define RTC_TIME_SET        _IOCTL_W(_IOC_RTC,0x08,struct rtc_time)   // Set RTC Time
#define RTC_ALARM_ENABLE    _IOCTL  (_IOC_RTC,0x09)                   // Alarm Interrupt Enable
#define RTC_ALARM_DISABLE   _IOCTL  (_IOC_RTC,0x0A)                   // Alarm Interrupt Disable
#define RTC_ALARM_GET       _IOCTL_R(_IOC_RTC,0x0B,struct rtc_time)   // Get Alarm Time
#define RTC_ALARM_SET       _IOCTL_W(_IOC_RTC,0x0C,struct rtc_time)   // Set Alarm Time


#endif // __IOCTLS_H
