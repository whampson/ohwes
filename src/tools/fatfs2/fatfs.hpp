#ifndef FATFS_H
#define FATFS_H

extern "C" {

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

// Global options
extern int g_bNoPrefix;
extern int g_bQuiet;
extern int g_bQuietAll;
extern int g_bVerbose;

void PrintGlobalHelp();

// Some of these macros rely heavily on GCC syntax.
// If you don't use GCC, well I don't have a solution for you.

// Logging
#define _Log(stream, level, ...)                                                                    \
do {                                                                                                \
    if (!g_bNoPrefix) {                                                                             \
        if (level[0]) {                                                                             \
            fprintf(stream, PROG_NAME ": " level ": " __VA_ARGS__);                                 \
        } else {                                                                                    \
            fprintf(stream, PROG_NAME ": " __VA_ARGS__);                                            \
        }                                                                                           \
    } else {                                                                                        \
        if (level[0]) {                                                                             \
            fprintf(stream, level ": " __VA_ARGS__);                                                \
        } else {                                                                                    \
            fprintf(stream, __VA_ARGS__);                                                           \
        }                                                                                           \
    }                                                                                               \
} while (0)

#define LogVerbose(...)                                                                             \
do {                                                                                                \
    if (!g_bQuiet && g_bVerbose) {                                                                  \
        _Log(stdout, "", __VA_ARGS__);                                                              \
    }                                                                                               \
} while (0)

#define LogInfo(...)                                                                                \
do {                                                                                                \
    if (!g_bQuiet) {                                                                                \
        _Log(stdout, "", __VA_ARGS__);                                                              \
    }                                                                                               \
} while (0)

#define LogWarning(...)                                                                             \
do {                                                                                                \
    if (!g_bQuietAll) {                                                                             \
        _Log(stdout, "warning", __VA_ARGS__);                                                       \
    }                                                                                               \
} while (0)
#define LogError(...)                                                                               \
do {                                                                                                \
    if (!g_bQuietAll) {                                                                             \
        _Log(stderr, "error", __VA_ARGS__);                                                         \
    }                                                                                               \
} while (0)

#define BAD_COMMAND(str)                                                                            \
do {                                                                                                \
    LogError("invalid command - '%s'\n", str);                                                      \
} while (0)

#define BAD_OPT(c)                                                                                  \
do {                                                                                                \
    if (isprint(optopt)) {                                                                          \
        LogError("invalid option - '%c'\n", c);                                                     \
    } else {                                                                                        \
        LogError("invalid option character - '\\x%02X'\n", c);                                      \
    }                                                                                               \
} while (0)

#define BAD_LONGOPT(str)                                                                            \
do {                                                                                                \
    LogError("invalid option - '%s'\n", str);                                                       \
} while (0)

#define MISSING_COMMAND()                                                                           \
do {                                                                                                \
    LogError("missing command\n");                                                                  \
} while (0)

#define MISSING_OPTARG(c)                                                                           \
do {                                                                                                \
    LogError("missing argument for '%c'\n", c);                                                     \
} while (0)

#define MISSING_LONGOPTARG(str)                                                                     \
do {                                                                                                \
    LogError("missing argument for '%s'\n", str);                                                   \
} while (0)

#define SafeAlloc(size)                                                                             \
({                                                                                                  \
    void *_ptr = malloc(size);                                                                      \
    if (!_ptr) { LogError("out of memory!\n"); success = false; goto Cleanup; }                     \
    _ptr;                                                                                           \
})

#define SafeFree(ptr)                                                                               \
({                                                                                                  \
    if (ptr) { free(ptr); (ptr) = NULL; }                                                           \
})

#define SafeOpen(path, mode)                                                                        \
({                                                                                                  \
    FILE *_fp = fopen(path, mode);                                                                  \
    if (!_fp) {                                                                                     \
        LogError("unable to open file '%s'\n", path);                                               \
        success = false;                                                                            \
        goto Cleanup;                                                                               \
    }                                                                                               \
    LogVerbose("opened file '%s' with mode '%s'\n", path, mode);                                    \
    _fp;                                                                                            \
})

#define SafeClose(fp)                                                                               \
({                                                                                                  \
    if (fp) { fclose(fp); fp = NULL; }                                                              \
})

#define SafeWrite(fp, ptr, size)                                                                    \
({                                                                                                  \
    size_t _i = ftell(fp);                                                                          \
    size_t _b = fwrite(ptr, 1, size, fp);                                                           \
    if (ferror(fp)) { LogError("unable to write file\n"); success = false; goto Cleanup; }          \
    LogVerbose("%d bytes written to file at address 0x%08zx\n", (int) size, _i);                    \
    _b;                                                                                             \
})

}

#endif  // FATFS_H
