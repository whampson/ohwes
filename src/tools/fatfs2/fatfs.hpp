#ifndef FATFS_HPP
#define FATFS_HPP

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

#include "fat.h"

// -----------------------------------------------------------------------------
// Global Defines
// -----------------------------------------------------------------------------

#define PROG_NAME               "fatfs"
#define PROG_VERSION            "0.1"
#define PROG_COPYRIGHT          "Copyright (C) 2022-2023 Wes Hampson"

#define STATUS_SUCCESS          0
#define STATUS_INVALIDARG       1
#define STATUS_ERROR            2

#define MAX_PATH                512
#define MAX_DATE                14      // "Sept 21, 1991"
#define MAX_TIME                9       // "12:34 PM"

extern int g_bShowHelp;
extern int g_bShowVersion;
extern int g_bUsePrefix;
extern int g_nQuietness;
extern int g_nVerbosity;
extern uint32_t g_nSectorOffset;

extern const char *g_ProgramName;

extern "C" int optidx;

#define LONGOPT_OFFSET_TOKEN    0x0FF5E7
#define GLOBAL_LONGOPTS                                                         \
    { "offset",         required_argument, 0, LONGOPT_OFFSET_TOKEN },           \
    { "prefix",         no_argument, &g_bUsePrefix, true },                     \
    { "quiet",          no_argument, &g_nQuietness, 1 },                        \
    { "quiet-all",      no_argument, &g_nQuietness, 2 },                        \
    { "verbose",        no_argument, &g_nVerbosity, 1 },                        \
    { "very-verbose",   no_argument, &g_nVerbosity, 2 },                        \
    { "help",           no_argument, &g_bShowHelp, true },                      \
    { "version",        no_argument, &g_bShowVersion, true }

#define ProcessGlobalOption(argv, longopts, optchar)                            \
do {                                                                            \
    switch (optchar) {                                                          \
        case LONGOPT_OFFSET_TOKEN:                                              \
            g_nSectorOffset = (uint32_t) strtol(optarg, NULL, 0);               \
            break;                                                              \
        case 0:                                                                 \
            if ((longopts)[optidx].flag != 0)                                   \
                break;                                                          \
            assert(!"unhandled getopt_long() case: non-flag long option");      \
            break;                                                              \
        case '?':                                                               \
            if (optopt != 0)                                                    \
                LogError_BadOpt(optopt);                                        \
            else                                                                \
                LogError_BadLongOpt(&(argv)[optind - 1][2]);                    \
            return STATUS_INVALIDARG;                                           \
        case ':':                                                               \
            if (optopt != 0)                                                    \
                LogError_MissingOptArg(optopt);                                 \
            else                                                                \
                LogError_MissingLongOptArg(&(argv)[optind - 1][2]);             \
            return STATUS_INVALIDARG;                                           \
    }                                                                           \
    if (g_bShowHelp) {                                                          \
        PrintHelp();                                                            \
        return STATUS_SUCCESS;                                                  \
    }                                                                           \
    if (g_bShowVersion) {                                                       \
        PrintVersion();                                                         \
        return STATUS_SUCCESS;                                                  \
    }                                                                           \
} while (0)     // some people might call this evil...

int PrintHelp();
int PrintGlobalHelp();
int PrintVersion();

void FormatDate(char dst[MAX_DATE], const struct tm *src);
void FormatTime(char dst[MAX_TIME], const struct tm *src);

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
#define CeilDiv(x, y)           (((x) + (y) - 1) / (y))
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

#define LogError_FileNotFound(str)                                              \
do {                                                                            \
    LogError("file not found - %s\n", str);                                     \
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

#define SafeAlloc(size)                                                         \
({                                                                              \
    void *_ptr = malloc(size);                                                  \
    SafeRIF(_ptr, "out of memory!\n");                                          \
    LogVeryVerbose("alloc'd %zu bytes at address %p\n",                         \
        (size_t) size, (void *) _ptr);                                          \
    _ptr;                                                                       \
})

#define SafeFree(ptr)                                                           \
({                                                                              \
    if (ptr) {                                                                  \
        free(ptr);                                                              \
        LogVeryVerbose("freed memory at address %p\n", (void *) ptr);           \
        (ptr) = NULL;                                                           \
    }                                                                           \
})

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
    void *_fp = fp;                                                             \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fread(ptr, 1, size, fp);                                        \
    SafeRIF(!ferror(fp), "unable to read file\n");                              \
    LogVeryVerbose("%zu bytes read from file handle %p at offset 0x%08zx\n",    \
        _b, _fp, _i);                                                           \
    _b;                                                                         \
})

#define SafeWrite(fp, ptr, size)                                                \
({                                                                              \
    void *_fp = fp;                                                             \
    size_t _i = ftell(fp);                                                      \
    size_t _b = fwrite(ptr, 1, size, fp);                                       \
    SafeRIF(!ferror(fp), "unable to write file\n");                             \
    LogVeryVerbose("%zu bytes written to file handle %p at offset 0x%08zx\n",   \
        _b, _fp, _i);                                                           \
    _b;                                                                         \
})

#endif  // FATFS_HPP
