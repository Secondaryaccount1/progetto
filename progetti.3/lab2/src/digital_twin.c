#include "digital_twin.h"
#include "log.h"
#include "utils.h"
#include "scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile int stop_twins = 0;

static int ts_sleep_intr(rescuer_dt_t *dt, double secs)
{
    struct timespec ts;
    double rem = secs;
    int interrupted = 0;
    while (rem > 0) {
        double step = rem > 0.5 ? 0.5 : rem;
        ts.tv_sec  = (time_t)step;
        ts.tv_nsec = (long)((step - ts.tv_sec) * 1e9);
        nanosleep(&ts, NULL);
        pthread_mutex_lock(&dt->mtx);
        if (dt->preempt) {
            interrupted = 1;
            pthread_mutex_unlock(&dt->mtx);
            break;
        }
        pthread_mutex_unlock(&dt->mtx);
        rem -= step;
    }
    return interrupted;
}

static void *twin_loop(void *arg)
{
    rescuer_dt_t *dt = arg;
    pthread_mutex_lock(&dt->mtx);
    while (!stop_twins) {
        while (!dt->assigned && !stop_twins)
            pthread_cond_wait(&dt->cond, &dt->mtx);
        if (stop_twins) break;

        int dx = dt->dest_x;
        int dy = dt->dest_y;
        int mtime = dt->manage_time;
        int eid = dt->emergency_id;
        dt->status = EN_ROUTE_TO_SCENE;
        scheduler_set_emergency_status(eid, EM_STATUS_IN_PROGRESS);
        log_event_ex("DT", "RESCUER_STATUS",
                    "id=%d type=%s status=EN_ROUTE_TO_SCENE",
                    dt->id, dt->type->name);
        log_event_ex("DT", "EMERGENCY_STATUS",
                    "id=%d status=IN_PROGRESS", eid);
        pthread_mutex_unlock(&dt->mtx);

        double ttrav = travel_time_secs(dt->type, dt->x, dt->y, dx, dy);
        if (ts_sleep_intr(dt, ttrav)) {
            pthread_mutex_lock(&dt->mtx);
            dt->preempt = 0;
            continue;
        }

        pthread_mutex_lock(&dt->mtx);
        dt->x = dx; dt->y = dy;
        dt->status = ON_SCENE;
        log_event_ex("DT", "RESCUER_STATUS",
                    "id=%d type=%s status=ON_SCENE",
                    dt->id, dt->type->name);
        pthread_mutex_unlock(&dt->mtx);

        if (ts_sleep_intr(dt, mtime)) {
            pthread_mutex_lock(&dt->mtx);
            dt->preempt = 0;
            continue;
        }

        pthread_mutex_lock(&dt->mtx);
        dt->status = RETURNING_TO_BASE;
        log_event_ex("DT", "RESCUER_STATUS",
                    "id=%d type=%s status=RETURNING_TO_BASE",
                    dt->id, dt->type->name);
        pthread_mutex_unlock(&dt->mtx);

        double tret = travel_time_secs(dt->type, dx, dy, dt->type->x, dt->type->y);
        if (ts_sleep_intr(dt, tret)) {
            pthread_mutex_lock(&dt->mtx);
            dt->preempt = 0;
            continue;
        }

        pthread_mutex_lock(&dt->mtx);
        dt->x = dt->type->x;
        dt->y = dt->type->y;
        dt->status = IDLE;
        dt->assigned = 0;
        atomic_fetch_add(&dt->type->number, 1);
        log_event_ex("DT", "RESCUER_STATUS",
                    "id=%d type=%s status=IDLE",
                    dt->id, dt->type->name);
        scheduler_set_emergency_status(eid, EM_STATUS_COMPLETED);
        log_event_ex("DT", "EMERGENCY_STATUS",
                    "id=%d status=COMPLETED", eid);
        pthread_mutex_unlock(&dt->mtx);
    }
    pthread_mutex_unlock(&dt->mtx);
    return NULL;
}

