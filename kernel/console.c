#include <nb/console.h>
#include <nb/kernel.h>
#include <drivers/vga.h>

// static int pos;

void con_init(void)
{
    vga_init();
}

void printk(const char *format, ...)
{
    (void) format;
}
