#ifndef __FATFS_H
#define __FATFS_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LogInfo(...)                                                        \
do {                                                                        \
    fprintf(stdout, "fatfs: " __VA_ARGS__);                                 \
} while (0)

#define LogWarning(...)                                                     \
do {                                                                        \
    fprintf(stderr, "fatfs: warning: " __VA_ARGS__);                        \
} while (0)

#define LogError(...)                                                       \
do {                                                                        \
    fprintf(stderr, "fatfs: error: " __VA_ARGS__);                          \
} while (0)

#define SafeFree(ptr)  if (ptr) { free(ptr); (ptr) = NULL; }

#endif // __FATFS_H
