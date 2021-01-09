#include <ohwes/test.h>
#include <drivers/vga.h>

#define write_vga(c)                                    \
do {                                                    \
    static char *__vga = (char *) VGA_FRAMEBUF_COLOR;   \
    int __pos = vga_get_cursor_pos();                   \
    *(__vga + (__pos << 1)) = c;                        \
    vga_set_cursor_pos(++__pos);                        \
} while (0)

static int font(void);

void test_vga(void)
{
    suite_begin("VGA");
    test("VGA Font Table", font);
    suite_end("VGA");

cancel:
done:
    anykey();
    return;
}

static int font(void)
{
    print("    ");
    for (int i = 0; i < 16; i++) {
        printf("%X ", i);
    }

    print("\n");
    for (int i = 0; i < 256; i++) {
        if ((i % 16) == 0) printf("\n %X  ", i / 16);
        write_vga(i); print(" ");
    }

    print("\n\n");
    return PASS;
}
