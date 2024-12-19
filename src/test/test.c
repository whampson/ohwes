#include "test.h"

extern void test_printf(void);
extern void test_ring(void);

int main(int argc, char *argv[])
{
    test_printf();
    test_ring();
    printf("\n*** TESTS PASSED ***\n");
    return TEST_PASSED;
}
