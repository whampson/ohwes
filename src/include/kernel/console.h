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
 *         File: include/kernel/console.h
 *      Created: May 12, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <kernel/device.h>

/**
 * Dump the klog buffer to this console when registered.
 */
#define _CONSOLE_FLAG_PRINTBUF  (1 << 0)

/**
 * Represents a console device.
 * Similar to Linux console.
 */
struct console {
    /**
     * Friendly name for this console.
     */
    char *name;

    /**
     * Console number. Can be used by driver to identify TTY associated with console.
     */
    int number;

    /**
     * Console flags. See `_CONSOLE_FLAG_*`.
     */
    int flags;

    /**
     * Gets the console device ID.
     */
    dev_t (*device)(struct console *);

    /**
     * Initializes the console device.
     * @return `false` if initialization unsuccessful
     */
    bool (*init)(struct console *);

    /**
     * Writes a character buffer to the console's output stream.
     * @return the number of characters written
     */
    ssize_t (*write)(struct console *, const char *buf, size_t count);

    /**
     * Reads a character from the console's input stream.
     * @return the character read
     */
    int (*read_char)(struct console *);

    // -- Not required to be populated by registrant --
    struct console *next;   // next console in list
};

/**
 * Linked-list of registered consoles.
 */
extern struct console *g_consoles;

/**
 * Registers a console device, then calls `init` on the console.
 * If the console is already registered, no operation is performed.
 *
 * @param cons console struct to register
 * @param flags console flags
 * @return `false` if the console structure is malformed or failed initialization
 */
bool register_console(struct console *cons);

/**
 * Removes a console from the registered console list.
 * @return `false` if the specified console was never registered
 */
bool unregister_console(struct console *cons);     // TODO: cons->destroy()? flush buffers, etc.

#endif // __CONSOLE_H
