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
 *         File: include/list.h
 *      Created: October 20, 2025
 *       Author: Wes Hampson
 *
 * Doubly-linked circular list implementation. Very similar to Linux's
 * 'list_head' structure, which I believe to be a very nice doubly-linked list
 * implementation for a kernel.
 * =============================================================================
 */

#ifndef __LIST_H
#define __LIST_H

#include <stdbool.h>

/**
 * Linked list node.
 */
struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

typedef struct list_node list_t;

/**
 * Empty list initializer. Initializes a list head by setting it's previous and
 * next pointers to itself, creating an empty list.
 *
 * Usage:
 *  list_t list = LIST_INITIALIZER(list);
 */
#define LIST_INITIALIZER(list)  { &(list), &(list) }

/**
 * Empty list initializer.
 *
 * Usage:
 *  list_t list_head;
 *  list_init(&list_head);
 */
#define list_init(head) { (head)->prev = (head); (head)->next = (head); }

/**
 * List node traversal for-loop iterator.
 *
 * @param it iterator name
 * @param list list to iterate
 *
 * Usage:
 *  list_t *list;
 *  for (list_iterator(it, list)) { ... }
 */
#define list_iterator(it, list) \
    struct list_node *it = (list)->next; (it) != (list); (it) = (it)->next

/**
 * Get a pointer to the structure containing the list node.
 *
 * @param node list node pointer
 * @param type struct type
 * @param member list member name in struct
 *
 * Usage:
 *  struct obj {
 *      list_t list;
 *      ...
 *  };
 *  struct list_node *n;    // e.g. from list_iterator
 *  struct obj *item = list_item(n, struct obj, list);
 */
#define list_item(node, type, member) \
    ((type *) (((char *) (node)) - offsetof(type, member)))

/**
 * Push an item into the front of the list.
 */
static inline void list_add(struct list_node *head, struct list_node *item)
{
    struct list_node *prev = head;
    struct list_node *next = head->next;

    item->prev = prev;
    prev->next = item;
    item->next = next;
    next->prev = item;
}

/**
 * Push an item into the end of the list.
 */
static inline void list_add_tail(struct list_node *head, struct list_node *item)
{
    // the end of the list points to the head,
    // so confusingly we insert before the list head
    struct list_node *prev = head->prev;
    struct list_node *next = head;

    item->prev = prev;
    prev->next = item;
    item->next = next;
    next->prev = item;
}

/**
 * Remove an item from its own list.
 */
static inline void list_remove(struct list_node *item)
{
    struct list_node *prev = item->prev;
    struct list_node *next = item->next;

    next->prev = prev;  // Breakin' the chains around me
    prev->next = next;  // Nobody else can bind me
    list_init(item);    // Take a good look around me
}                       // Now I'm breakin' the chains!

/**
 * Check whether a list has neighbors.
 */
static inline bool list_empty(struct list_node *head)
{
    return head->next == head;
}

#endif // __LIST_H
