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
 *         File: src/include/kernel/dpc.h
 *      Created: September 11, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __DPC_H
#define __DPC_H

//
// Deferred Procedure Calls
//
// DPCs allow an interrupt service routine to offload work that would otherwise
// slow the servicing of high-priority device interrupts. DPCs are serviced
// after any pending interrupt are handled (EOI is sent to PIC), so it's
// important to remember that DPCs ARE RUN WITH INTERRUPTS ENABLED.
//

// DPC function decorator, used to indicate to the reader that the function is
// not meant to be called directly.
#define __dpc

// A DPC function prototype.
typedef void __dpc (dpcfn_t)(void *arg);

//
// DPCs may be scheduled via `schedule_dpc()` from any context. The callback
// runs later, on the interrupt return path, with interrupts enabled.
//
// DPC structure must be statically-allocated; object must exist in a later
// scope.
//
struct dpc {
    dpcfn_t *fn;
    void *arg;

    struct dpc *_next;
};

//
// Add a Deferred Procedure Call to the end of the current DPC chain.
//
// DPCs are run synchronously at the end of a device IRQ, with interrupts
// enabled. Any new DPCs queued during DPC execution will be executed after the
// next interrupt cycle.
//
// @param dpc - statically-allocated DPC object
// @param fn  - the procedure to call at a later time
// @param arg - an argument to pass to the deferred procedure
//
// @return false if the DPC is already scheduled, or the arguments are malformed
//
bool schedule_dpc(struct dpc *dpc, dpcfn_t *fn, void *arg);

//
// Check whether any DPCs are currently pending.
//
bool dpc_pending(void);

//
// Execute all queued DPCs and simultaneously drain the DPC queue.
//
void dpc_run(void);

#endif  // __DPC_H
