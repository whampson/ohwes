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

#ifdef DEBUG

extern int g_test_crash_kernel;

#define gpfault()                                   \
({                                                  \
    __asm__ volatile ("lidt 0");                    \
})

#define pgfault()                                   \
({                                                  \
    *((volatile int *) NULL) = 0xBAADC0D3;          \
})                                                  \

#define divzero()                                   \
({                                                  \
    __asm__ volatile ("idiv %0" :: "a"(0), "b"(0)); \
})

#define softnmi()                                   \
({                                                  \
    __asm__ volatile ("int $2");                    \
})

#define dbgbrk()                                    \
({                                                  \
    __asm__ volatile ("int $3");                    \
})

#define testint()                                   \
({                                                  \
    __asm__ volatile ("int $69");                   \
})

#endif // DEBUG

#endif // __DEBUG_H
