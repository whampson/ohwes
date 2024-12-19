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
 *         File: src/include/kernel/ioctl.h
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __IOCTL_H
#define __IOCTL_H

#ifdef __KERNEL__
#include <string.h>

#define validate_user_address(addr,count)       \
do {                                            \
    /* TODO... */                               \
    /* Raise an exception or something if invalid... */ \
    if (!addr) {                                \
        panic("user supplied null address!");   \
    }                                           \
} while (0)

#define copy_to_user(u_dst,k_src,count)         \
do {                                            \
    validate_user_address(u_dst, count);        \
    memcpy(u_dst, k_src, count);                \
} while (0)

#define copy_from_user(k_dst,u_src,count)       \
do {                                            \
    validate_user_address(u_src, count);        \
    memcpy(k_dst, u_src, count);                \
} while (0)

#endif // __KERNEL__

//
// IOCTL number:
// +---+-------------+-------+-------+
// |dir|     size    | code  |  seq  |
// +---+-------------+-------+-------+
// 32  30            16      8       0
//
// [31:30]  dir: I/O direction; 0 = no I/O, 1 = read, 2 = write, 3 = read/write
// [29:16] size: size of argument buffer in bytes
// [15: 8] code: device class code
// [ 7: 0]  seq: command sequence number for device
//

// TODO: 64-bit number and argument ptr??

#define _IOCTL_SEQBITS          8
#define _IOCTL_CODEBITS         8
#define _IOCTL_SIZEBITS         14
#define _IOCTL_DIRBITS          2

#define _IOCTL_SEQSHIFT         0
#define _IOCTL_CODESHIFT        (_IOCTL_SEQSHIFT+_IOCTL_SEQBITS)
#define _IOCTL_SIZESHIFT        (_IOCTL_CODESHIFT+_IOCTL_CODEBITS)
#define _IOCTL_DIRSHIFT         (_IOCTL_SIZESHIFT+_IOCTL_SIZEBITS)

#define _IOCTL_SEQMASK          (((1<<_IOCTL_SEQBITS)-1)<<_IOCTL_SEQSHIFT)
#define _IOCTL_CODEMASK         (((1<<_IOCTL_CODEBITS)-1)<<_IOCTL_CODESHIFT)
#define _IOCTL_SIZEMASK         (((1<<_IOCTL_SIZEBITS)-1)<<_IOCTL_SIZESHIFT)
#define _IOCTL_DIRMASK          (((1<<_IOCTL_DIRBITS)-1)<<_IOCTL_DIRSHIFT)

//
// Direction Bits
//
#define _IOCTL_NOIO             0   // No I/O
#define _IOCTL_READ             1   // Read
#define _IOCTL_WRITE            2   // Write

//
// Define an IOCTL number.
//
#define _IOCTL(code,seq)                                        \
    ((((code) << _IOCTL_CODESHIFT) & _IOCTL_CODEMASK) |         \
    (((seq) << _IOCTL_SEQSHIFT) & _IOCTL_SEQMASK))

//
// Define a read-only IOCTL number.
//
#define _IOCTL_R(code,seq,type)                                 \
    (((_IOCTL_READ<<_IOCTL_DIRSHIFT)&_IOCTL_DIRMASK) |          \
    ((sizeof(type)<<_IOCTL_SIZESHIFT)&_IOCTL_SIZEMASK) |        \
    (_IOCTL(code, seq)))

//
// Define a write-only IOCTL number.
//
#define _IOCTL_W(code,seq,type)                                 \
    (((_IOCTL_WRITE<<_IOCTL_DIRSHIFT)&_IOCTL_DIRMASK) |         \
    ((sizeof(type)<<_IOCTL_SIZESHIFT)&_IOCTL_SIZEMASK) |        \
    (_IOCTL(code, seq)))

#endif // __IOCTL_H
