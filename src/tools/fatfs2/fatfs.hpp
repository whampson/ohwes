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

extern int g_bShowHelp;
extern int g_bShowVersion;
extern int g_bUsePrefix;
extern int g_nQuietness;
extern int g_nVerbosity;

extern const char *g_ProgramName;

#define GLOBAL_OPTSTRING        "+:pqv"
#define GLOBAL_LONGOPTS                                                         \
    { "prefix",     no_argument, 0, 'p' },                                      \
    { "quiet",      no_argument, 0, 'q' },                                      \
    { "verbose",    no_argument, 0, 'v' },                                      \
    { "help",       no_argument, &g_bShowHelp, true },                          \
    { "version",    no_argument, &g_bShowVersion, true }

int PrintHelp();
int PrintGlobalHelp();
int PrintVersion();
bool ProcessGlobalOption(int c);

// -----------------------------------------------------------------------------
// String Utilities
// -----------------------------------------------------------------------------

#define Plural(n, s)                ((n == 1) ? s : s "s")
#define PluralForPrintf(n, s)       n, Plural(n, s)

static inline const char * GetFileName(const char *path)
{
	const char *p;
    const char *name;

    name = path;    // TODO: handle backslash too
	if ((p = strrchr(path, '/')) != NULL) {
	    name = p + 1;
    }

    return name;
}

// -----------------------------------------------------------------------------
// Math Stuff
// -----------------------------------------------------------------------------

#define IsPow2(x)               (!((x) & ((x) - 1)))
#define Align(x, n)             (((x) + (n) - 1) & ~((n) - 1))
#define RoundDown(x, m)         (((x) / (m)) * (m))
#define RoundUp(x, m)           ((((x) + (m) - 1) / (m)) * (m))
#define Ceiling(x, y)           (((x) + (y) - 1) / (y))     // TODO: rename, cdiv?
#define Min(x, y)               ((x) < (y) ? (x) : (y))
#define Max(x, y)               ((x) > (y) ? (x) : (y))

// -----------------------------------------------------------------------------
// Logging
// -----------------------------------------------------------------------------

#define _Log(stream, level, ...)                                                \
do {                                                                            \
    if (g_bUsePrefix) {                                                         \
        fprintf(stream, "%s: ", g_ProgramName);                                 \
        if (level[0]) {                                                         \
            fprintf(stream, level ": " __VA_ARGS__);                            \
        } else {                                                                \
            fprintf(stream, __VA_ARGS__);                                       \
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
    if (g_nQuietness < 1 && g_nVerbosity >= 1) {                                \
        _Log(stdout, "", __VA_ARGS__);                                          \
    }                                                                           \
} while (0)

#define LogVeryVerbose(...)                                                     \
do {                                                                            \
    if (g_nQuietness < 1 && g_nVerbosity >= 2) {                                \
        _Log(stdout, "", __VA_ARGS__);                                          \
    }                                                                           \
} while (0)

#define LogInfo(...)                                                            \
do {                                                                            \
    if (g_nQuietness < 1) {                                                     \
        _Log(stdout, "", __VA_ARGS__);                                          \
    }                                                                           \
} while (0)

#define LogWarning(...)                                                         \
do {                                                                            \
    if (g_nQuietness < 2) {                                                     \
        _Log(stdout, "warning", __VA_ARGS__);                                   \
    }                                                                           \
} while (0)
#define LogError(...)                                                           \
do {                                                                            \
    if (g_nQuietness < 2) {                                                     \
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
        LogError("invalid option character - \\x%02x\n", c);                    \
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
// The macro functions with the prefix "Safe" are safe in that they guard
// against NULL pointers and jump to a common error-handling path when something
// goes wrong. To facilitate this, these macro functions require the following:
//   - a bool named 'success' to be defined
//   - a label named 'Cleanup' to be defined
//   - all pointers assigned with "Safe" functions to be initialized to NULL
// When an error occurs (such as a bad file open request), 'success' will be set
// to false and control flow will jump to the 'Cleanup' label. Here, any
// resources that were previously allocated should be freed. An error message
// will also be printed to the log.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Return If False
// -----------------------------------------------------------------------------

// #define RIF(cond,...)                                                           \
// ({                                                                              \
//     if (!(cond)) {                                                              \
//         LogError(__VA_ARGS__);                                                  \
//         return false;                                                           \
//     }                                                                           \
// })

#define SafeRIF(cond,...)                                                       \
({                                                                              \
    if (!(cond)) {                                                              \
        LogError(__VA_ARGS__);                                                  \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
})

// -----------------------------------------------------------------------------
// Alloc/Free/Open/Close/Read/Write
// -----------------------------------------------------------------------------

inline FILE * OpenFile(const char *path, const char *mode, size_t *pOutLen)
{
    FILE *fp = fopen(path, mode);
    if (!fp) {
        LogError("unable to open file '%s'\n", path);
        *pOutLen = 0;
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    LogVeryVerbose("opened '%s' with mode '%s', handle %p, size %zu\n",
        path, mode, (void *) fp, size);

    if (pOutLen) *pOutLen = size;
    return fp;
}

#define SafeAlloc(size)                                                         \
({                                                                              \
    void *_ptr = malloc(size);                                                  \
    SafeRIF(_ptr, "out of memory!\n");                                          \
    _ptr;                                                                       \
})

#define SafeFree(ptr)                                                           \
({                                                                              \
    if (ptr) {                                                                  \
        free(ptr);                                                              \
        (ptr) = NULL;                                                           \
    }                                                                           \
})

#define SafeOpen(path, mode, pOutSize)                                          \
({                                                                              \
    FILE *_fp = OpenFile(path, mode, pOutSize);                                 \
    if (!_fp) {                                                                 \
        success = false;                                                        \
        goto Cleanup;                                                           \
    }                                                                           \
    _fp;                                                                        \
})

#define SafeClose(fp)                                                           \
({                                                                              \
    if (fp) {                                                                   \
        void *_fp = fp;                                                         \
        fclose(fp);                                                             \
        (fp) = NULL;                                                            \
        LogVeryVerbose("closed file with handle %p\n", _fp);                    \
    }                                                                           \
})

#define SafeRead(fp, ptr, size)                                                 \
({                                                                              \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fread(ptr, 1, size, fp);                                        \
    SafeRIF(!ferror(fp), "unable to read file\n");                              \
    LogVeryVerbose("%zu bytes read from file at offset 0x%08zx\n", _b, _i);     \
    _b;                                                                         \
})

#define SafeWrite(fp, ptr, size)                                                \
({                                                                              \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fwrite(ptr, 1, size, fp);                                       \
    SafeRIF(!ferror(fp), "unable to write file\n");                             \
    LogVeryVerbose("%zu bytes written to file at offset 0x%08zx\n", _b, _i);    \
    _b;                                                                         \
})

#endif  // FATFS_H
