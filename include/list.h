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
 *         File: include/list.h
 *      Created: November 20, 2024
 *       Author: Wes Hampson
 *
 * Doubly-linked circular list implementation. Very similar to Linux's
 * 'list_head' structure, which I believe to be a very nice doubly-linked list
 * implementation for a kernel.
 * =============================================================================
 */

#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>
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
 * Empty list initializer.
 */
#define LIST_INITIALIZER(name)  { &(name), &(name) }

/**
 * List node traversal for-loop iterator.
 */
#define list_iterator(head, pos)        \
    (pos) = (head)->next; (pos) != (head); (pos) = (pos)->next

/**
 * Cast the list pointer to its enclosing structure.
 */
#define list_item(ptr, type, member) \
    ((type *) (((char *) (ptr)) - offsetof(type, member)))

/**
 * Initializes a list head by setting it's previous and next pointers to
 * itself, creating an empty list.
 */
void list_init(struct list_node *head);

/**
 * Returns 'true' if the specified list is empty.
 */
bool list_empty(struct list_node *head);

/**
 * Add an item to the list after the specified list head.
 */
void list_add(struct list_node *head, struct list_node *item);

/**
 * Add an item to the list before the specified list head.
 */
void list_add_tail(struct list_node *head, struct list_node *item);

/**
 * Remove an item from its own list.
 */
void list_remove(struct list_node *item);

#endif // __LIST_H
