#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct _queue queue_t;

void queue_init(queue_t *q, char *buf, size_t len);
char queue_get(queue_t *q);
void queue_put(queue_t *q, char c);
bool queue_empty(queue_t *q);
bool queue_full(queue_t *q);

#endif /* __QUEUE_H */
