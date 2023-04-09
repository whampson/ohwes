#include <stdio.h>
#include <os/console.h>

int putchar(int ch)
{
    console_write((char) ch);
    return 1;
}

int puts(const char *str)
{
    char c;
    int n = 0;
    while ((c = *(str++)) != '\0')
    {
        n += putchar(c);
    }

    return n;
}
