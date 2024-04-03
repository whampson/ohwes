#include "tests.h"

extern void test_printf(void);
extern void test_strings(void);
extern void test_queue(void);
extern void test_syscalls(void);

void tmain(void)
{
    test_printf();
    test_strings();
    test_queue();
}

void tmain_ring3(void)
{
    test_syscalls();
}
