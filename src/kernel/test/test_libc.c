#include <test.h>
#include "test_libc.h"

#ifdef TEST_BUILD

void test_libc(void)
{
    test_printf();
    test_strings();
}

#endif
