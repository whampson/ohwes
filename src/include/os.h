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
 *         File: kernel/include/os.h
 *      Created: September 18, 2023
 *       Author: Wes Hampson
 *       Module: System Includes
 * =============================================================================
 */

#ifndef __OS_H
#define __OS_H

#include <stdio.h>
#include <compiler.h>

#define dbgbrk()    __asm__ volatile ("int $3")
#define halt()      __asm__ volatile (".%=: hlt; jmp .%=" : : )

#define panic(...)                  \
do {                                \
    printf("panic: " __VA_ARGS__);  \
    halt();                         \
} while (0)

#endif // __OS_H
