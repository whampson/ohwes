#include "test.h"

extern void test_printf(void);
extern void test_char_queue(void);

int main(int argc, char *argv[])
{
    test_printf();
    test_char_queue();
    printf("\n*** TESTS PASSED ***\n");
    return TEST_PASSED;
}
