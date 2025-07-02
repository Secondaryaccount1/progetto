#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "scheduler.h"
#include "digital_twin.h"
#include "queue.h"

/* Stubs required by scheduler.c but unused in this test */
rescuer_type_t   *rescuer_list = NULL;
int               n_rescuers   = 0;
emergency_type_t *etype_list   = NULL;
int               n_etypes     = 0;
rescuer_dt_t     *dt_list      = NULL;
int               dt_count     = 0;
int               env_width    = 0;
int               env_height   = 0;

rescuer_dt_t *digital_twin_find_idle(rescuer_dt_t *l,int n,rescuer_type_t *t){return NULL;}
rescuer_dt_t *digital_twin_find_preemptible(rescuer_dt_t *l,int n,rescuer_type_t *t,int p){return NULL;}
int digital_twin_assign(rescuer_dt_t *dt,int id,const char *type,int pr,int x,int y,int tman){return 0;}
int digital_twin_preempt(rescuer_dt_t *dt,const emergency_request_t *r,int m,emergency_request_t *o){return 0;}
emergency_request_t bqueue_pop(bqueue_t *q){emergency_request_t r={0};return r;}

int main(void) {
    emergency_request_t r;
    r.id = 1;
    r.priority = 1;
    r.timestamp = time(NULL) - 100; /* older than timeout */

    scheduler_debug_add_paused(r);

    scheduler_check_deadlocks();

    assert(scheduler_debug_get_paused_count() == 1);
    emergency_request_t out = scheduler_debug_get_paused(0);
    assert(out.priority == 2);

    printf("[test_deadlock] all tests passed\n");
    return 0;
}
