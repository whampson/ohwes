/* =============================================================================
 * Copyright (C) 2023 Wes Hampson. All Rights Reserved.
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
 *         File: boot/boot.h
 *      Created: March 21, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef _BOOT_H
#define _BOOT_H

#define IDT_BASE                0x0000
#define IDT_SIZE                (256*8)

#define GDT_BASE                (IDT_BASE+IDT_SIZE)
#define GDT_SIZE                (8*8)

#define LDT_BASE                (GDT_BASE+GDT_SIZE)
#define LDT_SIZE                (2*8)

#define TSS_BASE                (LDT_BASE+LDT_SIZE)
#define TSS_SIZE                (108)

#define MEMMAP_BASE             0x1000

#define A20METHOD_NONE          0       /* A20 already enabled (emulators only) */
#define A20METHOD_KEYBOARD      1       /* A20 enabled via PS/2 keyboard controller */
#define A20METHOD_PORT92h       2       /* A20 enabled via IO port 92h */
#define A20METHOD_BIOS          3       /* A20 enabled via BIOS INT=15h,AX=2401h */

#endif  // _BOOT_H
