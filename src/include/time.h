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
 *         File: include/time.h
 *      Created: March 31, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __TIME_H
#define __TIME_H

struct tm {
    int tm_sec;     // seconds after the minute - [0, 60] (allows leapsecond)
    int tm_min;     // minutes after the hour - [0, 59]
    int tm_hour;    // hours since midnight - [0, 23]
    int tm_mday;    // day of the month - [1, 31]
    int tm_mon;     // months since January - [0, 11]
    int tm_year;    // years since 1900
    int tm_wday;    // days since Sunday - [0, 6]
    int tm_yday;    // days since January 1 - [0, 365]
    int tm_isdst;   // DST flag: >0 = DST in effect, 0 = no DST, <0 = no data
};

#endif // __TIME_H
