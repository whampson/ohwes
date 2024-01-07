#ifndef __TEST_LIBC_H
#define __TEST_LIBC_H

#include <stdbool.h>

#ifdef TEST_BUILD

bool test_printf(void);
bool test_strings(void);

#endif

#endif // __TEST_LIBC_H
