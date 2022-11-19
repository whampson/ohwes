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
extern bool g_Verbose;
void PrintGlobalHelp();

// Some of these macros rely heavily on GCC syntax.
// If you don't use GCC, well I don't have a solution for you.

// Logging
#define LogVerbose(...) do { if (g_bVerbose) fprintf(stdout, PROG_NAME ": " __VA_ARGS__); } while (0)
#define LogInfo(...)    do { fprintf(stdout, PROG_NAME ": " __VA_ARGS__); } while (0)
#define LogWarning(...) do { fprintf(stderr, PROG_NAME ": warning: " __VA_ARGS__); } while (0)
#define LogError(...)   do { fprintf(stderr, PROG_NAME ": error: " __VA_ARGS__); } while (0)

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

}

#endif  // FATFS_H
