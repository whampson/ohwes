#include "diskimage.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LogError("missing file\n");
        return 1;
    }

    if (!OpenImage(argv[1]))
    {
        return 2;
    }

    CloseImage();
    return 0;
}
