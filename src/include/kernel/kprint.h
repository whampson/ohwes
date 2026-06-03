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
 *         File: include/kernel/kprint.h
 *      Created: May 14, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __KPRINT_H
#define __KPRINT_H

#include <stdarg.h>

// inspired by Linux printk log levels
#define KLOG_FATAL      "<0>"   /* system is unusable */
#define KLOG_ERROR      "<1>"   /* error conditions */
#define KLOG_ALERT      "<2>"   /* urgent warning */
#define KLOG_WARN       "<3>"   /* warning conditions */
#define KLOG_INFO       "<4>"   /* general information messages */
#define KLOG_DEBUG      "<5>"   /* debug messages */
#define KLOG_CONT       "<c>"   /* continue previous line */
#define KLOG_DEFAULT    "<d>"   /* assume default log level */

// printf to console
__format_printf(1, 2)
extern int kprint(const char *fmt, ...);

__format_printf(1, 0)
extern int vkprint(const char *fmt, va_list args);

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_fatal(fmt, ...) \
    kprint(KLOG_FATAL pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...) \
    kprint(KLOG_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_error(fmt, ...) \
    kprint(KLOG_ERROR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
    kprint(KLOG_WARN  pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...) \
    kprint(KLOG_INFO  pr_fmt(fmt), ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
    kprint(KLOG_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_cont(fmt, ...) \
    kprint(KLOG_CONT  pr_fmt(fmt), ##__VA_ARGS__)

#define pr_warning  pr_warn
#define pr_err      pr_error
#define pr_dbg      pr_debug

#endif // __KPRINT_H
