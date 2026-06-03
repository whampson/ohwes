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

typedef struct {
    int id;
    char name[16];
} test_struct;

static void test_ring_init(void)
{
    int buffer[10];
    struct ring r = RING_INIT(buffer, 10);

    TEST("basic initialization");
    ASSERT_EQ_INT((void *)buffer, r.buf, "buffer pointer incorrect");
    ASSERT_EQ_INT(10, r.cap, "capacity incorrect");
    ASSERT_EQ_INT(0, r.count, "count should be 0");
    ASSERT_EQ_INT(0, r.head, "head should be 0");
    ASSERT_EQ_INT(0, r.tail, "tail should be 0");
    PASS();
}

static void test_ring_clear(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);

    TEST("clear/reset");
    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);

    ASSERT(r.count == 2, "expected 2 items in ring before clear");
    ASSERT(r.head == 0, "expected head to be 0 before clear");
    ASSERT(r.tail == 2, "expected tail to be 2 before clear");

    ring_clear(&r);

    ASSERT(r.count == 0, "expected 0 items in ring after clear");
    ASSERT(r.head == 0, "expected head to be 0 after clear");
    ASSERT(r.tail == 0, "expected tail to be 0 after clear");
    PASS();
}

static void test_push_pop_int(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    int val;

    TEST("push back and pop front (int)");
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
    struct ring r = RING_INIT(buffer, 5);
    char val;

    TEST("push back and pop front (char)");
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

static void test_push_pop_struct(void)
{
    test_struct buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    test_struct val;

    TEST("push back and pop front (struct)");
    val.id = 10;
    strcpy(val.name, "Nick");
    ASSERT(ring_push_back(&r, val, test_struct) == 1, "push struct failed");

    val.id = 20;
    strcpy(val.name, "Roger");
    ASSERT(ring_push_back(&r, val, test_struct) == 1, "push struct failed");

    val.id = 30;
    strcpy(val.name, "Richard");
    ASSERT(ring_push_back(&r, val, test_struct) == 1, "push struct failed");

    val.id = 40;
    strcpy(val.name, "David");
    ASSERT(ring_push_back(&r, val, test_struct) == 1, "push struct failed");

    val.id = 50;
    strcpy(val.name, "Syd");
    ASSERT(ring_push_back(&r, val, test_struct) == 1, "push struct failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");
    ASSERT(ring_push_back(&r, val, test_struct) == 0, "push on full ring should fail");

    ASSERT(ring_pop_front(&r, val, test_struct) == 1, "pop struct failed");
    ASSERT(val.id == 10 && strcmp(val.name, "Nick") == 0, "expected id=10, name=Nick");

    ASSERT(ring_pop_front(&r, val, test_struct) == 1, "pop struct failed");
    ASSERT(val.id == 20 && strcmp(val.name, "Roger") == 0, "expected id=20, name=Roger");

    ASSERT(ring_pop_front(&r, val, test_struct) == 1, "pop struct failed");
    ASSERT(val.id == 30 && strcmp(val.name, "Richard") == 0, "expected id=30, name=Richard");

    ASSERT(ring_pop_front(&r, val, test_struct) == 1, "pop struct failed");
    ASSERT(val.id == 40 && strcmp(val.name, "David") == 0, "expected id=40, name=David");

    ASSERT(ring_pop_front(&r, val, test_struct) == 1, "pop struct failed");
    ASSERT(val.id == 50 && strcmp(val.name, "Syd") == 0, "expected id=50, name=Syd");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_front(&r, val, test_struct) == 0, "pop from empty should fail");
    PASS();
}

static void test_peek(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);

    TEST("peek front and back");
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
    struct ring r = RING_INIT(buffer, 5);
    int val;

    TEST("get and set at position");
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
    struct ring r = RING_INIT(buffer, 5);
    int val;

    TEST("forward iteration");
    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    int expected[] = {10, 20, 30};
    size_t i = 0;

    for (ring_iterator(&r, val, int))
        ASSERT(val == expected[i++], "expected %d, got %d", expected[i-1], val);

    PASS();
}

static void test_iterator_struct(void)
{
    test_struct buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    test_struct val;

    TEST("forward iteration (struct)");
    val.id = 10;
    strcpy(val.name, "Nick");
    ring_push_back(&r, val, test_struct);

    val.id = 20;
    strcpy(val.name, "Roger");
    ring_push_back(&r, val, test_struct);

    val.id = 30;
    strcpy(val.name, "Richard");
    ring_push_back(&r, val, test_struct);

    test_struct expected[3] = {
        {10, "Nick"},
        {20, "Roger"},
        {30, "Richard"}
    };
    size_t i = 0;

    for (ring_iterator(&r, val, test_struct)) {
        ASSERT(val.id == expected[i].id && strcmp(val.name, expected[i].name) == 0,
            "expected id=%d, name=%s, got id=%d, name=%s",
            expected[i].id, expected[i].name, val.id, val.name);
        i++;
    }

    PASS();
}

static void test_reverse_iterator_int(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    int val;

    TEST("reverse iteration");
    ring_push_back(&r, 10, int);
    ring_push_back(&r, 20, int);
    ring_push_back(&r, 30, int);

    int expected[] = {30, 20, 10};
    size_t i = 0;

    for (ring_reverse_iterator(&r, val, int))
        ASSERT(val == expected[i++], "expected %d, got %d", expected[i-1], val);

    PASS();
}

