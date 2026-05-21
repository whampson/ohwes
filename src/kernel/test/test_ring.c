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
#include <kernel/ring.h>

void test_ring(void)
{
    DECLARE_TEST("ring buffer");

    const size_t RingLength = 4;

    char buf[RingLength];
    struct ring _ring;
    struct ring *ring = &_ring;

    // init
    ring_init(ring, buf, RingLength);
    VERIFY_IS_TRUE(ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // put into rear
    VERIFY_IS_TRUE(ring_put(ring, 'A') == true);
    VERIFY_IS_TRUE(!ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // get from front
    VERIFY_IS_TRUE(ring_get(ring) == 'A');
    VERIFY_IS_TRUE(ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // put into front
    VERIFY_IS_TRUE(ring_insert(ring, 'a') == true);
    VERIFY_IS_TRUE(!ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // get from rear
    VERIFY_IS_TRUE(ring_erase(ring) == 'a');
    VERIFY_IS_TRUE(ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // fill from rear
    VERIFY_IS_TRUE(ring_put(ring, 'W') == true);
    VERIFY_IS_TRUE(ring_put(ring, 'X') == true);
    VERIFY_IS_TRUE(ring_put(ring, 'Y') == true);
    VERIFY_IS_TRUE(ring_put(ring, 'Z') == true);
    VERIFY_IS_TRUE(ring_put(ring, 'A') == false);
    VERIFY_IS_TRUE(!ring_empty(ring));
    VERIFY_IS_TRUE(ring_full(ring));

    // drain from front
    VERIFY_IS_TRUE(ring_get(ring) == 'W');
    VERIFY_IS_TRUE(ring_get(ring) == 'X');
    VERIFY_IS_TRUE(ring_get(ring) == 'Y');
    VERIFY_IS_TRUE(ring_get(ring) == 'Z');
    VERIFY_IS_TRUE(ring_get(ring) == '\0');
    VERIFY_IS_TRUE(ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // fill from front
    VERIFY_IS_TRUE(ring_insert(ring, 'a') == true);
    VERIFY_IS_TRUE(ring_insert(ring, 'b') == true);
    VERIFY_IS_TRUE(ring_insert(ring, 'c') == true);
    VERIFY_IS_TRUE(ring_insert(ring, 'd') == true);
    VERIFY_IS_TRUE(ring_insert(ring, 'e') == false);
    VERIFY_IS_TRUE(!ring_empty(ring));
    VERIFY_IS_TRUE(ring_full(ring));

    // drain from rear
    VERIFY_IS_TRUE(ring_erase(ring) == 'a');
    VERIFY_IS_TRUE(ring_erase(ring) == 'b');
    VERIFY_IS_TRUE(ring_erase(ring) == 'c');
    VERIFY_IS_TRUE(ring_erase(ring) == 'd');
    VERIFY_IS_TRUE(ring_erase(ring) == '\0');
    VERIFY_IS_TRUE(ring_empty(ring));
    VERIFY_IS_TRUE(!ring_full(ring));

    // combined front/rear usage
    VERIFY_IS_TRUE(ring_put(ring, '1') == true);
    VERIFY_IS_TRUE(ring_put(ring, '2') == true);
    VERIFY_IS_TRUE(ring_put(ring, '3') == true);
    VERIFY_IS_TRUE(ring_put(ring, '4') == true);
    VERIFY_IS_TRUE(ring_full(ring));
    VERIFY_IS_TRUE(ring_erase(ring) == '4');
    VERIFY_IS_TRUE(ring_erase(ring) == '3');
    VERIFY_IS_TRUE(ring_insert(ring, '5') == true);
    VERIFY_IS_TRUE(ring_insert(ring, '6') == true);
    VERIFY_IS_TRUE(ring_full(ring));
    VERIFY_IS_TRUE(ring_get(ring) == '6');
    VERIFY_IS_TRUE(ring_get(ring) == '5');
    VERIFY_IS_TRUE(ring_get(ring) == '1');
    VERIFY_IS_TRUE(ring_get(ring) == '2');
    VERIFY_IS_TRUE(ring_empty(ring));
}
