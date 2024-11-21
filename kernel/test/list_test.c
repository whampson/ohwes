/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/test/list_test.c
 *      Created: November 20, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <list.h>
#include <pool.h>

struct device {
    struct list_node list;
    uint16_t major, minor;
    const char *name;
};

static struct list_node device_list;

#define NUM_ITEMS 8
static struct device _item_pool[NUM_ITEMS];
static pool_t item_pool;

static void add_device(const char *name, uint16_t major, uint16_t minor);
static void remove_device(const char *name);
static void enumerate_devices(void);

void test_list(void)
{
    item_pool = create_pool("device_pool", _item_pool, NUM_ITEMS, sizeof(struct device));
    list_init(&device_list);

    add_device("/dev/tty1", 1, 1);
    add_device("/dev/tty2", 1, 2);
    add_device("/dev/tty3", 1, 3);
    add_device("/dev/fd0", 2, 0);
    add_device("/dev/pcspk", 3, 0);
    enumerate_devices();

    remove_device("/dev/fd0");
    remove_device("/dev/tty2");
    enumerate_devices();

    destroy_pool(item_pool);
}

static void add_device(const char *name, uint16_t major, uint16_t minor)
{
    struct device *new_dev = pool_alloc(item_pool);
    new_dev->name = name;
    new_dev->major = major;
    new_dev->minor = minor;

    list_add_tail(&device_list, &new_dev->list);
}

static void remove_device(const char *name)
{
    struct list_node *entry;
    struct device *to_remove = NULL;

    for (list_iterator(&device_list, entry)) {
        struct device *dev = list_item(entry, struct device, list);
        if (strcmp(dev->name, name) == 0) {
            to_remove = dev;
            break;
        }
    }

    if (to_remove) {
        list_remove(&to_remove->list);
        pool_free(item_pool, to_remove);
    }
}

static void enumerate_devices(void)
{
    struct list_node *entry;

    _kprint("device list:\n");
    for (list_iterator(&device_list, entry)) {
        struct device *dev = list_item(entry, struct device, list);
        _kprint("    { %d, %d, %s }\n", dev->major, dev->minor, dev->name);
    }
}
