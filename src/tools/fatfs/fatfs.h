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
#include <getopt.h>

#define PROG_NAME           "fatfs"
#define PROG_VERSION        "0.1"

#define STATUS_SUCCESS      0
#define STATUS_INVALIDARG   1
#define STATUS_ERROR        2

// Command-line stuff
extern bool g_Verbose;
void PrintUsage(void);
void PrintVersionInfo(void);

// Some of these macros rely heavily on GCC syntax.
// If you don't use GCC, well I don't have a solution for you.

// Logging
#define LogInfo(...) \
    do { fprintf(stdout, PROG_NAME ": " __VA_ARGS__); } while (0)
#define LogWarning(...) \
    do { fprintf(stderr, PROG_NAME ": warning: " __VA_ARGS__); } while (0)
#define LogError(...) \
    do { fprintf(stderr, PROG_NAME ": error: " __VA_ARGS__); } while (0)

// Return If False
#define RIF(x) if (!x) { success = false; goto Cleanup; }

// Alloc/Free
#define SafeAlloc(size)                                                         \
({                                                                              \
    void *_ptr = malloc(size);                                                  \
    if (!_ptr) { LogError("out of memory!\n"); success = false; goto Cleanup; } \
    _ptr;                                                                       \
})

#define SafeFree(ptr)                                                           \
({                                                                              \
    if (ptr) { free(ptr); (ptr) = NULL; }                                       \
})

// File Open/Read
#define SafeOpen(path, mode)                                                    \
({                                                                              \
    FILE *_fp = fopen(path, mode);                                              \
    if (!_fp) { LogError("unable to open file\n"); success = false; goto Cleanup; }\
    _fp;                                                                        \
})

#define SafeRead(fp, ptr, size)                                                 \
({                                                                              \
    size_t _b = fread(ptr, 1, size, fp);                                        \
    if (ferror(fp)) { LogError("unable to read file\n"); success = false; goto Cleanup; }\
    _b;                                                                         \
})

#define SafeClose(fp)                                                           \
do {                                                                            \
    if (fp) { fclose(fp); fp = NULL; }                                          \
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

#define IsFlagSet(x,flag) (((x) & (flag)) == (flag))

// String utilities
#define PLURAL(s,n) (n == 1) ? s : s "s"    // PLURALIZE? Pluralize?
#define ISARE(n)    (n == 1) ? "is" : "are"

#endif  // FATFS_H
