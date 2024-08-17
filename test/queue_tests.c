#include <queue.h>
#include "test.h"

void test_ring(void)
{
    const size_t QueueLength = 4;

    char buf[QueueLength];
    struct ring _queue;
    struct ring *queue = &_queue;

    // init
    ring_init(queue, buf, QueueLength);
    VERIFY_IS_TRUE(ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // put into rear
    VERIFY_IS_TRUE(ring_put(queue, 'A') == true);
    VERIFY_IS_TRUE(!ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // get from front
    VERIFY_IS_TRUE(ring_get(queue) == 'A');
    VERIFY_IS_TRUE(ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // put into front
    VERIFY_IS_TRUE(ring_insert(queue, 'a') == true);
    VERIFY_IS_TRUE(!ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // get from rear
    VERIFY_IS_TRUE(ring_erase(queue) == 'a');
    VERIFY_IS_TRUE(ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // fill from rear
    VERIFY_IS_TRUE(ring_put(queue, 'W') == true);
    VERIFY_IS_TRUE(ring_put(queue, 'X') == true);
    VERIFY_IS_TRUE(ring_put(queue, 'Y') == true);
    VERIFY_IS_TRUE(ring_put(queue, 'Z') == true);
    VERIFY_IS_TRUE(ring_put(queue, 'A') == false);
    VERIFY_IS_TRUE(!ring_empty(queue));
    VERIFY_IS_TRUE(ring_full(queue));

    // drain from front
    VERIFY_IS_TRUE(ring_get(queue) == 'W');
    VERIFY_IS_TRUE(ring_get(queue) == 'X');
    VERIFY_IS_TRUE(ring_get(queue) == 'Y');
    VERIFY_IS_TRUE(ring_get(queue) == 'Z');
    VERIFY_IS_TRUE(ring_get(queue) == '\0');
    VERIFY_IS_TRUE(ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // fill from front
    VERIFY_IS_TRUE(ring_insert(queue, 'a') == true);
    VERIFY_IS_TRUE(ring_insert(queue, 'b') == true);
    VERIFY_IS_TRUE(ring_insert(queue, 'c') == true);
    VERIFY_IS_TRUE(ring_insert(queue, 'd') == true);
    VERIFY_IS_TRUE(ring_insert(queue, 'e') == false);
    VERIFY_IS_TRUE(!ring_empty(queue));
    VERIFY_IS_TRUE(ring_full(queue));

    // drain from rear
    VERIFY_IS_TRUE(ring_erase(queue) == 'a');
    VERIFY_IS_TRUE(ring_erase(queue) == 'b');
    VERIFY_IS_TRUE(ring_erase(queue) == 'c');
    VERIFY_IS_TRUE(ring_erase(queue) == 'd');
    VERIFY_IS_TRUE(ring_erase(queue) == '\0');
    VERIFY_IS_TRUE(ring_empty(queue));
    VERIFY_IS_TRUE(!ring_full(queue));

    // combined front/rear usage
    VERIFY_IS_TRUE(ring_put(queue, '1') == true);
    VERIFY_IS_TRUE(ring_put(queue, '2') == true);
    VERIFY_IS_TRUE(ring_put(queue, '3') == true);
    VERIFY_IS_TRUE(ring_put(queue, '4') == true);
    VERIFY_IS_TRUE(ring_full(queue));
    VERIFY_IS_TRUE(ring_erase(queue) == '4');
    VERIFY_IS_TRUE(ring_erase(queue) == '3');
    VERIFY_IS_TRUE(ring_insert(queue, '5') == true);
    VERIFY_IS_TRUE(ring_insert(queue, '6') == true);
    VERIFY_IS_TRUE(ring_full(queue));
    VERIFY_IS_TRUE(ring_get(queue) == '6');
    VERIFY_IS_TRUE(ring_get(queue) == '5');
    VERIFY_IS_TRUE(ring_get(queue) == '1');
    VERIFY_IS_TRUE(ring_get(queue) == '2');
    VERIFY_IS_TRUE(ring_empty(queue));
}
