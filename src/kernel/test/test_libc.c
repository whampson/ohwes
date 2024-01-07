#include <test.h>
#include "test_libc.h"

#ifdef TEST_BUILD

bool test_libc(void)
{
    bool pass = true;
    pass &= test_printf();
    pass &= test_strings();
    return pass;
}

#endif
