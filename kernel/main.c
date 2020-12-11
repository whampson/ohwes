#include <nb/foo.h>

void kmain(void)
{
    char *vid_mem = (char *) 0xb8000;

    vid_mem[160]++;
    vid_mem[161] = 2;

    foo();

    // for (;;);
}
