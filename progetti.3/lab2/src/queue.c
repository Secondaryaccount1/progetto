#include "queue.h"
#include <stdlib.h>
#include <pthread.h>

int bqueue_init(bqueue_t *q, size_t cap) {
    q->buf   = malloc(sizeof(*q->buf) * cap);
    if (!q->buf) return -1;
    q->cap   = cap;
    q->count = q->head = q->tail = 0;
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full,  NULL);
    return 0;
}

void bqueue_destroy(bqueue_t *q) {
    free(q->buf);
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

void bqueue_push(bqueue_t *q, emergency_request_t item) {
    pthread_mutex_lock(&q->mtx);
    while (q->count == q->cap)
        pthread_cond_wait(&q->not_full, &q->mtx);
    q->buf[q->tail] = item;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mtx);
}

emergency_request_t bqueue_pop(bqueue_t *q) {
    pthread_mutex_lock(&q->mtx);
    while (q->count == 0)
        pthread_cond_wait(&q->not_empty, &q->mtx);
    emergency_request_t item = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mtx);
    return item;
}
