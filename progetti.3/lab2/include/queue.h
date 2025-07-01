#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <pthread.h>
#include "models.h"

/* Una coda circolare di emergency_request_t */
typedef struct {
    emergency_request_t *buf; /* array di capacità cap */
    size_t head, tail;        /* indici */
    size_t cap, count;        /* dimensione e numero di elementi */
    pthread_mutex_t mtx;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} bqueue_t;

/* Inizializza una bqueue con capacità cap (ritorna 0 o -1). */
int  bqueue_init(bqueue_t *q, size_t cap);

/* Distrugge la bqueue, libera le risorse. */
void bqueue_destroy(bqueue_t *q);

/* Inserisce *blocking* un item in coda (attende se piena). */
void bqueue_push(bqueue_t *q, emergency_request_t item);

/* Estrae *blocking* un item da coda (attende se vuota). */
emergency_request_t bqueue_pop(bqueue_t *q);

#endif /* QUEUE_H */

