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
 *         File: include/debug.h
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#define gpfault()                       \
({                                      \
    __asm__ volatile ("int $69");       \
})

#define divzero()                       \
({                                      \
    volatile int a = 1;                 \
    volatile int b = 0;                 \
    volatile int c = a / b;             \
    (void) c;                           \
})

#define dbgbrk()                        \
({                                      \
    __asm__ volatile ("int $3");        \
})

#endif // __DEBUG_H