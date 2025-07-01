#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "models.h"
#include "queue.h"

/* Avvia il thread dello scheduler: consumer sulla bqueue */
int  scheduler_start(bqueue_t *q);
void scheduler_stop(void);

#endif /* SCHEDULER_H */
