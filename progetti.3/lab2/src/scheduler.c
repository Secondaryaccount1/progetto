#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "scheduler.h"
#include "queue.h"
#include "log.h"
#include "utils.h"
#include "models.h"
#include "digital_twin.h"

/* After this many seconds a priority 0 emergency becomes priority 1 */
#define AGING_THRESHOLD 60

// Globals defined in server.c. The scheduler assumes the logs directory
// already exists and is created by the server before threads start.
extern rescuer_type_t   *rescuer_list;
extern int               n_rescuers;
extern emergency_type_t *etype_list;
extern int               n_etypes;
extern rescuer_dt_t     *dt_list;
extern int               dt_count;
extern int               env_width;
extern int               env_height;

static pthread_t sched_thr;
static volatile sig_atomic_t sched_stop = 0;
static bqueue_t *q_global = NULL;

// Scheduler loop: consumes emergencies and assigns them
static void *scheduler_loop(void *arg) {
    bqueue_t *q = (bqueue_t *)arg;

    while (!sched_stop) {
        emergency_request_t req = bqueue_pop(q);

        long now = time(NULL);
        if (req.x < 0 || req.x >= env_width ||
            req.y < 0 || req.y >= env_height) {
            log_event("Invalid coords for request %d dropped", req.id);
            continue;
        }
        if (req.timestamp <= 0 || req.timestamp > now) {
            log_event("Invalid timestamp for request %d dropped", req.id);
            continue;
        }

        // 1) Find emergency type metadata
        emergency_type_t *et = NULL;
        for (int i = 0; i < n_etypes; i++) {
            if (strcmp(etype_list[i].name, req.type) == 0) {
                et = &etype_list[i];
                break;
            }
        }
        if (!et) {
            log_event("Unknown emergency type '%s'", req.type);
            continue;
        }

        // 2) Check availability and deadlines
        bool can_assign = true;
        double t_manage = (double)et->time_to_manage;

        int eff_priority = req.priority;
        if (req.priority == 0 && now - req.timestamp > AGING_THRESHOLD) {
            eff_priority = 1;
            log_event("AGING emergency %d priority 0→1", req.id);
        }

        int deadline   = priority_deadline_secs(eff_priority);
        double max_travel = 0.0;

        rescuer_dt_t *used[10];

        for (int j = 0; j < et->n_required; j++) {
            int ridx = et->required_units[j];
            rescuer_type_t *r = &rescuer_list[ridx];

            rescuer_dt_t *dt = digital_twin_find_idle(dt_list, dt_count, r);
            if (!dt) {
                can_assign = false;
                break;
            }
            used[j] = dt;

            double t_travel = travel_time_secs(r, dt->x, dt->y, req.x, req.y);
            if (t_travel > max_travel) max_travel = t_travel;
            if (t_travel + t_manage > deadline) {
                can_assign = false;
                break;
            }
        }

        // 3) Assign or timeout
        if (can_assign) {
            for (int j = 0; j < et->n_required; j++) {
                rescuer_dt_t *dt = used[j];
                rescuer_type_t *r = dt->type;

                log_event("ASSIGN emergenza %d → %s#%d", req.id, r->name, dt->id);
                log_event("EMERGENCY_STATUS id=%d status=ASSIGNED", req.id);

                digital_twin_assign(dt, req.id, req.x, req.y, et->time_to_manage);
            }

        } else {
            double needed = max_travel + t_manage;
            log_event("TIMEOUT emergenza %d (need=%.1f > d=%d)",
                      req.id, needed, deadline);
            log_event("EMERGENCY_STATUS id=%d status=TIMEOUT", req.id);
        }
    }
    return NULL;
}

int scheduler_start(bqueue_t *q) {
    sched_stop = 0;
    q_global   = q;
    return pthread_create(&sched_thr, NULL, scheduler_loop, q);
}

void scheduler_stop(void) {
    sched_stop = 1;
    pthread_mutex_lock(&q_global->mtx);
    pthread_cond_signal(&q_global->not_empty);
    pthread_mutex_unlock(&q_global->mtx);
    pthread_join(sched_thr, NULL);
}

