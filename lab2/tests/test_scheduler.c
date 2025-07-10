#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"
#include "digital_twin.h"
#include "queue.h"
#include <unistd.h>

/* Variabili globali richieste da scheduler.c */
rescuer_type_t   *rescuer_list = NULL;
int               n_rescuers   = 0;
emergency_type_t *etype_list   = NULL;
int               n_etypes     = 0;
rescuer_dt_t     *dt_list      = NULL;
int               dt_count     = 0;
int               env_width    = 0;
int               env_height   = 0;

/* Stub dati del digital twin */
static rescuer_dt_t dt_stub;
static int assign_count = 0;
static emergency_request_t last_assigned;

rescuer_dt_t *digital_twin_find_idle(rescuer_dt_t *l,int n,rescuer_type_t *t){
    if (n > 0 && l[0].assigned == 0 && l[0].type == t)
        return &l[0];
    return NULL;
}

rescuer_dt_t *digital_twin_find_preemptible(rescuer_dt_t *l,int n,rescuer_type_t *t,int p){
    return NULL; /* non utilizzato */
}

int digital_twin_assign(rescuer_dt_t *dt,int id,const char *type,int pr,int x,int y,int tman){
    dt->assigned = 1;
    assign_count++;
    last_assigned.id = id;
    strncpy(last_assigned.type, type, sizeof(last_assigned.type)-1);
    last_assigned.type[sizeof(last_assigned.type)-1] = '\0';
    last_assigned.priority = pr;
    last_assigned.x = x;
    last_assigned.y = y;
    return 0;
}

int digital_twin_preempt(rescuer_dt_t *dt,const emergency_request_t *r,int m,emergency_request_t *o){
    return 0;
}

int main(void){
    /* Inizializza un singolo soccorritore e tipo di emergenza */
    static rescuer_type_t rescuer;
    strcpy(rescuer.name, "Amb");
    rescuer.number = 1;
    rescuer.speed = 5;
    rescuer.x = 0; rescuer.y = 0;
    rescuer_list = &rescuer;
    n_rescuers = 1;

    static emergency_type_t etype;
    strcpy(etype.name, "Fire");
    etype.priority = 2;
    etype.required_units[0] = 0;
    etype.n_required = 1;
    etype.time_to_manage = 5;
    etype_list = &etype;
    n_etypes = 1;

    dt_stub.type = &rescuer;
    dt_stub.assigned = 0;
    dt_list = &dt_stub;
    dt_count = 1;

    env_width = 5;
    env_height = 5;

    bqueue_t q;
    assert(bqueue_init(&q, 10) == 0);
    assert(scheduler_start(&q) == 0);

    long now = time(NULL);

    /* 1) assegnazione valida */
    emergency_request_t r1 = {1, "Fire", 1,1, 2, now};
    bqueue_push(&q, r1);
    sleep(1);
    assert(assign_count == 1);
    assert(last_assigned.id == 1);
    assert(scheduler_debug_get_emergency_count() == 1);
    emergency_t e1 = scheduler_debug_get_emergency(0);
    assert(e1.status == EM_STATUS_ASSIGNED);

    /* 2) coordinate non valide */
    emergency_request_t r2 = {2, "Fire", -1,0, 2, now};
    bqueue_push(&q, r2);
    sleep(1);
    assert(assign_count == 1); /* invariato */
    assert(scheduler_debug_get_emergency_count() == 1);

    /* 3) timestamp non valido */
    emergency_request_t r3 = {3, "Fire", 2,2, 2, now + 3600};
    bqueue_push(&q, r3);
    sleep(1);
    assert(assign_count == 1);
    assert(scheduler_debug_get_emergency_count() == 1);

    /* 4) risorse insufficienti -> timeout */
    dt_stub.assigned = 1; /* nessun soccorritore libero */
    emergency_request_t r4 = {4, "Fire", 1,1, 2, now};
    bqueue_push(&q, r4);
    sleep(1);
    assert(scheduler_debug_get_emergency_count() == 2);
    emergency_t e2 = scheduler_debug_get_emergency(1);
    assert(e2.status == EM_STATUS_TIMEOUT);

    /* 5) aging priorità 0 -> 1 */
    dt_stub.assigned = 0; /* soccorritore libero */
    emergency_request_t r5 = {5, "Fire", 2,2, 0, now - 70};
    bqueue_push(&q, r5);
    sleep(1);
    assert(assign_count == 2);
    assert(last_assigned.id == 5);
    assert(last_assigned.priority == 1); /* incrementata */
    assert(scheduler_debug_get_emergency_count() == 3);
    emergency_t e3 = scheduler_debug_get_emergency(2);
    assert(e3.status == EM_STATUS_ASSIGNED);

    /* inserisce un evento fittizio per far terminare il thread scheduler */
    emergency_request_t done = {99, "Fire", 0,0, 2, now};
    bqueue_push(&q, done);

    scheduler_stop();
    bqueue_destroy(&q);

    printf("[test_scheduler] all tests passed\n");
    return 0;
}
