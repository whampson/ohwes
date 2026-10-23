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
 *
 * Console interface for kernel message logging.
 * =============================================================================
 */

#ifndef __KPRINT_H
#define __KPRINT_H

#include <stdarg.h>

// inspired by Linux printk log levels
#define KLOG_LEVEL_FATAL    "<0>"   // system is unusable
#define KLOG_LEVEL_ERROR    "<1>"   // error condition
#define KLOG_LEVEL_ALERT    "<2>"   // urgent warning
#define KLOG_LEVEL_WARN     "<3>"   // warning condition
#define KLOG_LEVEL_INFO     "<4>"   // general information
#define KLOG_LEVEL_DEBUG    "<5>"   // debug messages
#define KLOG_LEVEL_CONT     "<c>"   // continue message
#define KLOG_LEVEL_DEFAULT  "<d>"   // use kernel's set log level
#define MAX_KLOG_LEVEL      5

typedef unsigned int klog_level_t;

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define _KLOG_FATAL(fmt) KLOG_LEVEL_FATAL pr_fmt(fmt)
#define _KLOG_ERROR(fmt) KLOG_LEVEL_ERROR pr_fmt(fmt)
#define _KLOG_ALERT(fmt) KLOG_LEVEL_ALERT pr_fmt(fmt)
#define _KLOG_WARN(fmt)  KLOG_LEVEL_WARN  pr_fmt(fmt)
#define _KLOG_INFO(fmt)  KLOG_LEVEL_INFO  pr_fmt(fmt)
#define _KLOG_DEBUG(fmt) KLOG_LEVEL_DEBUG pr_fmt(fmt)
#define _KLOG_CONT(fmt)  KLOG_LEVEL_CONT  fmt

/**
 * `printf()` for the kernel. This is the kernel's message logging interface.
 * All log messages get sent to an internal ring buffer and, depending on the
 * current log level, written to all registered consoles. Only messages with an
 * equal to or higher importance than the kernel's set log level will be printed
 * to the consoles, however all messages will be retained in the internal ring
 * buffer (until it gets overwritten, see `CONFIG_KLOG_RING_SIZE`).
 *
 * Indicate a message's importance by prepending one of the  `KLOG_LEVEL_`
 * macros to the message, or by calling one of the log level macros below.
 * Messages written without a level prefix are logged at `CONFIG_KLOG_LEVEL`.
 * Control the kernel's current log level using `Alt+SysRq+<0-9>`. Log messages
 * are always terminated with `\n` unless `KLOG_LEVEL_CONT` is used.
 */
extern int kprint(const char *fmt, ...)
    __format_printf(1, 2);

/**
 * `vprintf()` for the kernel. See `kprint()`.
 */
extern int vkprint(const char *fmt, va_list args)
    __format_printf(1, 0);

/**
 * Macros for printing messages with a specific log level.
 */

#define pr_fatal(fmt, ...) kprint(_KLOG_FATAL(fmt), ##__VA_ARGS__)
#define pr_error(fmt, ...) kprint(_KLOG_ERROR(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...) kprint(_KLOG_ALERT(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...)  kprint(_KLOG_WARN(fmt),  ##__VA_ARGS__)
#define pr_info(fmt, ...)  kprint(_KLOG_INFO(fmt),  ##__VA_ARGS__)
#define pr_debug(fmt, ...) kprint(_KLOG_DEBUG(fmt), ##__VA_ARGS__)
#define pr_cont(fmt, ...)  kprint(_KLOG_CONT(fmt),  ##__VA_ARGS__)

/**
 * Defer all log message writes to a ring buffer until `resume_klog()` is
 * called.
 *
 * WARNING: ring buffer can wrap and overwrrite! Do not defer for a long time.
 * See `CONFIG_KLOG_RING_SIZE`.
 */
void suspend_klog(void);

/**
 * Flush all suspended kernel log messages and allow new messages to be written
 * to all consoles immediately.
 */
void resume_klog(void);

#endif // __KPRINT_H
