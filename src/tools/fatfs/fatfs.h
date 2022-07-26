#ifndef __FATFS_H
#define __FATFS_H

#include <assert.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PATH    512
#define PROG_NAME   "fatfs"

#define LogInfo(...)                                                            \
do {                                                                            \
    fprintf(stdout, PROG_NAME ": " __VA_ARGS__);                                \
} while (0)

#define LogWarning(...)                                                         \
do {                                                                            \
    fprintf(stderr, PROG_NAME ": warning: " __VA_ARGS__);                       \
} while (0)

#define LogError(...)                                                           \
do {                                                                            \
    fprintf(stderr, PROG_NAME ": error: " __VA_ARGS__);                         \
} while (0)

#define SafeFree(ptr)  if (ptr) { free(ptr); (ptr) = NULL; }

static inline void Uppercase(char *s)
{
    for (size_t i = 0; i < strlen(s); i++)
    {
        s[i] = toupper(s[i]);
    }
}

void Usage(void);

#endif // __FATFS_H
