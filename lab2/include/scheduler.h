#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "models.h"
#include "queue.h"

/* Avvia il thread dello scheduler: consumer sulla bqueue */
int  scheduler_start(bqueue_t *q);
void scheduler_stop(void);

/* Deadlock monitor utilities */
void scheduler_check_deadlocks(void);
void scheduler_debug_add_paused(emergency_request_t req);
int  scheduler_debug_get_paused_count(void);
emergency_request_t scheduler_debug_get_paused(int idx);

/* Emergency table utilities */
void scheduler_add_emergency(const emergency_request_t *req);
int  scheduler_set_emergency_status(int id, emergency_status_t st);
int  scheduler_debug_get_emergency_count(void);
emergency_t scheduler_debug_get_emergency(int idx);

#endif /* SCHEDULER_H */
