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

// Command-line stuff
extern bool g_Verbose;
void PrintUsage(void);
void PrintVersionInfo(void);

// Logging
#define LogInfo(...) \
    ({ fprintf(stdout, PROG_NAME ": " __VA_ARGS__); })
#define LogWarning(...) \
    ({ fprintf(stderr, PROG_NAME ": warning: " __VA_ARGS__); })
#define LogError(...) \
    ({ fprintf(stderr, PROG_NAME ": error: " __VA_ARGS__); })

// All Safe* macros require the following to exist before use:
//   - a bool named 'success'
//   - a label named 'Cleanup'

#define RIF(x) if (!x) { success = false; goto Cleanup; }

// Alloc/Free
#define SafeAlloc(size)                                                         \
({                                                                              \
    void *__ptr = malloc(size);                                                 \
    if (!__ptr) { LogError("out of memory!\n"); success = false; goto Cleanup; }\
    __ptr;                                                                      \
})

#define SafeFree(ptr)                                                           \
({                                                                              \
    if (ptr) { free(ptr); (ptr) = NULL; }                                       \
})

// File Open/Read
#define SafeOpen(path, mode)                                                    \
({                                                                              \
    FILE *__fp = fopen(path, mode);                                             \
    if (!__fp) { LogError("unable to open file\n"); success = false; goto Cleanup; }\
    __fp;                                                                       \
})

#define SafeRead(fp, ptr, size)                                                 \
({                                                                              \
    size_t __b = fread(ptr, 1, size, fp);                                       \
    if (ferror(fp)) { LogError("unable to read file\n"); success = false; goto Cleanup; }\
    __b;                                                                        \
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
#define PLURAL(s,n) (n == 1) ? s : s "s"
#define ISARE(n)    (n == 1) ? "is" : "are"

#endif  // FATFS_H
