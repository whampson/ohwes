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
 *         File: src/i386/boot/bootldr.ld
 *      Created: July 5, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include "bootldr.h"

#define STAGE1_SIZE 512
#define STAGE2_SIZE (KERNEL_LOAD - STAGE2_BASE)
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

ASSERT(KERNEL_LOAD > STAGE2_BASE, "kernel must be loaded higher than stage 2!")

MEMORY
{
    STAGE1 (rwx) : ORIGIN = STAGE1_BASE, LENGTH = STAGE1_SIZE
    STAGE2 (rwx) : ORIGIN = STAGE2_BASE, LENGTH = STAGE2_SIZE
}

SECTIONS
{
    .stage1 : ALIGN(SECTOR_SIZE)
    {
        *(.stage1)
        . = STAGE1_SIZE - 2;
        SHORT(STAGE1_MAGIC);
    } > STAGE1 =FILL_CHAR

    ASSERT(SIZEOF(.stage1) % SECTOR_SIZE == 0, ".stage1 invalid alignment!")
    ASSERT(SIZEOF(.stage1) == STAGE1_SIZE, ".stage1 too big!")

    .stage2 : ALIGN(SECTOR_SIZE)
    {
        *(.stage2)
        . = ALIGN(SECTOR_SIZE) - 4;
        LONG(STAGE2_MAGIC);
    } > STAGE2 =FILL_CHAR

    ASSERT(SIZEOF(.stage2) % SECTOR_SIZE == 0, ".stage2 invalid alignment!")
    ASSERT(SIZEOF(.stage2) + STAGE2_BASE < KERNEL_LOAD, ".stage2 too big!")
}
