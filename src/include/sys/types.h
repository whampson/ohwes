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
 *         File: src/include/sys/types.h
 *      Created: April 3, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __SYS_TYPES_H
#define __SYS_TYPES_H

typedef unsigned int dev_t;

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef __SSIZE_T_DEFINED
#define __SSIZE_T_DEFINED
typedef signed long ssize_t;
#endif

#endif // __SYS_TYPES_H
