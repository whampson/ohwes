#include <drivers/vga.h>
#include <nb/console.h>

void con_init(void)
{
    vga_init();
}

void blink_off(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_REG_ATTR_MODE);
    modectl &= ~VGA_FLD_ATTR_MODE_BLINK;
    vga_attr_write(VGA_REG_ATTR_MODE, modectl);
}

void blink_on(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_REG_ATTR_MODE);
    modectl |= VGA_FLD_ATTR_MODE_BLINK;
    vga_attr_write(VGA_REG_ATTR_MODE, modectl);
}

void hide_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_REG_CRTC_CSS);
    css |= VGA_FLD_CRTC_CSS_CD;
    vga_crtc_write(VGA_REG_CRTC_CSS, css);
}

void show_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_REG_CRTC_CSS);
    css &= ~VGA_FLD_CRTC_CSS_CD;
    vga_crtc_write(VGA_REG_CRTC_CSS, css);
}

uint16_t get_cursor_pos(void)
{
    uint8_t poshi, poslo;
    poshi = vga_crtc_read(VGA_REG_CRTC_CL_HI);
    poslo = vga_crtc_read(VGA_REG_CRTC_CL_LO);
    return (poshi << 8) | poslo;
}

void set_cursor_pos(uint16_t pos)
{
    vga_crtc_write(VGA_REG_CRTC_CL_HI, pos >> 8);
    vga_crtc_write(VGA_REG_CRTC_CL_LO, pos & 0xFF);
}

uint16_t get_cursor_shape(void)
{
    uint8_t shapehi, shapelo;
    shapelo = vga_crtc_read(VGA_REG_CRTC_CSS) & VGA_FLD_CRTC_CSS_CSS;
    shapehi = vga_crtc_read(VGA_REG_CRTC_CSE) & VGA_FLD_CRTC_CSE_CSE;
    return (shapehi << 8) | shapelo;
}

void set_cursor_shape(uint8_t start, uint8_t end)
{
    vga_crtc_write(VGA_REG_CRTC_CSS, start & VGA_FLD_CRTC_CSS_CSS);
    vga_crtc_write(VGA_REG_CRTC_CSE, end   & VGA_FLD_CRTC_CSE_CSE);
}
