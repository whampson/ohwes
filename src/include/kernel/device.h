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
 *         File: src/include/kernel/device.h
 *      Created: August 26, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __DEVICE_H
#define __DEVICE_H

#include <sys/types.h>

#define _DEV_MAJ(id)        ((id) & 0xFFFF)
#define _DEV_MIN(id)        (((id) >> 16) & 0xFFFF)

#define __mkdev(maj,min)    ((((min) & 0xFFFF) << 16) | ((maj) & 0xFFFF))

#define TTY_MAJOR           1

#endif // __DEVICE_H
