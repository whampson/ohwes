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
 *         File: kernel/test/test_list.c
 *      Created: December 20, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <test.h>
#include <kernel/ohwes.h>
#include <kernel/list.h>

struct thing {
    struct list_node node;
    int value;
};
static list_t thing_list;
static struct thing _thing_buf[8];

static int count_items(list_t *list)
{
    int count = 0;
    for (list_iterator(e, list)) {
        count++;
    }

    return count;
}

void test_list(void)
{
    DECLARE_TEST("linked list");

    int i;

    zeromem(_thing_buf, sizeof(_thing_buf));
    for (int i = 0; i < countof(_thing_buf); i++) {
        _thing_buf[i].value = i;
    }

    // initial empty list
    list_init(&thing_list);
    VERIFY_IS_TRUE(list_empty(&thing_list));

    // add one item
    list_add(&thing_list, &_thing_buf[0].node);
    VERIFY_IS_FALSE(list_empty(&thing_list));
    VERIFY_ARE_EQUAL(1, count_items(&thing_list));

    // remove item
    list_remove(&_thing_buf[0].node);
    VERIFY_IS_TRUE(list_empty(&thing_list));
    VERIFY_ARE_EQUAL(0, count_items(&thing_list));

    // add items to tail
    list_add_tail(&thing_list, &_thing_buf[0].node);
    list_add_tail(&thing_list, &_thing_buf[1].node);
    list_add_tail(&thing_list, &_thing_buf[2].node);
    VERIFY_IS_FALSE(list_empty(&thing_list));
    VERIFY_ARE_EQUAL(3, count_items(&thing_list));

    // ensure items are in correct order for tail insertion
    i = 0;
    for (list_iterator(e, &thing_list)) {
        struct thing *item = list_item(e, struct thing, node);
        switch (i) {
            case 0: VERIFY_ARE_EQUAL(0, item->value); break;
            case 1: VERIFY_ARE_EQUAL(1, item->value); break;
            case 2: VERIFY_ARE_EQUAL(2, item->value); break;
        }
        i++;
    }

    // remove an item and check order
    i = 0;
    list_remove(&_thing_buf[1].node);
    VERIFY_IS_FALSE(list_empty(&thing_list));
    VERIFY_ARE_EQUAL(2, count_items(&thing_list));
    for (list_iterator(e, &thing_list)) {
        struct thing *item = list_item(e, struct thing, node);
        switch (i) {
            case 0: VERIFY_ARE_EQUAL(0, item->value); break;
            case 1: VERIFY_ARE_EQUAL(2, item->value); break;
        }
        i++;
    }

    // clear list
    list_remove(&thing_list);
    VERIFY_IS_TRUE(list_empty(&thing_list));

    // insert at head
    list_add(&thing_list, &_thing_buf[0].node);
    list_add(&thing_list, &_thing_buf[1].node);
    list_add(&thing_list, &_thing_buf[2].node);
    VERIFY_IS_FALSE(list_empty(&thing_list));
    VERIFY_ARE_EQUAL(3, count_items(&thing_list));

    // ensure items are in correct order for head insertion
    i = 0;
    for (list_iterator(e, &thing_list)) {
        struct thing *item = list_item(e, struct thing, node);
        switch (i) {
            case 0: VERIFY_ARE_EQUAL(2, item->value); break;
            case 1: VERIFY_ARE_EQUAL(1, item->value); break;
            case 2: VERIFY_ARE_EQUAL(0, item->value); break;
        }
        i++;
    }

    // add more items at tail
    list_add_tail(&thing_list, &_thing_buf[3].node);
    list_add_tail(&thing_list, &_thing_buf[4].node);
    list_add_tail(&thing_list, &_thing_buf[5].node);
    list_add_tail(&thing_list, &_thing_buf[6].node);

    // and one more at head
    list_add(&thing_list, &_thing_buf[7].node);

    // list order should now be: 7 2 1 0 3 4 5 6
    i = 0;
    for (list_iterator(e, &thing_list)) {
        struct thing *item = list_item(e, struct thing, node);
        switch (i) {
            case 0: VERIFY_ARE_EQUAL(7, item->value); break;
            case 1: VERIFY_ARE_EQUAL(2, item->value); break;
            case 2: VERIFY_ARE_EQUAL(1, item->value); break;
            case 3: VERIFY_ARE_EQUAL(0, item->value); break;
            case 4: VERIFY_ARE_EQUAL(3, item->value); break;
            case 5: VERIFY_ARE_EQUAL(4, item->value); break;
            case 6: VERIFY_ARE_EQUAL(5, item->value); break;
            case 7: VERIFY_ARE_EQUAL(6, item->value); break;
        }
        i++;
    }

    // success!
}
