#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdbool.h>
#include <stddef.h>

struct _queue {
    char *ring;
    uint32_t rptr;
    uint32_t wptr;
    size_t len;
    size_t count;
};

typedef struct _queue queue_t;

void q_init(queue_t *q, char *buf, size_t len);

bool q_empty(const queue_t *q);
bool q_full(const queue_t *q);

char q_get(queue_t *q);
void q_put(queue_t *q, char c);

#endif /* __QUEUE_H */
