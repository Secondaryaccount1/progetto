// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#include "models.h"
#include "parse_env.h"
#include "parse_rescuers.h"
#include "parse_emergency_types.h"
#include "mq_manager.h"
#include "log.h"
#include "queue.h"
#include "scheduler.h"
#include "digital_twin.h"

/* Rendiamo rescuer_list e etype_list globali, visibili allo scheduler */
rescuer_type_t   *rescuer_list = NULL;
int               n_rescuers   = 0;
emergency_type_t *etype_list   = NULL;
int               n_etypes     = 0;
rescuer_dt_t     *dt_list      = NULL;
int               dt_count     = 0;

extern void sigint_handler(int);

int main(void) {
    /* 1) Config */
    env_config_t cfg;
    if (parse_env_file("conf/env.conf", &cfg) != 0) {
        fprintf(stderr,
                "Warning: conf/env.conf non trovato, uso valori di default\n");
        strcpy(cfg.queue_name, "/emergenze123");
        cfg.width = 0;
        cfg.height = 0;
    }
    printf("[SERVER] Queue: %s  (grid %dx%d)\n",
           cfg.queue_name, cfg.width, cfg.height);

    /* 2) Parser rescuers & emergency types */
    if (parse_rescuers_file("conf/rescuers.conf",
                            &rescuer_list, &n_rescuers) != 0) {
        fprintf(stderr, "Errore parsing conf/rescuers.conf\n");
        return 1;
    }
    if (parse_emergency_types_file("conf/emergency_types.conf",
                                   rescuer_list, n_rescuers,
                                   &etype_list, &n_etypes) != 0) {
        fprintf(stderr, "Errore parsing conf/emergency_types.conf\n");
        return 1;
    }

    /* 3) Logger */
    if (mkdir("logs", 0755) == -1 && errno != EEXIST) {
        perror("mkdir logs");
        return 1;
    }
    if (log_init("logs/server.log") != 0) {
        perror("log_init");
        return 1;
    }
    log_event("Loaded %d rescuers, %d emergency types",
              n_rescuers, n_etypes);

    if (digital_twin_factory(rescuer_list, n_rescuers, &dt_list, &dt_count) != 0) {
        log_event("Errore creazione digital twins");
        return 1;
    }

    /* 4) Queue interna + scheduler */
    bqueue_t queue;
    if (bqueue_init(&queue, 100) != 0) {
        log_event("Errore init queue");
        return 1;
    }
    if (scheduler_start(&queue) != 0) {
        log_event("Errore avvio scheduler");
        return 1;
    }

    /* 5) Listener POSIX MQ */
    signal(SIGINT, sigint_handler);
    pthread_t listener_thread;
    mq_args_t args = {
        .qname    = cfg.queue_name,
        .queue    = &queue,
        .msg_size = sizeof(emergency_request_t)
    };
    if (pthread_create(&listener_thread, NULL,
                       mq_listener, &args) != 0) {
        log_event("pthread_create listener fallita");
        scheduler_stop();
        bqueue_destroy(&queue);
        return 1;
    }
    log_event("Server avviato e in ascolto su %s", cfg.queue_name);

    /* 6) Attende CTRL-C */
    pthread_join(listener_thread, NULL);
    scheduler_stop();
    digital_twin_shutdown(dt_list, dt_count);
    bqueue_destroy(&queue);
    log_close();

    /* 7) Cleanup parser arrays */
    free(rescuer_list);
    free(etype_list);
    return 0;
}

