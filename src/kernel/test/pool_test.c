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
 *         File: kernel/test/pool_test.c
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <list.h>
#include <pool.h>
#include <stdio.h>

struct device {
    uint32_t magic;
    uint32_t id;
    char name[8];
};

#define NUM_TEST_DEVICES 8
static struct device _pool[NUM_TEST_DEVICES];
static struct device _tmp_pool0[NUM_TEST_DEVICES];
static struct device _tmp_pool1[NUM_TEST_DEVICES];
static struct device _tmp_pool2[NUM_TEST_DEVICES];
static pool_t device_pool;

static struct device * create_device(int id, const char *name);
static int destroy_device(struct device *device);

void test_pool(void)
{
    static_assert(NUM_TEST_DEVICES >= 2, "NUM_TEST_DEVICES too small!");

    struct device *devices[NUM_TEST_DEVICES];
    struct device *dev;

    pool_t p0, p1, p2;
    p0 = create_pool(_tmp_pool0, "p0", 1, 32);
    p1 = create_pool(_tmp_pool1, "p1", 1, 64);
    p2 = create_pool(_tmp_pool2, "p2", 1, 6);

    destroy_pool(p0);

    device_pool = create_pool(_pool, "device_pool", sizeof(struct device), NUM_TEST_DEVICES);
    assert(device_pool != NULL);

    for (int i = 0; i < NUM_TEST_DEVICES; i++) {
        char name[8]; sprintf(name, "dev%d", i);
        dev = create_device(i, name);
        assert(dev != NULL);
        devices[i] = dev;
    }

    for (int i = 1; i < NUM_TEST_DEVICES; i++) {
        int ret = destroy_device(devices[i]);
        assert(ret == 0);
        devices[i] = NULL;
    }

    for (int i = 1; i < NUM_TEST_DEVICES; i++) {
        char name[8]; sprintf(name, "dev%d", i);
        dev = create_device(i, name);
        assert(dev != NULL);
        devices[i] = dev;
    }

    dev = pool_alloc(device_pool);
    assert(dev == NULL);

    for (int i = 0; i < 2; i++) {
        int ret = destroy_device(devices[i]);
        assert(ret == 0);
        devices[i] = NULL;
    }

    dev = create_device(123, "dev123");
    assert(dev != NULL);
    devices[0] = dev;

    dev = create_device(456, "dev456");
    assert(dev != NULL);
    devices[1] = dev;

    int ret = destroy_device(devices[5]);
    assert(ret == 0);
    dev = create_device(6969, "dev6969");
    assert(dev != NULL);
    devices[5] = dev;

    _kprint("device list:\n");
    for (int i = 0; i < NUM_TEST_DEVICES; i++) {
        if (devices[i] != NULL) {
            _kprint("  %d: %08X: %s\n", i, devices[i], devices[i]->name);
        }
    }

    destroy_pool(device_pool);
    destroy_pool(p2);
    destroy_pool(p1);
}

static struct device * create_device(int id, const char *name)
{
    struct device *dev = pool_alloc(device_pool);
    if (dev != NULL) {
        dev->magic = 'vedc';
        dev->id = id;
        strncpy(dev->name, name, sizeof(dev->name));
    }

    return dev;
}

static int destroy_device(struct device *device)
{
    return pool_free(device_pool, device);
}
