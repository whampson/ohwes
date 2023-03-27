#include "boot.h"

#include <stdint.h>
#include <drivers/vga.h>

char * const g_VgaBuf = (char * const) 0xB8000;

void puts(const char *s)
{
    uint16_t pos = VgaGetCursorPos();

    char c;
    while ((c = *(s++)) != '\0')
    {
        g_VgaBuf[(pos++) << 1] = c;
    }
    VgaSetCursorPos(pos);
}

void Init()
{
    VgaSetCursorPos(420);
    puts("Bong rips for Jesus!");
}
