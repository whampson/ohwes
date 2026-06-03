/* =============================================================================
 * Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
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
 *         File: src/libc/test/test_ring.c
 *      Created: June 2, 2026
 *       Author: Wes Hampson
 *
 * Ring buffer tests.
 * =============================================================================
 */

#include "framework.h"
#include <ring.h>

static void test_ring_init(void)
{
    int buffer[10];
    struct ring2 r;

    TEST("basic initialization");
    ring_init(&r, buffer, 10);

    ASSERT_EQ_INT((void *)buffer, r.buf, "buffer pointer incorrect");
    ASSERT_EQ_INT(10, r.cap, "capacity incorrect");
    ASSERT_EQ_INT(0, r.count, "count should be 0");
    ASSERT_EQ_INT(0, r.head, "head should be 0");
    ASSERT_EQ_INT(0, r.tail, "tail should be 0");
    PASS();
}

static void test_push_pop_int(void)
{
    int buffer[5];
    struct ring2 r;
    int val;

    TEST("push_pop_int - push back and pop front");
    ring_init(&r, buffer, 5);

    ASSERT(ring_push_back(&r, 10, int) == 1, "push 10 failed");
    ASSERT(ring_push_back(&r, 20, int) == 1, "push 20 failed");
    ASSERT(ring_push_back(&r, 30, int) == 1, "push 30 failed");
    ASSERT(ring_push_back(&r, 40, int) == 1, "push 40 failed");
    ASSERT(ring_push_back(&r, 50, int) == 1, "push 50 failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");

    ASSERT(ring_push_back(&r, 60, int) == 0, "push on full ring should fail");

    ASSERT(ring_pop_front(&r, val, int) == 1, "pop 10 failed");
    ASSERT(val == 10, "expected 10");

    ASSERT(ring_pop_front(&r, val, int) == 1, "pop 20 failed");
    ASSERT(val == 20, "expected 20");

    ASSERT(ring_pop_front(&r, val, int) == 1, "pop 30 failed");
    ASSERT(val == 30, "expected 30");

    ASSERT(ring_pop_front(&r, val, int) == 1, "pop 40 failed");
    ASSERT(val == 40, "expected 40");

    ASSERT(ring_pop_front(&r, val, int) == 1, "pop 50 failed");
    ASSERT(val == 50, "expected 50");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_front(&r, val, int) == 0, "pop from empty should fail");
    PASS();
}

static void test_push_pop_char(void)
{
    char buffer[5];
    struct ring2 r;
    char val;

    TEST("push_pop_char - char buffer operations");
    ring_init(&r, buffer, 5);

    ASSERT(ring_push_back(&r, 'a', char) == 1, "push 'a' failed");
    ASSERT(ring_push_back(&r, 'b', char) == 1, "push 'b' failed");
    ASSERT(ring_push_back(&r, 'c', char) == 1, "push 'c' failed");
    ASSERT(ring_push_back(&r, 'd', char) == 1, "push 'd' failed");
    ASSERT(ring_push_back(&r, 'e', char) == 1, "push 'e' failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");
    ASSERT(ring_push_back(&r, 'f', char) == 0, "push on full should fail");

    ASSERT(ring_pop_front(&r, val, char) == 1, "pop 'a' failed");
    ASSERT(val == 'a', "expected 'a'");

    ASSERT(ring_pop_front(&r, val, char) == 1, "pop 'b' failed");
    ASSERT(val == 'b', "expected 'b'");

    ASSERT(ring_pop_front(&r, val, char) == 1, "pop 'c' failed");
    ASSERT(val == 'c', "expected 'c'");

    ASSERT(ring_pop_front(&r, val, char) == 1, "pop 'd' failed");
    ASSERT(val == 'd', "expected 'd'");

    ASSERT(ring_pop_front(&r, val, char) == 1, "pop 'e' failed");
    ASSERT(val == 'e', "expected 'e'");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_front(&r, val, char) == 0, "pop from empty should fail");
    PASS();
}

static void test_peek(void)
{
    int buffer[5];
    struct ring2 r;

    TEST("peek - peek front and back");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    ASSERT(ring_peek_front(&r, int) == 10, "peek front should be 10");
    ASSERT(ring_peek_back(&r, int) == 30, "peek back should be 30");
    PASS();
}

static void test_get_set_at(void)
{
    int buffer[5];
    struct ring2 r;
    int val;

    TEST("get_set_at - get and set at position");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    ASSERT(ring_get_at(&r, 0, val, int) == 1, "get at 0 failed");
    ASSERT(val == 10, "expected 10 at 0");

    ASSERT(ring_get_at(&r, 1, val, int) == 1, "get at 1 failed");
    ASSERT(val == 20, "expected 20 at 1");

    ASSERT(ring_get_at(&r, 2, val, int) == 1, "get at 2 failed");
    ASSERT(val == 30, "expected 30 at 2");

    ASSERT(ring_set_at(&r, 1, 99, int) == 1, "set at 1 failed");
    ASSERT(ring_get_at(&r, 1, val, int) == 1, "get at 1 after set failed");
    ASSERT(val == 99, "expected 99 at 1");
    PASS();
}

static void test_iterator_int(void)
{
    int buffer[5];
    struct ring2 r;
    int val;

    TEST("iterator_int - forward iteration");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    int expected[] = {10, 20, 30};
    size_t i = 0;

    for (ring_iterator(&r, val, int))
        ASSERT(val == expected[i++], "expected %d, got %d", expected[i-1], val);

    PASS();
}

