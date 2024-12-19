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
 *         File: src/i386/boot/boot.ld
 *      Created: July 5, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

MEMORY
{
    STAGE1 (rwx)    : ORIGIN = 0x7C00, LENGTH = 512
    STAGE2 (rwx)    : ORIGIN = 0x7E00, LENGTH = 0x8200
}

SECTIONS
{
    .stage1 :
    {
        *(.stage1)
    } > STAGE1

    .stage2 :
    {
        *(.stage2)
    } > STAGE2
}
