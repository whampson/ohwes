#ifndef FATFS_H
#define FATFS_H

#include <assert.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROG_NAME           "fatfs"
#define PROG_VERSION        "0.1"

#define STATUS_SUCCESS      0
#define STATUS_INVALIDARG   1
#define STATUS_ERROR        2

#define MAX_PATH            256

// Logging
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

// Alloc/Free
#define SafeAlloc(ptr,size)                                                     \
do {                                                                            \
    ptr = malloc(size);                                                         \
    if (!ptr) { LogError("out of memory!\n"); success = false; goto Cleanup; }  \
} while (0)

#define SafeFree(ptr)                                                           \
do {                                                                            \
    if (ptr) { free(ptr); (ptr) = NULL; }                                       \
} while (0)

// Math
#define max(a,b)                                                                \
({  __typeof__ (a) _a = (a);                                                    \
    __typeof__ (b) _b = (b);                                                    \
    _a > _b ? _a : _b;                                                          \
})

#define min(a,b)                                                                \
({  __typeof__ (a) _a = (a);                                                    \
    __typeof__ (b) _b = (b);                                                    \
    _a < _b ? _a : _b;                                                          \
})


// String utilities
#define PLURAL(s,n) (n == 1) ? s : s "s"
#define ISARE(n)    (n == 1) ? "is" : "are"

static void Uppercase(char *s)
{
    for (size_t i = 0; i < strnlen(s, MAX_PATH); i++)
    {
        s[i] = toupper(s[i]);
    }
}

// Command-line stuff
void Usage(void);
void VersionInfo(void);

#endif  // FATFS_H
