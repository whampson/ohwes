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
 *         File: src/kernel/list.c
 *      Created: November 20, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <kernel/list.h>

static void insert_into_list(
    struct list_node *prev, struct list_node *next,
    struct list_node *item)
{
    next->prev = item;
    prev->next = item;
    item->prev = prev;
    item->next = next;
}

static void remove_from_list(
    struct list_node *prev, struct list_node *next,
    struct list_node *item)
{
    next->prev = prev;  // Breakin' the chains around me...
    prev->next = next;  // Nobody else can bind me...
    item->prev = item;  // Take a good look around me..
    item->next = item;  // Now I'm breakin' the chains!
}

void list_init(struct list_node *head)
{
    head->prev = head;
    head->next = head;
}

bool list_empty(struct list_node *head)
{
    return head->next == head;
}

void list_add(struct list_node *head, struct list_node *item)
{
    insert_into_list(head->prev, head, item);
}

void list_add_tail(struct list_node *head, struct list_node *item)
{
    insert_into_list(head, head->next, item);
}

void list_remove(struct list_node *item)
{
    remove_from_list(item->prev, item->next, item);
}
