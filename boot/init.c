#include "boot.h"

#include <stdint.h>
#include <os/console.h>

char * const g_VgaBuf = (char * const) 0xB8000;

void puts(const char *s)
{
    uint16_t pos = con_get_cursor();

    char c;
    while ((c = *(s++)) != '\0')
    {
        switch (c)
        {
            case '\r':
                pos -= (pos % 80);
                break;
            case '\n':
                pos += (80 - (pos % 80));
                break;

            default:
                g_VgaBuf[(pos++) << 1] = c;
                break;
        }
    }
    con_set_cursor(pos);
}

void init()
{
    struct equipment_flags *ef = (struct equipment_flags *) &g_hwflags;
    puts("\nhas diskette drive: "); puts(((ef->has_diskette_drive) ? "yes" : "no"));
    puts("\nhas coprocessor: "); puts(((ef->has_coprocessor) ? "yes" : "no"));
    puts("\nhas mouse: "); puts(((ef->has_ps2_mouse) ? "yes" : "no"));
    puts("\nhas game port: "); puts(((ef->has_game_port) ? "yes" : "no"));
    puts("\n");
}
