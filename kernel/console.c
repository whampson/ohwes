#include <os/console.h>

uint16_t con_get_cursor()
{
    uint8_t cursorLocHi, cursorLocLo;
    cursorLocHi = vga_crtc_read(VGA_REG_CRTC_CL_HI);
    cursorLocLo = vga_crtc_read(VGA_REG_CRTC_CL_LO);

    return (cursorLocHi << 8) | cursorLocLo;
}

void con_set_cursor(uint16_t pos)
{
    vga_crtc_write(VGA_REG_CRTC_CL_HI, pos >> 8);
    vga_crtc_write(VGA_REG_CRTC_CL_LO, pos & 0xFF);
}
