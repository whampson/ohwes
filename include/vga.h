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
 *         File: include/vga.h
 *      Created: December 13, 2020
 *       Author: Wes Hampson
 *
 * VGA controller interface.
 *
 * http://www.osdever.net/FreeVGA/home.htm
 * =============================================================================
 */

#ifndef __VGA_H
#define __VGA_H

#include <assert.h>
#include <stdint.h>

/**
 * CRT Controller Registers
 * http://www.osdever.net/FreeVGA/vga/crtcreg.htm
 */
// CRTC Port Addresses controled by IOAS bit in Miscellaneous Output Register
#define VGA_CRTC_PORT_ADDR          0x3D4   /* CRTC Address Port */
#define VGA_CRTC_PORT_DATA          0x3D5   /* CRTC Data Port */
#define VGA_CRTC_PORT_ADDR_MONO     0x3B4   /* CRTC Address Port (Monochrome) */
#define VGA_CRTC_PORT_DATA_MONO     0x3B5   /* CRTC Data Port (Monochrome) */

#define VGA_CRTC_REG_HT             0x00    /* Horizontal Total Register */
#define VGA_CRTC_REG_HDE            0x01    /* End Horizontal Display Register */
#define VGA_CRTC_REG_HBS            0x02    /* Start Horizontal Blanking Register */
#define VGA_CRTC_REG_HBE            0x03    /* End Horizontal Blanking Register */
#define VGA_CRTC_REG_HRS            0x04    /* Start Horizontal Retrace Register */
#define VGA_CRTC_REG_HRE            0x05    /* End Horizontal Retrace Register */
#define VGA_CRTC_REG_VT             0x06    /* Vertical Total Register */
#define VGA_CRTC_REG_OF             0x07    /* Overflow Register */
#define VGA_CRTC_REG_PRESCAN        0x08    /* Preset Row Scan Register */
#define VGA_CRTC_REG_MAXSCAN        0x09    /* Maximum Scan Line Register */
#define VGA_CRTC_REG_CSS            0x0A    /* Cursor Scan Line Start Register */
#define VGA_CRTC_REG_CSE            0x0B    /* Cursor Scan Line End Register */
#define VGA_CRTC_REG_ADDR_HI        0x0C    /* Start Address High Register */
#define VGA_CRTC_REG_ADDR_LO        0x0D    /* Start Address Low Register */
#define VGA_CRTC_REG_CL_HI          0x0E    /* Cursor Location High Register */
#define VGA_CRTC_REG_CL_LO          0x0F    /* Cursor Location Low Register */
#define VGA_CRTC_REG_VRS            0x10    /* Vertical Retrace Start Register */
#define VGA_CRTC_REG_VRE            0x11    /* Vertical Retrace End Register */
#define VGA_CRTC_REG_VDE            0x12    /* Vertical Display End Register */
#define VGA_CRTC_REG_OFFSET         0x13    /* Offset Register */
#define VGA_CRTC_REG_UNDERLINE      0x14    /* Underline Location Register */
#define VGA_CRTC_REG_VBS            0x15    /* Start Vertical Blanking Register */
#define VGA_CRTC_REG_VBE            0x16    /* End Vertical Blanking */
#define VGA_CRTC_REG_MODE           0x17    /* CRTC Mode Control Register */
#define VGA_CRTC_REG_LC             0x18    /* Line Compare Register */

/* Cursor Scan Line Start Register Fields */
#define VGA_CRTC_FLD_CSS_CSS_MASK   0x1F    /* Cursor Scan Line Start Field */
#define VGA_CRTC_FLD_CSS_CD_MASK    0x20    /* Cursor Disable Bit Field */

/* Cursor Scan Line End Register Fields */
#define VGA_CRTC_FLD_CSE_CSE_MASK   0x1F    /* Cursor Scan Line End Field */
#define VGA_CRTC_FLD_CSE_CSK_MASK   0x60    /* Cursor Skew Field */

/**
 * Graphics Registers
 * http://www.osdever.net/FreeVGA/vga/graphreg.htm
 */
