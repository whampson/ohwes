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
 *         File: include/assert.h
 *      Created: December 26, 2023
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/error (C11)
 * =============================================================================
 */

#ifndef __ASSERT_H
#define __ASSERT_H

extern __noreturn void kpanic(const char *, ...);
#define panic(...) kpanic(__VA_ARGS__)

#ifdef DEBUG
#define assert(x) \
do { \
    if (!(x)) { \
        panic("%s:%d assertion failed: " #x "\n", __FILE__, __LINE__); \
    } \
} while (0)
#else
#define assert(x) (void) 0
#endif

#define static_assert _Static_assert

#endif  // __ASSERT_H
