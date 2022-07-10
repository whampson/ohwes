#include <cstdio>
#include "fat.hpp"
#include "image.hpp"

#define ERROR(msg)                                      \
do {                                                    \
    fprintf(stderr, "fatfs: error: " msg);              \
} while (0)

#define ERRORF(fmt, ...)                                \
do {                                                    \
    fprintf(stderr, "fatfs: error: " fmt, __VA_ARGS__); \
} while (0)

int main(int argc, char **argv)
{
    if (argc == 0) {
        ERROR("missing argument");
        return 1;
    }

    FatImage *img = FatImage::Load(argv[1]);
    
    delete img;
    return 0;
}
