#include <stdio.h>
#include <console.h>

int putchar(int ch)
{
    con_write((char) ch);
    return ch;
}

int puts(const char *str)
{
    char c;
    int n = 0;
    while ((c = *(str++)) != '\0')
    {
        con_write(c);
        n += 1;
    }
    con_write('\n');

    return n;       // string length
}
