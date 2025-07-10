#ifndef DIGITAL_TWIN_H
#define DIGITAL_TWIN_H

#include <pthread.h>
#include "models.h"

typedef enum {
    IDLE,
    EN_ROUTE_TO_SCENE,
    ON_SCENE,
    RETURNING_TO_BASE
} rescuer_status_t;

typedef struct rescuer_dt {
    int id;
    int x, y;
    rescuer_type_t *type;
    rescuer_status_t status;
    pthread_t thread;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    int dest_x, dest_y;
    int manage_time;
    int emergency_id;
    int emergency_priority;
    char emergency_type[32];
    int preempt;
    int assigned;
} rescuer_dt_t;

int  digital_twin_factory(rescuer_type_t *types, int n_types,
                          rescuer_dt_t **out_list, int *out_n);
void digital_twin_shutdown(rescuer_dt_t *list, int n);
rescuer_dt_t *digital_twin_find_idle(rescuer_dt_t *list, int n,
                                     rescuer_type_t *type);
rescuer_dt_t *digital_twin_find_preemptible(rescuer_dt_t *list, int n,
                                            rescuer_type_t *type,
                                            int min_priority);
int  digital_twin_assign(rescuer_dt_t *dt, int emergency_id,
                         const char *etype, int priority,
                         int dest_x, int dest_y, int manage_time);
int  digital_twin_preempt(rescuer_dt_t *dt,
                          const emergency_request_t *new_req,
                          int manage_time,
                          emergency_request_t *out_old);

#endif /* DIGITAL_TWIN_H */
