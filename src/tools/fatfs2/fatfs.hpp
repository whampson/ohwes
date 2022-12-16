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

// -----------------------------------------------------------------------------
// Global Defines
// -----------------------------------------------------------------------------

#define PROG_NAME               "fatfs"
#define PROG_VERSION            "0.1"
#define PROG_COPYRIGHT          "Copyright (C) 2022-2023 Wes Hampson"

#define STATUS_SUCCESS          0
#define STATUS_INVALIDARG       1
#define STATUS_ERROR            2

extern int g_bPrefix;
extern int g_bQuiet;
extern int g_bQuietAll;
extern int g_bVerbose;

void PrintGlobalHelp();

// -----------------------------------------------------------------------------
// String Utilities
// -----------------------------------------------------------------------------

#define Plural(n, s)                ((n == 1) ? s : s "s")
#define PluralForPrintf(n, s)       n, Plural(n, s)

// -----------------------------------------------------------------------------
// Math Stuff
// -----------------------------------------------------------------------------

#define IsPow2(x)               (!((x) & ((x) - 1)))
#define Align(x, n)             (((x) + (n) - 1) & ~((n) - 1))
#define RoundDown(x, m)         (((x) / (m)) * (m))
#define RoundUp(x, m)           ((((x) + (m) - 1) / (m)) * (m))

// -----------------------------------------------------------------------------
// Logging
// -----------------------------------------------------------------------------

#define _Log(stream, level, ...)                                                \
do {                                                                            \
    if (g_bPrefix) {                                                            \
        if (level[0]) {                                                         \
            fprintf(stream, PROG_NAME ": " level ": " __VA_ARGS__);             \
        } else {                                                                \
            fprintf(stream, PROG_NAME ": " __VA_ARGS__);                        \
        }                                                                       \
    } else {                                                                    \
        if (level[0]) {                                                         \
            fprintf(stream, level ": " __VA_ARGS__);                            \
        } else {                                                                \
            fprintf(stream, __VA_ARGS__);                                       \
        }                                                                       \
    }                                                                           \
} while (0)

#define LogVerbose(...)                                                         \
do {                                                                            \
    if (!g_bQuiet && g_bVerbose) {                                              \
        _Log(stdout, "", __VA_ARGS__);                                          \
    }                                                                           \
} while (0)

#define LogInfo(...)                                                            \
do {                                                                            \
    if (!g_bQuiet) {                                                            \
        _Log(stdout, "", __VA_ARGS__);                                          \
    }                                                                           \
} while (0)

#define LogWarning(...)                                                         \
do {                                                                            \
    if (!g_bQuietAll) {                                                         \
        _Log(stdout, "warning", __VA_ARGS__);                                   \
    }                                                                           \
} while (0)
#define LogError(...)                                                           \
do {                                                                            \
    if (!g_bQuietAll) {                                                         \
        _Log(stderr, "error", __VA_ARGS__);                                     \
    }                                                                           \
} while (0)

#define LogError_BadCommand(str)                                                \
do {                                                                            \
    LogError("invalid command - %s\n", str);                                    \
} while (0)

#define LogError_BadArg(str)                                                    \
do {                                                                            \
    LogError("invalid argument - %s\n", str);                                   \
} while (0)

#define LogError_BadOpt(c)                                                      \
do {                                                                            \
    if (isprint(optopt)) {                                                      \
        LogError("invalid option - %c\n", c);                                   \
    } else {                                                                    \
        LogError("invalid option character - \\x%02X\n", c);                    \
    }                                                                           \
} while (0)

#define LogError_BadLongOpt(str)                                                \
do {                                                                            \
    LogError("invalid option - %s\n", str);                                     \
} while (0)

#define LogError_MissingCommand()                                               \
do {                                                                            \
    LogError("missing command\n");                                              \
} while (0)

#define LogError_MissingOptArg(c)                                               \
do {                                                                            \
    LogError("missing argument for '%c'\n", c);                                 \
} while (0)

#define LogError_MissingLongOptArg(str)                                         \
do {                                                                            \
    LogError("missing argument for '%s'\n", str);                               \
} while (0)

// -----------------------------------------------------------------------------
// Alloc/Free/Open/Close/Read/Write
//
// These macro functions are "Safe" in that they guard against NULL pointers
// and jump to a common error-handling path when something goes wrong. To
// facilitate this, these macro functions require the following:
//   - a bool named 'success' to be defined
//   - a label named 'Cleanup' to be defined
//   - all pointer variables assigned with SafeAlloc to be initialized to NULL
// When an error occurs (such as a bad file open request), 'success' will be set
// to false and control flow will jump to the 'Cleanup' label. Here, any
// resources that were previously allocated should be freed. An error message
// will also be printed to the log.
// -----------------------------------------------------------------------------

#define SafeAlloc(size)                                                         \
({                                                                              \
    void *_ptr = malloc(size);                                                  \
    if (!_ptr) {                                                                \
        LogError("out of memory!\n");                                           \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
    _ptr;                                                                       \
})

#define SafeFree(ptr)                                                           \
({                                                                              \
    if (ptr) {                                                                  \
        free(ptr);                                                              \
        (ptr) = NULL;                                                           \
    }                                                                           \
})

#define SafeOpen(path, mode)                                                    \
({                                                                              \
    FILE *_fp = fopen(path, mode);                                              \
    if (!_fp) {                                                                 \
        LogError("unable to open file '%s'\n", path);                           \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
    LogVerbose("opened file '%s' with mode '%s'\n", path, mode);                \
    _fp;                                                                        \
})

#define SafeClose(fp)                                                           \
({                                                                              \
    if (fp) {                                                                   \
        fclose(fp);                                                             \
        (fp) = NULL;                                                            \
    }                                                                           \
})

#define SafeRead(fp, ptr, size)                                                 \
({                                                                              \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fread(ptr, 1, size, fp);                                        \
    if (ferror(fp)) {                                                           \
        LogError("unable to read file\n");                                      \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
    LogVerbose("%d bytes read from file at offset 0x%08zx\n", (int) size, _i);  \
    _b;                                                                         \
})

#define SafeWrite(fp, ptr, size)                                                \
({                                                                              \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fwrite(ptr, 1, size, fp);                                       \
    if (ferror(fp)) {                                                           \
        LogError("unable to write file\n");                                     \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
    LogVerbose("%d bytes written to file at offset 0x%08zx\n", (int) size, _i); \
    _b;                                                                         \
})

static inline const char * GetFileName(const char *path)
{
	const char *p;
    const char *name;

    name = path;
	if ((p = strrchr(path, '/')) != NULL) {
	    name = p + 1;
    }

    return name;
}

#endif  // FATFS_H
