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
 *         File: include/cpu.h
 *      Created: January 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <x86.h>

struct tss * get_tss(struct tss *tss);
struct x86_desc * get_seg_desc(uint16_t segsel);

extern __fastcall void _thunk_except00h(void);
extern __fastcall void _thunk_except01h(void);
extern __fastcall void _thunk_except02h(void);
extern __fastcall void _thunk_except03h(void);
extern __fastcall void _thunk_except04h(void);
extern __fastcall void _thunk_except05h(void);
extern __fastcall void _thunk_except06h(void);
extern __fastcall void _thunk_except07h(void);
extern __fastcall void _thunk_except08h(void);
extern __fastcall void _thunk_except09h(void);
extern __fastcall void _thunk_except0Ah(void);
extern __fastcall void _thunk_except0Bh(void);
extern __fastcall void _thunk_except0Ch(void);
extern __fastcall void _thunk_except0Dh(void);
extern __fastcall void _thunk_except0Eh(void);
extern __fastcall void _thunk_except0Fh(void);
extern __fastcall void _thunk_except10h(void);
extern __fastcall void _thunk_except11h(void);
extern __fastcall void _thunk_except12h(void);
extern __fastcall void _thunk_except13h(void);
extern __fastcall void _thunk_except14h(void);
extern __fastcall void _thunk_except15h(void);
extern __fastcall void _thunk_except16h(void);
extern __fastcall void _thunk_except17h(void);
extern __fastcall void _thunk_except18h(void);
extern __fastcall void _thunk_except19h(void);
extern __fastcall void _thunk_except1Ah(void);
extern __fastcall void _thunk_except1Bh(void);
extern __fastcall void _thunk_except1Ch(void);
extern __fastcall void _thunk_except1Dh(void);
extern __fastcall void _thunk_except1Eh(void);
extern __fastcall void _thunk_except1Fh(void);
extern __fastcall void _thunk_irq00h(void);
extern __fastcall void _thunk_irq01h(void);
extern __fastcall void _thunk_irq02h(void);
extern __fastcall void _thunk_irq03h(void);
extern __fastcall void _thunk_irq04h(void);
extern __fastcall void _thunk_irq05h(void);
extern __fastcall void _thunk_irq06h(void);
extern __fastcall void _thunk_irq07h(void);
extern __fastcall void _thunk_irq08h(void);
extern __fastcall void _thunk_irq09h(void);
extern __fastcall void _thunk_irq0Ah(void);
extern __fastcall void _thunk_irq0Bh(void);
extern __fastcall void _thunk_irq0Ch(void);
extern __fastcall void _thunk_irq0Dh(void);
extern __fastcall void _thunk_irq0Eh(void);
extern __fastcall void _thunk_irq0Fh(void);
extern __fastcall void _thunk_syscall(void);
extern __fastcall void _thunk_test(void);