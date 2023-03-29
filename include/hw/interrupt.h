/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: include/hw/interrupt.h
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

/* WARNING: The functions defined in this file are tightly coupled to x86. */

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

/**
 * Clears the interrupt flag.
 */
static inline void cli()
{
    __asm__ volatile (
        "cli"
        :
        :
        : "cc"
    );
}

/**
 * Sets the interrupt flag.
 */
static inline void sti()
{
    __asm__ volatile (
        "sti"
        :
        :
        : "cc"
    );
}

/**
 * Backs up the EFLAGS register, then clears the interrupt flag.
 */
static inline void cli_save(uint32_t *pFlags)
{
    __asm__ volatile (
        "               \n\
        pushfl          \n\
        popl %0         \n\
        cli             \n\
        "
        : "=r"(*pFlags)
        :
        : "memory", "cc"
    );
}

/**
 * Restores the EFLAGS register.
 * If interrupts were previously enabled, this will also restore interrupts.
 */
static inline void restore_flags(uint32_t flags)
{
    __asm__ volatile (
        "               \n\
        pushl %0        \n\
        popfl           \n\
        "
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

#endif /* _INTERRUPT_H */