static void test_reverse_iterator_struct(void)
{
    test_struct buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    test_struct val;

    TEST("reverse iteration (struct)");
    val.id = 10;
    strcpy(val.name, "Nick");
    ring_push_back(&r, val, test_struct);

    val.id = 20;
    strcpy(val.name, "Roger");
    ring_push_back(&r, val, test_struct);

    val.id = 30;
    strcpy(val.name, "Richard");
    ring_push_back(&r, val, test_struct);

    test_struct expected[3] = {
        {30, "Richard"},
        {20, "Roger"},
        {10, "Nick"}
    };
    size_t i = 0;

    for (ring_reverse_iterator(&r, val, test_struct)) {
        ASSERT(val.id == expected[i].id && strcmp(val.name, expected[i].name) == 0,
            "expected id=%d, name=%s, got id=%d, name=%s",
            expected[i].id, expected[i].name, val.id, val.name);
        i++;
    }

    PASS();
}

static void test_push_front_pop_back_int(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    int val;

    TEST("push front and pop back (int)");
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
    struct ring r = RING_INIT(buffer, 5);
    char val;

    TEST("push front and pop back (char)");
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

static void test_push_front_pop_back_struct(void)
{
    test_struct buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    test_struct val;

    TEST("push front and pop back (struct)");
    val.id = 10;
    strcpy(val.name, "Nick");
    ASSERT(ring_push_front(&r, val, test_struct) == 1, "push front failed");

    val.id = 20;
    strcpy(val.name, "Roger");
    ASSERT(ring_push_front(&r, val, test_struct) == 1, "push front failed");

    val.id = 30;
    strcpy(val.name, "Richard");
    ASSERT(ring_push_front(&r, val, test_struct) == 1, "push front failed");

    val.id = 40;
    strcpy(val.name, "David");
    ASSERT(ring_push_front(&r, val, test_struct) == 1, "push front failed");

    val.id = 50;
    strcpy(val.name, "Syd");
    ASSERT(ring_push_front(&r, val, test_struct) == 1, "push front failed");

    ASSERT(ring_full(&r) == 1, "ring should be full");
    ASSERT(ring_push_front(&r, val, test_struct) == 0, "push on full should fail");

    ASSERT(ring_pop_back(&r, val, test_struct) == 1, "pop back failed");
    ASSERT(val.id == 10 && strcmp(val.name, "Nick") == 0, "expected id=10, name=Nick");

    ASSERT(ring_pop_back(&r, val, test_struct) == 1, "pop back failed");
    ASSERT(val.id == 20 && strcmp(val.name, "Roger") == 0, "expected id=20, name=Roger");

    ASSERT(ring_pop_back(&r, val, test_struct) == 1, "pop back failed");
    ASSERT(val.id == 30 && strcmp(val.name, "Richard") == 0, "expected id=30, name=Richard");

    ASSERT(ring_pop_back(&r, val, test_struct) == 1, "pop back failed");
    ASSERT(val.id == 40 && strcmp(val.name, "David") == 0, "expected id=40, name=David");

    ASSERT(ring_pop_back(&r, val, test_struct) == 1, "pop back failed");
    ASSERT(val.id == 50 && strcmp(val.name, "Syd") == 0, "expected id=50, name=Syd");

    ASSERT(ring_empty(&r) == 1, "ring should be empty");
    ASSERT(ring_pop_back(&r, val, test_struct) == 0, "pop from empty should fail");
    PASS();
}

static void test_peek_both_ends_int(void)
{
    int buffer[5];
    struct ring r = RING_INIT(buffer, 5);

    TEST("peek both ends after mixed ops (int)");
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
    struct ring r = RING_INIT(buffer, 5);

    TEST("peek both ends after mixed ops (char)");
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

static void test_peek_both_ends_struct(void)
{
    test_struct buffer[5];
    struct ring r = RING_INIT(buffer, 5);
    test_struct val;

    TEST("peek both ends after mixed ops (struct)");
    val.id = 10;
    strcpy(val.name, "Nick");
    ring_push_back(&r, val, test_struct);

    val.id = 20;
    strcpy(val.name, "Roger");
    ring_push_back(&r, val, test_struct);

    val.id = 30;
    strcpy(val.name, "Richard");
    ring_push_back(&r, val, test_struct);

    val.id = 40;
    strcpy(val.name, "David");
    ring_push_front(&r, val, test_struct);

    val.id = 50;
    strcpy(val.name, "Syd");
    ring_push_front(&r, val, test_struct);

    ASSERT(ring_peek_front(&r, test_struct).id == 50
            && strcmp(ring_peek_front(&r, test_struct).name, "Syd") == 0,
        "peek front should be id=50, name=Syd");
    ASSERT(ring_peek_back(&r, test_struct).id == 30
            && strcmp(ring_peek_back(&r, test_struct).name, "Richard") == 0,
        "peek back should be id=30, name=Richard");
    PASS();
}

/* ========================================================================= */
/*  run_ring_tests                                                           */
/* ========================================================================= */

TEST_SUITE(ring, "ring.h tests")
{
    printf(COLOR_YELLOW "[initialization]" COLOR_RESET "\n");
    test_ring_init();
    test_ring_clear();

    printf(COLOR_YELLOW "[push/pop]" COLOR_RESET "\n");
    test_push_pop_int();
    test_push_pop_char();
    test_push_pop_struct();

    printf(COLOR_YELLOW "[get/set/peek]" COLOR_RESET "\n");
    test_peek();
    test_get_set_at();

    printf(COLOR_YELLOW "[iterator]" COLOR_RESET "\n");
    test_iterator_int();
    test_iterator_struct();
    test_reverse_iterator_int();
    test_reverse_iterator_struct();

    printf(COLOR_YELLOW "[double-ended push/pop/peek]" COLOR_RESET "\n");
    test_push_front_pop_back_int();
    test_push_front_pop_back_char();
    test_push_front_pop_back_struct();
    test_peek_both_ends_int();
    test_peek_both_ends_char();
    test_peek_both_ends_struct();
}
