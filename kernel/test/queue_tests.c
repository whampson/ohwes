#include "test.h"
#include <ring.h>

static queue_t _testq;
static queue_t *testq = &_testq;

// TODO: more tests!!

#define Q_SIZE 4

void test_queue(void)
{
    char buf[Q_SIZE];

    ring_init(testq, buf, Q_SIZE);
    VERIFY_IS_TRUE(ring_empty(testq));
    VERIFY_IS_TRUE(!ring_full(testq));

    ring_put(testq, 1);
    ring_put(testq, 2);
    ring_put(testq, 3);
    ring_put(testq, 4);
    VERIFY_IS_TRUE(!ring_empty(testq));
    VERIFY_IS_TRUE(ring_full(testq));

    VERIFY_ARE_EQUAL(1, ring_get(testq));
    VERIFY_ARE_EQUAL(2, ring_get(testq));
    VERIFY_ARE_EQUAL(3, ring_get(testq));
    VERIFY_ARE_EQUAL(4, ring_get(testq));
    VERIFY_IS_TRUE(ring_empty(testq));
    VERIFY_IS_TRUE(!ring_full(testq));
}