#define VGA_GRFX_PORT_ADDR          0x3CE   /* Graphics Address Port */
#define VGA_GRFX_PORT_DATA          0x3CF   /* Graphics Data Port */

#define VGA_GRFX_REG_SR             0x00    /* Set/Reset Register */
#define VGA_GRFX_REG_ESR            0x01    /* Enable Set/Reset Register */
#define VGA_GRFX_REG_CCMP           0x02    /* Color Compare Register */
#define VGA_GRFX_REG_DR             0x03    /* Data Rotate Register */
#define VGA_GRFX_REG_RMS            0x04    /* Read Map Select Register */
#define VGA_GRFX_REG_MODE           0x05    /* Graphics Mode Register */
#define VGA_GRFX_REG_MISC           0x06    /* Miscellaneous Graphics Register */
#define VGA_GRFX_REG_CDC            0x07    /* Color Don't Care Register */
#define VGA_GRFX_REG_MASK           0x08    /* Bitmask Register */

/* Miscellaneous Graphics Register Fields */
#define VGA_GRFX_FLD_MISC_MMAP_MASK     0x0C    /* Memory Map Select */
#define VGA_GRFX_ENUM_MISC_MMAP_128K    0x00    /* 0xA0000-0xBFFFF */
#define VGA_GRFX_ENUM_MISC_MMAP_64K     0x01    /* 0xB0000-0xBFFFF */
#define VGA_GRFX_ENUM_MISC_MMAP_32K_LO  0x02    /* 0xB0000-0xB7FFF */
#define VGA_GRFX_ENUM_MISC_MMAP_32K_HI  0x03    /* 0xB8000-0xBFFFF */

/**
 * Attribute Controller Registers
 * http://www.osdever.net/FreeVGA/vga/attrreg.htm
 */
#define VGA_ATTR_PORT_ADDR          0x3C0   /* Attribute Address Port */
#define VGA_ATTR_PORT_DATA_R        0x3C1   /* Attribute Data Port (Read) */
#define VGA_ATTR_PORT_DATA_W        0x3C0   /* Attribute Data Port (Write) */

#define VGA_ATTR_REG_PL_0           0x00    /* Palette Register 0 */
#define VGA_ATTR_REG_PL_1           0x01    /* Palette Register 1 */
#define VGA_ATTR_REG_PL_2           0x02    /* Palette Register 2 */
#define VGA_ATTR_REG_PL_3           0x03    /* Palette Register 3 */
#define VGA_ATTR_REG_PL_4           0x04    /* Palette Register 4 */
#define VGA_ATTR_REG_PL_5           0x05    /* Palette Register 5 */
#define VGA_ATTR_REG_PL_6           0x06    /* Palette Register 6 */
#define VGA_ATTR_REG_PL_7           0x07    /* Palette Register 7 */
#define VGA_ATTR_REG_PL_8           0x08    /* Palette Register 8 */
#define VGA_ATTR_REG_PL_9           0x09    /* Palette Register 9 */
#define VGA_ATTR_REG_PL_A           0x0A    /* Palette Register 10 */
#define VGA_ATTR_REG_PL_B           0x0B    /* Palette Register 11 */
#define VGA_ATTR_REG_PL_C           0x0C    /* Palette Register 12 */
#define VGA_ATTR_REG_PL_D           0x0D    /* Palette Register 13 */
#define VGA_ATTR_REG_PL_E           0x0E    /* Palette Register 14 */
#define VGA_ATTR_REG_PL_F           0x0F    /* Palette Register 15 */
#define VGA_ATTR_REG_MODE           0x10    /* Attribute Mode Control Register */
#define VGA_ATTR_REG_OSC            0x11    /* Overscan Color Register */
#define VGA_ATTR_REG_CPE            0x12    /* Color Plane Enable Register */
#define VGA_ATTR_REG_HPP            0x13    /* Horizontal Pixel Panning Register */
#define VGA_ATTR_REG_CS             0x14    /* Color Select Register */

