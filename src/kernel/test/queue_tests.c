#include "test.h"
#include <queue.h>

static queue_t _testq;
static queue_t *testq = &_testq;

// TODO: more tests!!

#define Q_SIZE 4

void test_queue(void)
{
    char buf[Q_SIZE];

    q_init(testq, buf, Q_SIZE);
    VERIFY_IS_TRUE(q_empty(testq));
    VERIFY_IS_TRUE(!q_full(testq));

    q_put(testq, 1);
    q_put(testq, 2);
    q_put(testq, 3);
    q_put(testq, 4);
    VERIFY_IS_TRUE(!q_empty(testq));
    VERIFY_IS_TRUE(q_full(testq));

    VERIFY_ARE_EQUAL(1, q_get(testq));
    VERIFY_ARE_EQUAL(2, q_get(testq));
    VERIFY_ARE_EQUAL(3, q_get(testq));
    VERIFY_ARE_EQUAL(4, q_get(testq));
    VERIFY_IS_TRUE(q_empty(testq));
    VERIFY_IS_TRUE(!q_full(testq));
}
