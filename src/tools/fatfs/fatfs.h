#ifndef __FATFS_H
#define __FATFS_H

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

#define static_assert(expr) _Static_assert(expr, #expr)

#endif // __FATFS_H
