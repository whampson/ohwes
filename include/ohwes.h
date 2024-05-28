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
 *         File: include/ohwes.h
 *      Created: January 7, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __OHWES_H
#define __OHWES_H

#include <config.h>
#include <interrupt.h>
#include <x86.h>

#ifndef __ASSEMBLER__

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <panic.h>
#include <debug.h>

#define PAGE_SHIFT                      12
#define PAGE_SIZE                       (1 << PAGE_SHIFT)

#define LARGE_PAGE_SHIFT                22
#define LARGE_PAGE_SIZE                 (1 << LARGE_PAGE_SHIFT)

#define OS_NAME                         "OH-WES"
#define OS_VERSION                      "0.1"
#define OS_MONIKER                      "Ronnie Raven"
#define OS_BUILDDATE                    __DATE__ " " __TIME__

extern void idle(void);                 // see entry.S
extern void init(void);                 // see init.c

void timer_sleep(int millis);           // see timer.c
void pcspk_beep(int freq, int millis);  // see timer.c

#define beep(f,ms)                      pcspk_beep(f, ms)   // beep at frequency for millis (nonblocking)
#define sleep(ms)                       timer_sleep(ms)     // spin for millis (blocking)

#define die()                           for (;;)            // spin forever, satisfies __noreturn
#define spin(cond)                      while (cond) { }    // spin while cond == true, TODO: THIS NEEDS TO HAVE A TIMEOUT!!

#define zeromem(p,n)                    memset(p, 0, n)
#define kprint(...)                     printf(__VA_ARGS__)

#define has_flag(x,f)                   (((x)&(f))==(f))
#define countof(x)                      (sizeof(x)/sizeof(x[0]))
#define div_round(n,d)                  (((n)<0)==((d)<0)?(((n)+(d)/2)/(d)):(((n)-(d)/2)/(d)))

#define reboot()                        \
do {                                    \
    *((uint16_t *) 0x0472) = 0x1234;    \
    ps2_cmd(PS2_CMD_SYSRESET);          \
    die();                              \
} while (0)

#define swap(a,b)                       \
do {                                    \
    (a) ^= (b);                         \
    (b) ^= (a);                         \
    (a) ^= (b);                         \
} while(0)

#define getpl()                         \
({                                      \
    struct segsel cs;                   \
    store_cs(cs);                       \
    cs.rpl;                             \
})

#define STRINGIFY(x)                    # x
#define CONCAT(a,b)                     a ## b
#define HASNO(cond)                     ((cond)?"has":"no")
#define YN(cond)                        ((cond)?"yes":"no")
#define ONOFF(cond)                     ((cond)?"on":"off")
#define PLURAL(n,a)                     (((n)==1)?a:a "s")
#define PLURAL2(n,a,b)                  (((n)==1)?a:b)

#endif // __ASSEMBLER__

#endif // __OHWES_H
