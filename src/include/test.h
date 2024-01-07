#ifndef __TEST_H
#define __TEST_H

#if TEST_BUILD

#include <os.h>
#include <stdio.h>

#define _VERIFY_PANIC(fn,...) \
    panic("TEST FAILED!!\n%s:%d:\n\t" fn "(" #__VA_ARGS__ ")", __FILE__, __LINE__)

#define VERIFY_IS_TRUE(x)                                                       \
do {                                                                            \
    if (!(x)) {                                                                 \
        _VERIFY_PANIC("VERIFY_IS_TRUE", x);                                     \
    }                                                                           \
} while (0)

#define VERIFY_IS_FALSE(x)                                                      \
do {                                                                            \
    if (x) {                                                                    \
        _VERIFY_PANIC("VERIFY_IS_FALSE", x);                                    \
    }                                                                           \
} while (0)

#define VERIFY_IS_ZERO(x)                                                       \
do {                                                                            \
    if ((x) != 0) {                                                             \
        _VERIFY_PANIC("VERIFY_IS_ZERO", x);                                     \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_ZERO(x)                                                   \
do {                                                                            \
    if ((x) == 0) {                                                             \
        _VERIFY_PANIC("VERIFY_IS_NOT_ZERO", x);                                 \
    }                                                                           \
} while (0)

#define VERIFY_IS_NULL(x)                                                       \
do {                                                                            \
    if ((x) != NULL) {                                                          \
        _VERIFY_PANIC("VERIFY_IS_NULL", x);                                     \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_NULL(x)                                                   \
do {                                                                            \
    if ((x) == NULL) {                                                          \
        _VERIFY_PANIC("VERIFY_IS_NOT_NULL", x);                                 \
    }                                                                           \
} while (0)

#define VERIFY_ARE_EQUAL(x,y)                                                   \
do {                                                                            \
    if ((x) != (y)) {                                                           \
        _VERIFY_PANIC("VERIFY_ARE_EQUAL", x, y);                                \
    }                                                                           \
} while (0)

#define VERIFY_ARE_NOT_EQUAL(x,y)                                               \
do {                                                                            \
    if ((x) == (y)) {                                                           \
        _VERIFY_PANIC("VERIFY_ARE_NOT_EQUAL", x, y);                            \
    }                                                                           \
} while (0)

void test_libc(void);

#endif // TEST_BUILD

#endif // __TEST_H
