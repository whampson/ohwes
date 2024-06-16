#include "test.h"
#include <char_queue.h>

static queue_t _testq;
static queue_t *testq = &_testq;

// TODO: more tests!!

#define Q_SIZE 4

void test_queue(void)
{
    char buf[Q_SIZE];

    char_queue_init(testq, buf, Q_SIZE);
    VERIFY_IS_TRUE(char_queue_empty(testq));
    VERIFY_IS_TRUE(!char_queue_full(testq));

    char_queue_put(testq, 1);
    char_queue_put(testq, 2);
    char_queue_put(testq, 3);
    char_queue_put(testq, 4);
    VERIFY_IS_TRUE(!char_queue_empty(testq));
    VERIFY_IS_TRUE(char_queue_full(testq));

    VERIFY_ARE_EQUAL(1, char_queue_get(testq));
    VERIFY_ARE_EQUAL(2, char_queue_get(testq));
    VERIFY_ARE_EQUAL(3, char_queue_get(testq));
    VERIFY_ARE_EQUAL(4, char_queue_get(testq));
    VERIFY_IS_TRUE(char_queue_empty(testq));
    VERIFY_IS_TRUE(!char_queue_full(testq));
}