int digital_twin_factory(rescuer_type_t *types, int n_types,
                         rescuer_dt_t **out_list, int *out_n)
{
    int total = 0;
    for (int i=0;i<n_types;i++) total += types[i].number;

    rescuer_dt_t *arr = calloc(total, sizeof(rescuer_dt_t));
    if (!arr) return -1;

    int id=0;
    for (int i=0;i<n_types;i++) {
        for (int j=0;j<types[i].number;j++) {
            rescuer_dt_t *dt = &arr[id];
            dt->id = id;
            dt->x = types[i].x;
            dt->y = types[i].y;
            dt->type = &types[i];
            dt->status = IDLE;
            pthread_mutex_init(&dt->mtx, NULL);
            pthread_cond_init(&dt->cond, NULL);
            if (pthread_create(&dt->thread, NULL, twin_loop, dt) != 0) {
                log_event_ex("DT", "THREAD", "failed to start twin %d", id);
                free(arr);
                return -1;
            }
            id++;
        }
    }
    *out_list = arr;
    *out_n = total;
    return 0;
}

void digital_twin_shutdown(rescuer_dt_t *list, int n)
{
    stop_twins = 1;
    for (int i=0;i<n;i++) {
        pthread_mutex_lock(&list[i].mtx);
        pthread_cond_signal(&list[i].cond);
        pthread_mutex_unlock(&list[i].mtx);
    }
    for (int i=0;i<n;i++) {
        pthread_join(list[i].thread, NULL);
        pthread_mutex_destroy(&list[i].mtx);
        pthread_cond_destroy(&list[i].cond);
    }
    free(list);
}

rescuer_dt_t *digital_twin_find_idle(rescuer_dt_t *list, int n,
                                     rescuer_type_t *type)
{
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&list[i].mtx);
        int match = (list[i].type == type && list[i].status == IDLE);
        pthread_mutex_unlock(&list[i].mtx);
        if (match)
            return &list[i];
    }
    return NULL;
}

int digital_twin_assign(rescuer_dt_t *dt, int emergency_id,
                        const char *etype, int priority,
                        int dest_x, int dest_y, int manage_time)
{
    pthread_mutex_lock(&dt->mtx);
    if (dt->status!=IDLE) {
        pthread_mutex_unlock(&dt->mtx);
        return -1;
    }
    dt->dest_x = dest_x;
    dt->dest_y = dest_y;
    dt->manage_time = manage_time;
    dt->emergency_id = emergency_id;
    dt->emergency_priority = priority;
    strncpy(dt->emergency_type, etype, sizeof(dt->emergency_type)-1);
    dt->emergency_type[sizeof(dt->emergency_type)-1] = '\0';
    dt->assigned = 1;
    dt->preempt = 0;
    atomic_fetch_sub(&dt->type->number, 1); /* decrease available */
    pthread_cond_signal(&dt->cond);
    pthread_mutex_unlock(&dt->mtx);
    return 0;
}

rescuer_dt_t *digital_twin_find_preemptible(rescuer_dt_t *list, int n,
                                            rescuer_type_t *type,
                                            int min_priority)
{
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&list[i].mtx);
        int match = (list[i].type == type &&
                     (list[i].status == EN_ROUTE_TO_SCENE ||
                      list[i].status == ON_SCENE) &&
                     list[i].emergency_priority < min_priority);
        pthread_mutex_unlock(&list[i].mtx);
        if (match)
            return &list[i];
    }
    return NULL;
}

int digital_twin_preempt(rescuer_dt_t *dt,
                          const emergency_request_t *new_req,
                          int manage_time,
                          emergency_request_t *out_old)
{
    pthread_mutex_lock(&dt->mtx);
    if (dt->status==IDLE || dt->status==RETURNING_TO_BASE) {
        pthread_mutex_unlock(&dt->mtx);
        return -1;
    }
    if (out_old) {
        out_old->id = dt->emergency_id;
        strncpy(out_old->type, dt->emergency_type, sizeof(out_old->type));
        out_old->x = dt->dest_x;
        out_old->y = dt->dest_y;
        out_old->priority = dt->emergency_priority;
        out_old->timestamp = time(NULL);
    }

    dt->dest_x = new_req->x;
    dt->dest_y = new_req->y;
    dt->manage_time = manage_time;
    dt->emergency_id = new_req->id;
    dt->emergency_priority = new_req->priority;
    strncpy(dt->emergency_type, new_req->type, sizeof(dt->emergency_type)-1);
    dt->emergency_type[sizeof(dt->emergency_type)-1] = '\0';
    dt->preempt = 1;
    pthread_cond_signal(&dt->cond);
    pthread_mutex_unlock(&dt->mtx);
    return 0;
}
