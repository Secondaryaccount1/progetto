#include "digital_twin.h"
#include "log.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile int stop_twins = 0;

static void ts_sleep(double secs)
{
    struct timespec ts;
    ts.tv_sec  = (time_t)secs;
    ts.tv_nsec = (long)((secs - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
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
        log_event("RESCUER_STATUS id=%d type=%s status=EN_ROUTE_TO_SCENE", dt->id, dt->type->name);
        pthread_mutex_unlock(&dt->mtx);

        double ttrav = travel_time_secs(dt->type, dt->x, dt->y, dx, dy);
        ts_sleep(ttrav);

        pthread_mutex_lock(&dt->mtx);
        dt->x = dx; dt->y = dy;
        dt->status = ON_SCENE;
        log_event("RESCUER_STATUS id=%d type=%s status=ON_SCENE", dt->id, dt->type->name);
        pthread_mutex_unlock(&dt->mtx);

        ts_sleep(mtime);

        pthread_mutex_lock(&dt->mtx);
        dt->status = RETURNING_TO_BASE;
        log_event("RESCUER_STATUS id=%d type=%s status=RETURNING_TO_BASE", dt->id, dt->type->name);
        pthread_mutex_unlock(&dt->mtx);

        double tret = travel_time_secs(dt->type, dx, dy, dt->type->x, dt->type->y);
        ts_sleep(tret);

        pthread_mutex_lock(&dt->mtx);
        dt->x = dt->type->x;
        dt->y = dt->type->y;
        dt->status = IDLE;
        dt->assigned = 0;
        dt->type->number++;
        log_event("RESCUER_STATUS id=%d type=%s status=IDLE", dt->id, dt->type->name);
        log_event("EMERGENCY_STATUS id=%d status=COMPLETED", eid);
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
            pthread_create(&dt->thread, NULL, twin_loop, dt);
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
    for (int i=0;i<n;i++) {
        if (list[i].type==type && list[i].status==IDLE)
            return &list[i];
    }
    return NULL;
}

int digital_twin_assign(rescuer_dt_t *dt, int emergency_id,
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
    dt->assigned = 1;
    dt->type->number--; /* decrease available */
    pthread_cond_signal(&dt->cond);
    pthread_mutex_unlock(&dt->mtx);
    return 0;
}
