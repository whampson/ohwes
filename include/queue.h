#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdbool.h>
#include <stddef.h>

struct char_queue {
    char *ring;
    uint32_t rptr;
    uint32_t wptr;
    size_t length;
    size_t count;
};

void q_init(struct char_queue *q, char *buf, size_t length);

bool q_empty(const struct char_queue *q);
bool q_full(const struct char_queue *q);

char q_get(struct char_queue *q);            // pop from front of queue
void q_put(struct char_queue *q, char c);    // push to back of queue

char q_erase(struct char_queue *q);          // pop from back of queue
// void q_insert(struct char_queue *q, char c);  // push to front of queue

size_t q_length(struct char_queue *q);       // size of ring buffer
size_t q_count(struct char_queue *q);        // number of chars in queue

#endif /* __QUEUE_H */
