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

bool run_tests(void)
{
    return test_libc();
}

#ifdef MAIN
int main(int argc, char **argv)
{
    bool pass = run_tests();
    return (pass) ? 0 : 1;
}
#endif

#endif
