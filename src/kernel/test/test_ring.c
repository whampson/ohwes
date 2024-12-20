/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: kernel/test/test_ring.c
 *      Created: December 20, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <test.h>
#include <kernel/queue.h>

void test_ring(void)
{
    DECLARE_TEST("ring buffer");

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
