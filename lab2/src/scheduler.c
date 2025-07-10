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

/* Dopo questo numero di secondi una emergenza di priorità 0 diventa 1 */
#define AGING_THRESHOLD 60

// Variabili globali definite in server.c. Lo scheduler presume che la
// directory dei log esista già ed è creata dal server prima del lancio
// dei thread.
extern rescuer_type_t   *rescuer_list;
extern int               n_rescuers;
extern emergency_type_t *etype_list;
extern int               n_etypes;
extern rescuer_dt_t     *dt_list;
extern int               dt_count;
extern int               env_width;
extern int               env_height;

static pthread_t sched_thr;
static pthread_t monitor_thr;
static volatile sig_atomic_t sched_stop = 0;
static bqueue_t *q_global = NULL;

#define MAX_PAUSED 128
static emergency_request_t paused_q[MAX_PAUSED];
static int paused_count = 0;
static pthread_mutex_t paused_mtx = PTHREAD_MUTEX_INITIALIZER;

#define MAX_EMERGENCIES 256
static emergency_t emergency_table[MAX_EMERGENCIES];
static int emergency_count = 0;
static pthread_mutex_t emergency_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Dopo questo intervallo una emergenza in pausa viene incrementata di priorità */
#define DEADLOCK_TIMEOUT 30

void scheduler_check_deadlocks(void)
{
    long now = time(NULL);
    pthread_mutex_lock(&paused_mtx);
    for (int i = 0; i < paused_count; i++) {
        if (now - paused_q[i].timestamp > DEADLOCK_TIMEOUT &&
            paused_q[i].priority < 2) {
            paused_q[i].priority++;
            paused_q[i].timestamp = now;
            log_event_ex("SCH", "DEADLOCK_ESCALATE",
                        "emergency %d priority %d",
                        paused_q[i].id, paused_q[i].priority);
        }
    }
    pthread_mutex_unlock(&paused_mtx);
}

