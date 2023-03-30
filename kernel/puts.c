#include <os/console.h>

int puts(const char *str)
{
    char c;
    int n = 0;
    while ((c = *(str++)) != '\0')
    {
        console_write(c);
        n++;
    }

    return n;
}
