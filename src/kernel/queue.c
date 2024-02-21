#include <stdbool.h>
#include <string.h>
#include <queue.h>

// TODO: TEST TEST TEST!
// TODO: move to own lib.. libnb? Niobium Kernel NbOS

struct _queue
{
    char *buf;
    size_t buflen;
    size_t head;
    size_t tail;
    size_t count;
    bool empty;
    bool full;
};

void queue_init(queue_t *q, char *buf, size_t len)
{
    memset(q, 0, sizeof(queue_t));
    q->buf = buf;
    q->buflen = len;
    q->empty = true;
}

char queue_get(queue_t *q)
{
    char c;
    if (q == NULL || q->empty) {
        return 0;
    }

    c = q->buf[q->tail++];
    q->full = false;
    q->count--;

    if (q->tail >= q->buflen) {
        q->tail = 0;
    }

    if (q->tail == q->head) {
        q->empty = true;
    }

    return c;
}

void queue_put(queue_t *q, char c)
{
    if (q == NULL || q->full) {
        return;
    }

    q->buf[q->head++] = c;
    q->empty = false;
    q->count++;

    if (q->head >= q->buflen) {
        q->head = 0;
    }

    if (q->head == q->tail) {
        q->full = true;
    }
}

bool queue_empty(queue_t *q)
{
    return q->empty;
}

bool queue_full(queue_t *q)
{
    return q->full;
}