/* Attribute Address Register Fields */
#define VGA_ATTR_FLD_ADDR_ADDR      0x1F    /* Attribute Address Field */
#define VGA_ATTR_FLD_ADDR_PAS       0x20    /* Palette Address Source Field */

/* Attribute Mode Control Register Fields */
#define VGA_ATTR_FLD_MODE_ATGE      0x01    /* Attribute Controller Graphics Enable Field */
#define VGA_ATTR_FLD_MODE_MONO      0x02    /* Monochrome Emulation Field */
#define VGA_ATTR_FLD_MODE_LGE       0x04    /* Line Graphics Enable Field */
#define VGA_ATTR_FLD_MODE_BLINK     0x08    /* Blink Enable Field */
#define VGA_ATTR_FLD_MODE_PPM       0x20    /* Pixel Panning Mode Field */
#define VGA_ATTR_FLD_MODE_8BIT      0x40    /* 8-bit Color Enable Field */
#define VGA_ATTR_FLD_MODE_P54S      0x80    /* Palette Bits 5-4 Select Field */

/**
 * Sequencer Registers
 * http://www.osdever.net/FreeVGA/vga/seqreg.htm
 */
#define VGA_SEQR_PORT_ADDR          0x3C4   /* Sequencer Address Port */
#define VGA_SEQR_PORT_DATA          0x3C5   /* Sequencer Data Port */
#define VGA_SEQR_REG_RESET          0x00    /* Reset Register */
#define VGA_SEQR_REG_CLOCKING       0x01    /* Clocking Mode Register */
#define VGA_SEQR_REG_MASK           0x02    /* Map Mask Register */
#define VGA_SEQR_REG_CHMAP          0x03    /* Character Map Select Register */
#define VGA_SEQR_REG_MODE           0x04    /* Sequencer Memory Mode Register */

/**
 * Color Registers
 * http://www.osdever.net/FreeVGA/vga/colorreg.htm
 */
#define VGA_COLR_PORT_ADDR_RD_MODE  0x3C7   /* DAC Address Read Mode Port (Write-Only) */
#define VGA_COLR_PORT_ADDR_WR_MODE  0x3C8   /* DAC Address Write Mode Port (Read/Write) */
#define VGA_COLR_PORT_DATA          0x3C9   /* DAC Data Port (Read/Write) */
#define VGA_COLR_PORT_STATE         0x3C7   /* DAC State Port (Read-Only) */
/* TODO: color registers */

/**
 * External Registers
 * http://www.osdever.net/FreeVGA/vga/extreg.htm
 */
#define VGA_EXTL_PORT_MO_R          0x33C   /* Miscellaneous Output Port (Read) */
#define VGA_EXTL_PORT_MO_W          0x332   /* Miscellaneous Output Port (Write) */
#define VGA_EXTL_PORT_IS0           0x3C2   /* Input Status Port #0 */
#define VGA_EXTL_PORT_IS1           0x3DA   /* Input Status Port #1 */
#define VGA_EXTL_PORT_IS1_MONO      0x3BA   /* Input Status Port #1 (Monochrome) */

/* Miscellaneous Output Port Fields */
#define VGA_EXTL_FLD_MO_IOAS        0x01    /* Input/Output Address Select Field */
#define VGA_EXTL_FLD_MO_RAMEN       0x02    /* RAM Enable Field */
#define VGA_EXTL_FLD_MO_CS          0x0C    /* Clock Select Field */
#define VGA_EXTL_FLD_MO_OEP         0x20    /* Odd/Even Page Select Field */
#define VGA_EXTL_FLD_MO_HSYNCP      0x40    /* Horizontal Sync Polarity Field */
#define VGA_EXTL_FLD_MO_VSYNCP      0x80    /* Vertical Sync Polarity Field */

/**
 * Default Text Mode colors.
 */
enum vga_color {
    VGA_BLACK,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_YELLOW,
    VGA_WHITE,
};

/**
 * Text Mode character attribute.
 */
