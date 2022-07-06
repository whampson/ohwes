#include <ohwes/console.h>
#include <ohwes/keyboard.h>
#include <ohwes/test.h>

static void rawterm(void);

void test_keyboard(void)
{
    rawterm();
    return;
}

static void rawterm(void)       // TODO: naming; raw, cooked, hmmm...
{
    save_console();
    clear_screen();

    kbd_ioctl(KBSETMODE, KB_COOKED);

    print("Mash some keys! Press CTRL+C to quit.\n\n");

    ssize_t r;
    char c;
    while (1) {
        r = kbd_read(&c, 1);
        if (r == 0) continue;
        if (r < 0) perror("keyboard");
        if (c == 0x03) goto done;
        con_write(c);
    }

done:
    restore_console();
}

