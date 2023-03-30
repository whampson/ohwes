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
 *         File: include/stdarg.h
 *      Created: December 13, 2020
 *       Author: Wes Hampson
 *       Module: C Standard Library (C99)
 * =============================================================================
 */

/* Status: DONE */

#ifndef __STDARG_H
#define __STDARG_H

typedef void *va_list;

#define va_start(list,param) \
    (void) ((list) = ((char *) &(param) + sizeof(param)))

#define va_arg(list,type) \
    ((type *) ((list) = ((char *) (list) + sizeof(type))))[-1]

#define va_end(list) \
    (list = (void *) 0)

#define va_copy(dest,src) \
    (dest = src)

#endif /* __STDARG_H */
