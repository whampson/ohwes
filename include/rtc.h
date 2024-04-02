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
 *         File: include/rtc.h
 *      Created: March 31, 2024
 *       Author: Wes Hampson
 *
 * Dallas Semiconductor DS12887 Real Time Clock
 * =============================================================================
 */

#ifndef __RTC_H
#define __RTC_H

#include <time.h>

int rtc_gettime(struct tm *tm);     // TODO: might be a good idea to use an
                                    // RTC-specific struct here and reserve the
                                    // C 'tm' struct for time calculated via the
                                    // PIT, since the PIT is more accurate.
                                    // Linux does this.
#endif // __RTC_H
