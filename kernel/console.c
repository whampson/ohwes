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
 *         File: kernel/console.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <os/console.h>

char * const g_VgaBuf = (char * const) 0xB8000;

uint16_t console_get_cursor()
{
    uint8_t cursorLocHi, cursorLocLo;
    cursorLocHi = vga_crtc_read(VGA_REG_CRTC_CL_HI);
    cursorLocLo = vga_crtc_read(VGA_REG_CRTC_CL_LO);

    return (cursorLocHi << 8) | cursorLocLo;
}

void console_set_cursor(uint16_t pos)
{
    vga_crtc_write(VGA_REG_CRTC_CL_HI, pos >> 8);
    vga_crtc_write(VGA_REG_CRTC_CL_LO, pos & 0xFF);
}

void console_write(char c)
{
    uint16_t pos = console_get_cursor();

    switch (c)
    {
        case '\r':
            pos -= (pos % 80);
            break;
        case '\n':
            pos += (80 - (pos % 80));   // TODO: CRLF vs LF
            break;

        default:
            g_VgaBuf[(pos++) << 1] = c;
            break;
    }

    console_set_cursor(pos);
}
