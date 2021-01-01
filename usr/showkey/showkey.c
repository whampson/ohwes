#include <stdio.h>
#include <ohwes/keyboard.h>
#include <ohwes/io.h>

int main(void)
{
    char buf[16];
    int i, n, m;

    /* TODO: get/set mode via ioctl */

    m = kbd_getmode();
    while (1) {
        if (m == KB_COOKED) {
            n = read(0, buf, 1);
            if (n == 1) {
                printf("\t%3d 0%03o 0x%02x\n", buf[0], buf[0], buf[0]);
            }
            continue;
        }

        n = read(0, buf, sizeof(buf));
        for (i = 0; i < n; i++) {
            if (m == KB_RAW) {
                printf("0x%02hhx ", buf[i]);
            }
            if (m == KB_MEDIUMRAW) {
                printf("keycode %3d %s\n",
                        buf[i] & 0x7F,
                        buf[i] & 0x80 ? "release" : "press");
            }
        }
        if (m == KB_RAW) printf("\n");
    }

    return 0;
}
