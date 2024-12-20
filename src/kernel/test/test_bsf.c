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
 *         File: kernel/test/test_bsf.c
 *      Created: December 20, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <test.h>
#include <i386/bitops.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>

void test_bsf(void)
{
    DECLARE_TEST("bit scan forward");

    unsigned char bits[8];
    zeromem(bits, sizeof(bits));

    // all zeros...
    VERIFY_IS_TRUE(bit_scan_forward(bits, sizeof(bits)) == -1);

    // lsb == 1
    bits[0] = 1;
    VERIFY_IS_TRUE(bit_scan_forward(bits, sizeof(bits)) == 0);

    // lsb != 1
    bits[0] = 2;
    VERIFY_IS_TRUE(bit_scan_forward(bits, sizeof(bits)) == 1);
    bits[0] = 0;

    // msb == 1
    bits[7] = 0x80;
    VERIFY_IS_TRUE(bit_scan_forward(bits, sizeof(bits)) == 63);
}
