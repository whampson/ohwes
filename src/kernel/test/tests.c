#include "tests.h"

extern void test_printf(void);
extern void test_strings(void);
extern void test_queue(void);

void testmain(void)
{
    test_printf();
    test_strings();
    test_queue();
}
