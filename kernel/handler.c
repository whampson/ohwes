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
 *         File: kernel/handler.c
 *      Created: April 4, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdio.h>
#include <hw/interrupt.h>

__fastcall void handle_exception(struct iframe *frame)
{
    printf("!!! exception: 0x%02x\n", frame->vecNum);
    __asm__ volatile (".1: jmp .1");
}

__fastcall void handle_irq(struct iframe *frame)
{
    printf("!!! irq: %d\n", ~frame->vecNum);
    __asm__ volatile (".2: jmp .2");
    IrqEnd(~frame->vecNum);
}

__fastcall void handle_syscall(struct iframe *frame)
{
    printf("!!! system call: %d\n", frame->eax);
    __asm__ volatile (".3: jmp .3");
}