static void *monitor_loop(void *arg)
{
    (void)arg;
    while (!sched_stop) {
        scheduler_check_deadlocks();
        struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void scheduler_debug_add_paused(emergency_request_t req)
{
    pthread_mutex_lock(&paused_mtx);
    if (paused_count < MAX_PAUSED)
        paused_q[paused_count++] = req;
    pthread_mutex_unlock(&paused_mtx);
}

int scheduler_debug_get_paused_count(void)
{
    pthread_mutex_lock(&paused_mtx);
    int c = paused_count;
    pthread_mutex_unlock(&paused_mtx);
    return c;
}

emergency_request_t scheduler_debug_get_paused(int idx)
{
    emergency_request_t r = {0};
    pthread_mutex_lock(&paused_mtx);
    if (idx >= 0 && idx < paused_count)
        r = paused_q[idx];
    pthread_mutex_unlock(&paused_mtx);
    return r;
}

static emergency_t *find_emergency(int id)
{
    for (int i = 0; i < emergency_count; i++)
        if (emergency_table[i].id == id)
            return &emergency_table[i];
    return NULL;
}

void scheduler_add_emergency(const emergency_request_t *req)
{
    pthread_mutex_lock(&emergency_mtx);
    emergency_t *e = find_emergency(req->id);
    if (!e && emergency_count < MAX_EMERGENCIES) {
        e = &emergency_table[emergency_count++];
        e->id = req->id;
        snprintf(e->type, sizeof(e->type), "%s", req->type);
        e->type[sizeof(e->type) - 1] = '\0';
        e->creation_time = req->timestamp;
    }
    if (e) {
        e->x = req->x;
        e->y = req->y;
        e->priority = req->priority;
        e->status = EM_STATUS_WAITING;
    }
    pthread_mutex_unlock(&emergency_mtx);
}

int scheduler_set_emergency_status(int id, emergency_status_t st)
{
    int changed = 0;
    pthread_mutex_lock(&emergency_mtx);
    emergency_t *e = find_emergency(id);
    if (e) {
        if (e->status != st) {
            e->status = st;
            changed = 1;
        }
    }
    pthread_mutex_unlock(&emergency_mtx);
    return changed;
}

int scheduler_debug_get_emergency_count(void)
{
    pthread_mutex_lock(&emergency_mtx);
    int c = emergency_count;
    pthread_mutex_unlock(&emergency_mtx);
    return c;
}

emergency_t scheduler_debug_get_emergency(int idx)
{
    emergency_t e = {0};
    pthread_mutex_lock(&emergency_mtx);
    if (idx >= 0 && idx < emergency_count)
        e = emergency_table[idx];
    pthread_mutex_unlock(&emergency_mtx);
    return e;
}

// Ciclo dello scheduler: consuma le emergenze e le assegna
static void *scheduler_loop(void *arg) {
    bqueue_t *q = (bqueue_t *)arg;

    while (!sched_stop) {
        emergency_request_t req;
        bool from_paused = false;
        pthread_mutex_lock(&paused_mtx);
        if (paused_count > 0) {
            req = paused_q[--paused_count];
            from_paused = true;
        }
        pthread_mutex_unlock(&paused_mtx);
        if (!from_paused) {
            req = bqueue_pop(q);
        }

        long now = time(NULL);
        if (req.x < 0 || req.x >= env_width ||
            req.y < 0 || req.y >= env_height) {
            log_event_ex("SCH", "INVALID_REQUEST",
                        "Coordinate non valide per la richiesta %d, scartata",
                        req.id);
            continue;
        }
        if (req.timestamp <= 0 || req.timestamp > now) {
            log_event_ex("SCH", "INVALID_REQUEST",
                        "Timestamp non valido per la richiesta %d, scartata",
                        req.id);
            continue;
        }

        scheduler_add_emergency(&req);
        scheduler_set_emergency_status(req.id, EM_STATUS_WAITING);
        log_event_ex("SCH", "EMERGENCY_STATUS",
                    "id=%d status=WAITING", req.id);

        // 1) Ricerca dei metadati del tipo di emergenza
        emergency_type_t *et = NULL;
        for (int i = 0; i < n_etypes; i++) {
            if (strcmp(etype_list[i].name, req.type) == 0) {
                et = &etype_list[i];
                break;
            }
        }
        if (!et) {
            log_event_ex("SCH", "UNKNOWN_EMERGENCY_TYPE",
                        "'%s'", req.type);
            continue;
        }

        // 2) Verifica disponibilità e scadenze
        bool can_assign = true;
        double t_manage = (double)et->time_to_manage;

        int eff_priority = req.priority;
        if (req.priority == 0 && now - req.timestamp > AGING_THRESHOLD) {
            eff_priority = 1;
            log_event_ex("SCH", "AGING",
                        "emergency %d priority 0→1", req.id);
        }

        int deadline   = priority_deadline_secs(eff_priority);
        double max_travel = 0.0;

        rescuer_dt_t *used[10];
        int preempt_flags[10];

        for (int j = 0; j < et->n_required; j++) {
            int ridx = et->required_units[j];
            rescuer_type_t *r = &rescuer_list[ridx];

            rescuer_dt_t *dt = digital_twin_find_idle(dt_list, dt_count, r);
            preempt_flags[j] = 0;
            if (!dt) {
                dt = digital_twin_find_preemptible(dt_list, dt_count, r, eff_priority);
                if (dt) {
                    preempt_flags[j] = 1;
                } else {
                    can_assign = false;
                    break;
                }
            }
            used[j] = dt;

            double t_travel = travel_time_secs(r, dt->x, dt->y, req.x, req.y);
            if (t_travel > max_travel) max_travel = t_travel;
            if (t_travel + t_manage > deadline) {
                can_assign = false;
                break;
            }
        }

        // 3) Assegna oppure va in timeout
        if (can_assign) {
            scheduler_set_emergency_status(req.id, EM_STATUS_ASSIGNED);
            log_event_ex("SCH", "EMERGENCY_STATUS",
                        "id=%d status=ASSIGNED", req.id);
            for (int j = 0; j < et->n_required; j++) {
                rescuer_dt_t *dt = used[j];
                rescuer_type_t *r = dt->type;
                if (preempt_flags[j]) {
                    emergency_request_t old_req;
                    digital_twin_preempt(dt, &req, et->time_to_manage, &old_req);
                    log_event_ex("SCH", "PREEMPT",
                                "%s#%d emergency %d→%d", r->name, dt->id,
                                old_req.id, req.id);
                    log_event_ex("SCH", "EMERGENCY_STATUS",
                                "id=%d status=PAUSED", old_req.id);
                    pthread_mutex_lock(&paused_mtx);
                    if (paused_count < MAX_PAUSED) paused_q[paused_count++] = old_req;
                    pthread_mutex_unlock(&paused_mtx);
                } else {
                    digital_twin_assign(dt, req.id, req.type, eff_priority, req.x, req.y, et->time_to_manage);
                }
                log_event_ex("SCH", "ASSIGN",
                            "emergenza %d → %s#%d", req.id, r->name, dt->id);
            }

        } else {
            if (from_paused) {
                pthread_mutex_lock(&paused_mtx);
                if (paused_count < MAX_PAUSED) paused_q[paused_count++] = req;
                pthread_mutex_unlock(&paused_mtx);
            } else {
                double needed = max_travel + t_manage;
                log_event_ex("SCH", "TIMEOUT",
                            "emergenza %d (need=%.1f > d=%d)",
                            req.id, needed, deadline);
                scheduler_set_emergency_status(req.id, EM_STATUS_TIMEOUT);
                log_event_ex("SCH", "EMERGENCY_STATUS",
                            "id=%d status=TIMEOUT", req.id);
            }
        }
    }
    return NULL;
}

int scheduler_start(bqueue_t *q) {
    sched_stop = 0;
    q_global   = q;
    int rc = pthread_create(&sched_thr, NULL, scheduler_loop, q);
    if (rc != 0) return rc;
    return pthread_create(&monitor_thr, NULL, monitor_loop, NULL);
}

void scheduler_stop(void) {
    sched_stop = 1;
    pthread_mutex_lock(&q_global->mtx);
    pthread_cond_signal(&q_global->not_empty);
    pthread_mutex_unlock(&q_global->mtx);
    pthread_join(sched_thr, NULL);
    pthread_join(monitor_thr, NULL);
}

