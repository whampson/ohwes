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
 *         File: include/math.h
 *      Created: October 7, 2025
 *       Author: Wes Hampson
 *
 * =============================================================================
 */

#ifndef __MATH_H
#define __MATH_H

#define min(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a < __b ? __a : __b; })

#define max(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a > __b ? __a : __b; })

#define swap(a,b) ({ \
    ((a) == (b)) || \
    ((a) ^= (b), \
     (b) ^= (a), \
     (a) ^= (b)); })

#define div_round(n,d)  (((n)<0)==((d)<0)?(((n)+(d)/2)/(d)):(((n)-(d)/2)/(d)))
#define div_ceil(n,d)   (((n)+(d)-1)/(d))

#endif // __MATH_H
