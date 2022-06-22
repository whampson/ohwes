#include <ohwes/test.h>
#include <drivers/vga.h>

#define write_char(c)                                           \
do {                                                            \
    static uint8_t *__vga = (uint8_t *) VGA_FRAMEBUF_COLOR;     \
    int __pos = vga_get_cursor_pos();                           \
    *(__vga + (__pos << 1)) = c;                                \
    vga_set_cursor_pos(++__pos);                                \
} while (0)

#define write_attr(a)                                           \
do {                                                            \
    static uint8_t *__vga = (uint8_t *) VGA_FRAMEBUF_COLOR;     \
    int __pos = vga_get_cursor_pos();                           \
    *(__vga + ((__pos << 1) + 1)) = a;                          \
} while (0)

static int font(void);
// static int cursor(void);

void test_vga(void)
{
    test("Font & Character Attributes", font);
    // test("Cursor Settings", cursor);

cancel:
done:
    anykey();
    return;
}

static int font(void)
{
    save_console();

    /* font table */
    print("    ");
    for (int i = 0; i < 16; i++) {
        printf("%X ", i);
    }
    print("\n");
    for (int i = 0; i < 256; i++) {
        if ((i % 16) == 0) printf("\n %X  ", i / 16);
        write_char(i); print(" ");
    }
    print("\n\n");

    /* color & attribute table */
    vga_enable_blink();
    vga_set_cursor_pos((6 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xD5);
    write_attr(0x07); write_char(0xCD);
    write_attr(0x07); write_char(0xCD);
    write_attr(0x70); write_char('t');
    write_attr(0x70); write_char('e');
    write_attr(0x70); write_char('s');
    write_attr(0x70); write_char('t');
    write_attr(0x07); write_char(0xCD);
    write_attr(0x07); write_char(0xCD);
    write_attr(0x07); write_char(0xB8);
    vga_set_cursor_pos((7 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xB3);
    write_attr(0x00); write_char(' ');
    write_attr(0x10); write_char(' ');
    write_attr(0x20); write_char(' ');
    write_attr(0x30); write_char(' ');
    write_attr(0x40); write_char(' ');
    write_attr(0x50); write_char(' ');
    write_attr(0x60); write_char(' ');
    write_attr(0x70); write_char(' ');
    write_attr(0x07); write_char(0xB3);
    vga_set_cursor_pos((8 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xB3);
    write_attr(0x00); write_char('w');
    write_attr(0x01); write_char('d');
    write_attr(0x02); write_char('j');
    write_attr(0x03); write_char('n');
    write_attr(0x04); write_char('s');
    write_attr(0x05); write_char('y');
    write_attr(0x06); write_char('x');
    write_attr(0x07); write_char('m');
    write_attr(0x07); write_char(0xB3);
    vga_set_cursor_pos((9 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xB3);
    write_attr(0x08); write_char('c');
    write_attr(0x09); write_char('t');
    write_attr(0x0A); write_char('l');
    write_attr(0x0B); write_char('q');
    write_attr(0x0C); write_char('g');
    write_attr(0x0D); write_char('b');
    write_attr(0x0E); write_char('v');
    write_attr(0x0F); write_char('z');
    write_attr(0x07); write_char(0xB3);
    vga_set_cursor_pos((10 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xB3);
    write_attr(0x0A); write_char(' ');
    write_attr(0x19); write_char('{');
    write_attr(0x1A); write_char('}');
    write_attr(0x0B); write_char(' ');
    write_attr(0x0C); write_char(' ');
    write_attr(0x8D); write_char(0x0F);
    write_attr(0x8E); write_char(0x02);
    write_attr(0x0F); write_char(' ');
    write_attr(0x07); write_char(0xB3);
    vga_set_cursor_pos((11 * VGA_TEXT_COLS) + (VGA_TEXT_COLS / 2) + 10);
    write_attr(0x07); write_char(0xC0);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xC4);
    write_attr(0x07); write_char(0xD9);

    restore_console();
    vga_set_cursor_pos(((VGA_TEXT_ROWS - 4) * VGA_TEXT_COLS));
    return PASS;
}

// static int cursor(void)
// {
//     return FAIL;
// }
