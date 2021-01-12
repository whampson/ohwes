#include <ohwes/test.h>
#include <drivers/vga.h>

static int cursor(void);
static int colors(void);
static void c(int bg, int fg);

void test_ansi(void)
{
    test("Cursor Sequences", cursor);
    test("Color Sequences", colors);

cancel:
done:
    anykey();
    return;
}

static int cursor(void)
{
    print("Cursor escape sequences will be tested.\n");
    print("NOTE: depending on your system, it may not be possible to hide the cursor.\n");

    print("\nCursor: OFF\0335"); wait();
    print("\nCursor: ON\0336"); wait();

    return PASS;
    cancel: return FAIL;
}

static int colors(void)
{
    print("A color table will be shown to test color escape sequences. The number in each\n");
    print("cell represents the color combination. The rightmost digit is the foreground\n");
    print("color, the middle digit is the background color, and the leftmost digit is a\n");
    print("bitmask where the 0th bit indicates a bright foreground and the 1st bit\n");
    print("indicates a bright background.\n\n");
    anykey();

    save_console();
    print("\033[0m");
    clear_screen();
    vga_disable_blink();

    /* Black Row */
    c(00,00); c(00,01); c(00,02); c(00,03); c(00,04); c(00,05); c(00,06); c(00,07);
    c(00,10); c(00,11); c(00,12); c(00,13); c(00,14); c(00,15); c(00,16); c(00,17); print("\n");
    /* Red Row */
    c(01,00); c(01,01); c(01,02); c(01,03); c(01,04); c(01,05); c(01,06); c(01,07);
    c(01,10); c(01,11); c(01,12); c(01,13); c(01,14); c(01,15); c(01,16); c(01,17); print("\n");
    /* Green Row */
    c(02,00); c(02,01); c(02,02); c(02,03); c(02,04); c(02,05); c(02,06); c(02,07);
    c(02,10); c(02,11); c(02,12); c(02,13); c(02,14); c(02,15); c(02,16); c(02,17); print("\n");
    /* Brown Row */
    c(03,00); c(03,01); c(03,02); c(03,03); c(03,04); c(03,05); c(03,06); c(03,07);
    c(03,10); c(03,11); c(03,12); c(03,13); c(03,14); c(03,15); c(03,16); c(03,17); print("\n");
    /* Blue Row */
    c(04,00); c(04,01); c(04,02); c(04,03); c(04,04); c(04,05); c(04,06); c(04,07);
    c(04,10); c(04,11); c(04,12); c(04,13); c(04,14); c(04,15); c(04,16); c(04,17); print("\n");
    /* Magenta Row */
    c(05,00); c(05,01); c(05,02); c(05,03); c(05,04); c(05,05); c(05,06); c(05,07);
    c(05,10); c(05,11); c(05,12); c(05,13); c(05,14); c(05,15); c(05,16); c(05,17); print("\n");
    /* Cyan Row */
    c(06,00); c(06,01); c(06,02); c(06,03); c(06,04); c(06,05); c(06,06); c(06,07);
    c(06,10); c(06,11); c(06,12); c(06,13); c(06,14); c(06,15); c(06,16); c(06,17); print("\n");
    /* Gray Row */
    c(07,00); c(07,01); c(07,02); c(07,03); c(07,04); c(07,05); c(07,06); c(07,07);
    c(07,10); c(07,11); c(07,12); c(07,13); c(07,14); c(07,15); c(07,16); c(07,17); print("\n");
    /* Light Gray Row */
    c(10,00); c(10,01); c(10,02); c(10,03); c(10,04); c(10,05); c(10,06); c(10,07);
    c(10,10); c(10,11); c(10,12); c(10,13); c(10,14); c(10,15); c(10,16); c(10,17); print("\n");
    /* Bright Red Row */
    c(11,00); c(11,01); c(11,02); c(11,03); c(11,04); c(11,05); c(11,06); c(11,07);
    c(11,10); c(11,11); c(11,12); c(11,13); c(11,14); c(11,15); c(11,16); c(11,17); print("\n");
    /* Bright Green Row */
    c(12,00); c(12,01); c(12,02); c(12,03); c(12,04); c(12,05); c(12,06); c(12,07);
    c(12,10); c(12,11); c(12,12); c(12,13); c(12,14); c(12,15); c(12,16); c(12,17); print("\n");
    /* Yellow Row */
    c(13,00); c(13,01); c(13,02); c(13,03); c(13,04); c(13,05); c(13,06); c(13,07);
    c(13,10); c(13,11); c(13,12); c(13,13); c(13,14); c(13,15); c(13,16); c(13,17); print("\n");
    /* Bright Blue Row */
    c(14,00); c(14,01); c(14,02); c(14,03); c(14,04); c(14,05); c(14,06); c(14,07);
    c(14,10); c(14,11); c(14,12); c(14,13); c(14,14); c(14,15); c(14,16); c(14,17); print("\n");
    /* Bright Magenta Row */
    c(15,00); c(15,01); c(15,02); c(15,03); c(15,04); c(15,05); c(15,06); c(15,07);
    c(15,10); c(15,11); c(15,12); c(15,13); c(15,14); c(15,15); c(15,16); c(15,17); print("\n");
    /* Bright Cyan Row */
    c(16,00); c(16,01); c(16,02); c(16,03); c(16,04); c(16,05); c(16,06); c(16,07);
    c(16,10); c(16,11); c(16,12); c(16,13); c(16,14); c(16,15); c(16,16); c(16,17); print("\n");
    /* White Row */
    c(17,00); c(17,01); c(17,02); c(17,03); c(17,04); c(17,05); c(17,06); c(17,07);
    c(17,10); c(17,11); c(17,12); c(17,13); c(17,14); c(17,15); c(17,16); c(17,17); print("\n");
    print("\033[0m");
    anykey();

    restore_console();
    clear_screen();
    return PASS;
}

static inline void c(int bg, int fg)
{
    int bf = (fg >= 10);
    int bb = (bg >= 10);

    print("\033[");
    if (bf) { print("1;"); fg -= 10; } else { print("21;"); }
    if (bb) { print("5;"); bg -= 10; } else { print("25;"); }
    printf("3%d;4%dm", fg, bg);

    printf(" %d%d%d", (bb << 1) | (bf << 0), bg, fg);
}
