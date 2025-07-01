#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "scheduler.h"
#include "queue.h"
#include "log.h"
#include "utils.h"
#include "models.h"

// Globals defined in server.c
extern rescuer_type_t   *rescuer_list;
extern int               n_rescuers;
extern emergency_type_t *etype_list;
extern int               n_etypes;

static pthread_t sched_thr;
static volatile sig_atomic_t sched_stop = 0;
static bqueue_t *q_global = NULL;

// Arguments for the timer thread
typedef struct {
    int    req_id;
    int    n_units;
    int   *r_idxs;      // array of rescuer indices
    double delay_sec;   // travel + manage time
} timer_args_t;

// Detached thread that sleeps and then completes the emergency
static void *timer_thread(void *v) {
    timer_args_t *a = (timer_args_t *)v;
    struct timespec ts;
    ts.tv_sec  = (time_t)a->delay_sec;
    ts.tv_nsec = (long)((a->delay_sec - ts.tv_sec) * 1E9);
    nanosleep(&ts, NULL);

    // Free each rescuer and log completion
    for (int i = 0; i < a->n_units; i++) {
        rescuer_type_t *r = &rescuer_list[a->r_idxs[i]];
        r->number++;
        log_event("RESCUER_STATUS name=%s status=FREE", r->name);
    }
    log_event("EMERGENCY_STATUS id=%d status=COMPLETED", a->req_id);

    free(a->r_idxs);
    free(a);
    return NULL;
}

// Scheduler loop: consumes emergencies and assigns them
static void *scheduler_loop(void *arg) {
    bqueue_t *q = (bqueue_t *)arg;

    while (!sched_stop) {
        emergency_request_t req = bqueue_pop(q);

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
        int deadline   = priority_deadline_secs(req.priority);
        double max_travel = 0.0;
        
        for (int j = 0; j < et->n_required; j++) {
            int ridx = et->required_units[j];
            rescuer_type_t *r = &rescuer_list[ridx];

            if (r->number <= 0) {
                can_assign = false;
                break;
            }
            double t_travel = travel_time_secs(r, r->x, r->y, req.x, req.y);
            if (t_travel > max_travel) max_travel = t_travel;
            if (t_travel + t_manage > deadline) {
                can_assign = false;
                break;
            }
        }

        // 3) Assign or timeout
        if (can_assign) {
            // Allocate args for timer thread
            timer_args_t *targs = malloc(sizeof(timer_args_t));
            targs->req_id    = req.id;
            targs->n_units   = et->n_required;
            targs->delay_sec = max_travel + t_manage;
            targs->r_idxs    = malloc(sizeof(int) * et->n_required);

            pthread_t tid;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            for (int j = 0; j < et->n_required; j++) {
                int ridx = et->required_units[j];
                rescuer_type_t *r = &rescuer_list[ridx];

                log_event("ASSIGN emergenza %d → %s", req.id, r->name);
                log_event("EMERGENCY_STATUS id=%d status=ASSIGNED", req.id);
                log_event("RESCUER_STATUS name=%s status=BUSY", r->name);

                // Update rescuer state
                r->x      = req.x;
                r->y      = req.y;
                r->number--;

                // Track rescuer indices for timer freeing
                targs->r_idxs[j] = ridx;
            }

            // Create detached timer thread
            pthread_create(&tid, &attr, timer_thread, targs);
            pthread_attr_destroy(&attr);

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

