/* =============================================================================
 * Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/dpc.c
 *      Created: September 11, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/interrupt.h>
#include <kernel/dpc.h>

static struct dpc *s_dpc_head;
static struct dpc *s_dpc_tail;

bool dpc_pending(void)
{
    uint32_t flags;
    cli_save(flags);

    bool pending = (s_dpc_head != NULL);

    restore_flags(flags);
    return pending;
}

bool schedule_dpc(struct dpc *dpc, dpcfn_t *fn, void *arg)
{
    uint32_t flags;
    cli_save(flags);

    if (!dpc || !fn || (dpc->fn != NULL)) {
        restore_flags(flags);
        return false;
    }

    dpc->fn = fn;
    dpc->arg = arg;
    dpc->_next = NULL;

    if (s_dpc_tail) {
        s_dpc_tail->_next = dpc;
    }
    else {
        s_dpc_head = dpc;
    }
    s_dpc_tail = dpc;

    restore_flags(flags);
    return true;
}

void dpc_run(void)
{
    struct dpc *curr, *next;
    dpcfn_t *dpc; void *arg;
    uint32_t flags;

    // invalidate current DPC queue
    cli_save(flags);
    curr = s_dpc_head;
    s_dpc_head = NULL;
    s_dpc_tail = NULL;
    restore_flags(flags);

    while (curr) {
        next = curr->_next;
        dpc = curr->fn;
        arg = curr->arg;

        // invalidate DPC object; DPC may be rescheduled for next ISR
        curr->fn = NULL;
        curr->_next = NULL;
        curr = next;

        dpc(arg);
    }
}