struct vga_attr {
    union {
        struct {
            uint8_t color_fg : 3;
            uint8_t bright   : 1;
            uint8_t color_bg : 3;
            uint8_t blink    : 1;   // not supported on all devices
        };
        struct {
            uint8_t fg : 4;
            uint8_t bg : 4;
        };
        uint8_t _value;
    };
};
static_assert(sizeof(struct vga_attr) == 1, "sizeof(struct vga_attr)");

/**
 * Text Mode character cell.
 */
struct vga_cell {
    union {
        struct {
            char ch;
            struct vga_attr attr;
        };
        uint16_t _value;
    };
};
static_assert(sizeof(struct vga_cell) == 2, "sizeof(struct vga_cell)");

/**
 * Disables the cursor blink effect.
 */
void vga_disable_blink(void);

/**
 * Enables the cursor blink effect.
 */
void vga_enable_blink(void);

/**
 * Disables the cursor.
 */
void vga_hide_cursor(void);

/**
 * Enables the cursor.
 */
void vga_show_cursor(void);

/**
 * Gets the current linear cursor position.
 * A value of 0 represents the top left corner of the display area.
 *
 * @return the cursor position
 */
uint16_t vga_get_cursor_pos(void);

/**
 * Sets the current linear cursor position.
 * A value of 0 represents the top left corner of the display area.
 *
 * @param pos the new linear cursor position
 */
void vga_set_cursor_pos(uint16_t pos);

/**
 * Gets the current cursor shape.
 * The cursor shape is defined as the area between two scan lines. A scan line
 * value of 0 represents the top of the current row. The maximum scan line is
 * determined by the character height (usually 15).
 *
 * @return the cursor shape, represented as a packed scan line tuple where the
 *         low byte contains the starting scan line and the high byte contains
 *         the ending scan line
 */
uint16_t vga_get_cursor_shape(void);

/**
 * Sets the cursor shape.
 * The cursor shape is defined as the area between two scan lines. A scan line
 * value of 0 represents the top of the current row. The maximum scan line is
 * determined by the character height (usually 15).
 *
 * @param start the scan line at which to begin drawing the cursor
 * @param end the scan line at which to stop drawing the cursor
 */
void vga_set_cursor_shape(uint8_t start, uint8_t end);

/**
 * Reads a CRT Controller register.
 *
 * @param reg one of VGA_CRTC_REG_*
 * @return the register value
 */
uint8_t vga_crtc_read(uint8_t reg);

/**
 * Writes a CRT Controller register.
 *
 * @param reg one of VGA_CRTC_REG_*
 * @param data the value to write
 */
void vga_crtc_write(uint8_t reg, uint8_t data);

/**
 * Reads a Graphics register.
 *
 * @param reg one of VGA_GRFX_REG_*
 * @return the register value
 */
uint8_t vga_grfx_read(uint8_t reg);

/**
 * Writes a Graphics register.
 *
 * @param reg one of VGA_GRFX_REG_*
 * @param data the value to write
 */
void vga_grfx_write(uint8_t reg, uint8_t data);

/**
 * Reads a Sequencer register.
 *
 * @param reg one of VGA_SEQR_REG_*
 * @return the register value
 */
uint8_t vga_seqr_read(uint8_t reg);

/**
 * Writes a Sequencer register.
 *
 * @param reg one of VGA_SEQR_REG_*
 * @param data the value to write
 */
void vga_seqr_write(uint8_t reg, uint8_t data);

/**
 * Reads an Attribute register.
 *
 * @param reg one of VGA_ATTR_REG_*
 * @return the register value
 */
uint8_t vga_attr_read(uint8_t reg);

/**
 * Writes an Attribute register.
 *
 * @param reg one of VGA_ATTR_REG_*
 * @param data the value to write
 */
void vga_attr_write(uint8_t reg, uint8_t data);

/**
 * Reads an External register.
 *
 * @param port one of VGA_EXTL_PORT_*
 * @return the register value
 */
uint8_t vga_extl_read(uint16_t port);

/**
 * Writes an External register.
 *
 * @param port one of VGA_EXTL_PORT_*
 * @param data the value to write
 */
void vga_extl_write(uint16_t port, uint8_t data);

#endif /* __VGA_H */