static void test_reverse_iterator_int(void)
{
    int buffer[5];
    struct ring2 r;
    int val;

    TEST("reverse_iterator_int - reverse iteration");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    int expected[] = {30, 20, 10};
    size_t i = 0;

    for (ring_reverse_iterator(&r, val, int))
        ASSERT(val == expected[i++], "expected %d, got %d", expected[i-1], val);

    PASS();
}

static void test_push_front_pop_back_int(void)
{
    int buffer[5];
    struct ring2 r;
    int val;

    TEST("push_front_pop_back_int - push front and pop back");
    ring_init(&r, buffer, 5);

    ASSERT(ring_push_front(&r, 10, int) == 1, "push front 10 failed");
    ASSERT(ring_push_front(&r, 20, int) == 1, "push front 20 failed");
    ASSERT(ring_push_front(&r, 30, int) == 1, "push front 30 failed");
    ASSERT(ring_push_front(&r, 40, int) == 1, "push front 40 failed");
    ASSERT(ring_push_front(&r, 50, int) == 1, "push front 50 failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");
    ASSERT(ring_push_front(&r, 60, int) == 0, "push on full should fail");

    ASSERT(ring_pop_back(&r, val, int) == 1, "pop back 10 failed");
    ASSERT(val == 10, "expected 10");

    ASSERT(ring_pop_back(&r, val, int) == 1, "pop back 20 failed");
    ASSERT(val == 20, "expected 20");

    ASSERT(ring_pop_back(&r, val, int) == 1, "pop back 30 failed");
    ASSERT(val == 30, "expected 30");

    ASSERT(ring_pop_back(&r, val, int) == 1, "pop back 40 failed");
    ASSERT(val == 40, "expected 40");

    ASSERT(ring_pop_back(&r, val, int) == 1, "pop back 50 failed");
    ASSERT(val == 50, "expected 50");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_back(&r, val, int) == 0, "pop from empty should fail");
    PASS();
}

static void test_push_front_pop_back_char(void)
{
    char buffer[5];
    struct ring2 r;
    char val;

    TEST("push_front_pop_back_char - char push front and pop back");
    ring_init(&r, buffer, 5);

    ASSERT(ring_push_front(&r, 'a', char) == 1, "push front 'a' failed");
    ASSERT(ring_push_front(&r, 'b', char) == 1, "push front 'b' failed");
    ASSERT(ring_push_front(&r, 'c', char) == 1, "push front 'c' failed");
    ASSERT(ring_push_front(&r, 'd', char) == 1, "push front 'd' failed");
    ASSERT(ring_push_front(&r, 'e', char) == 1, "push front 'e' failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");
    ASSERT(ring_push_front(&r, 'f', char) == 0, "push on full should fail");

    ASSERT(ring_pop_back(&r, val, char) == 1, "pop back 'a' failed");
    ASSERT(val == 'a', "expected 'a'");

    ASSERT(ring_pop_back(&r, val, char) == 1, "pop back 'b' failed");
    ASSERT(val == 'b', "expected 'b'");

    ASSERT(ring_pop_back(&r, val, char) == 1, "pop back 'c' failed");
    ASSERT(val == 'c', "expected 'c'");

    ASSERT(ring_pop_back(&r, val, char) == 1, "pop back 'd' failed");
    ASSERT(val == 'd', "expected 'd'");

    ASSERT(ring_pop_back(&r, val, char) == 1, "pop back 'e' failed");
    ASSERT(val == 'e', "expected 'e'");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_back(&r, val, char) == 0, "pop from empty should fail");
    PASS();
}

static void test_peek_both_ends_int(void)
{
    int buffer[5];
    struct ring2 r;

    TEST("peek_both_ends_int - peek both ends after mixed ops");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    ASSERT(ring_peek_front(&r, int) == 10, "peek front should be 10");
    ASSERT(ring_peek_back(&r, int) == 30, "peek back should be 30");

    ring_push_front(&r, 5, int);
    ring_push_front(&r, 4, int);

    ASSERT(ring_peek_front(&r, int) == 4, "peek front should be 4");
    ASSERT(ring_peek_back(&r, int) == 30, "peek back should be 30");
    PASS();
}

static void test_peek_both_ends_char(void)
{
    char buffer[5];
    struct ring2 r;

    TEST("peek_both_ends_char - peek both ends after mixed ops");
    ring_init(&r, buffer, 5);

    ring_push_back(&r, 'a', char);
    ring_push_back(&r, 'b', char);
    ring_push_back(&r, 'c', char);

    ASSERT(ring_peek_front(&r, char) == 'a', "peek front should be 'a'");
    ASSERT(ring_peek_back(&r, char) == 'c', "peek back should be 'c'");

    ring_push_front(&r, 'd', char);
    ring_push_front(&r, 'e', char);

    ASSERT(ring_peek_front(&r, char) == 'e', "peek front should be 'e'");
    ASSERT(ring_peek_back(&r, char) == 'c', "peek back should be 'c'");
    PASS();
}

/* ========================================================================= */
/*  run_ring_tests                                                           */
/* ========================================================================= */

TEST_SUITE(ring, "ring.h tests")
{
    test_ring_init();
    test_push_pop_int();
    test_push_pop_char();
    test_peek();
    test_get_set_at();
    test_iterator_int();
    test_reverse_iterator_int();
    test_push_front_pop_back_int();
    test_push_front_pop_back_char();
    test_peek_both_ends_int();
    test_peek_both_ends_char();
}

