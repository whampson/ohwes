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
 *         File: include/drivers/vga_cntl.h
 *      Created: December 13, 2020
 *       Author: Wes Hampson
 *
 * VGA controller interface. A lot of register and port information can be found
 * here: http://www.osdever.net/FreeVGA/home.htm
 * =============================================================================
 */

#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>
#include <drivers/vga_cntl.h>

static inline uint16_t VgaGetCursorPos()
{
    uint8_t cursorLocHi, cursorLocLo;
    cursorLocHi = VgaCntlCrtcRead(VGA_REG_CRTC_CL_HI);
    cursorLocLo = VgaCntlCrtcRead(VGA_REG_CRTC_CL_LO);

    return (cursorLocHi << 8) | cursorLocLo;
}

static inline void VgaSetCursorPos(uint16_t pos)
{
    VgaCntlCrtcWrite(VGA_REG_CRTC_CL_HI, pos >> 8);
    VgaCntlCrtcWrite(VGA_REG_CRTC_CL_LO, pos & 0xFF);
}

#endif /* _VGA_H */