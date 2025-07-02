// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

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
int               env_width    = 0;
int               env_height   = 0;

extern void sigint_handler(int);

int main(int argc, char *argv[]) {
    const char *resc_path = "conf/rescuers.conf";
    const char *etype_path = "conf/emergency_types.conf";
    const char *env_path = "conf/env.conf";

    int opt;
    while ((opt = getopt(argc, argv, "r:e:n:")) != -1) {
        switch (opt) {
            case 'r': resc_path = optarg; break;
            case 'e': etype_path = optarg; break;
            case 'n': env_path = optarg; break;
            default:
                fprintf(stderr,
                        "Usage: %s [-r resc.conf] [-e etypes.conf] [-n env.conf]\n",
                        argv[0]);
                return 1;
        }
    }

    /* 1) Config */
    env_config_t cfg;
    if (parse_env_file(env_path, &cfg) != 0) {
        fprintf(stderr,
                "Warning: %s non trovato, uso valori di default\n",
                env_path);
        strcpy(cfg.queue_name, "/emergenze123");
        cfg.width = 0;
        cfg.height = 0;
    }
    env_width  = cfg.width;
    env_height = cfg.height;
    printf("[SERVER] Queue: %s  (grid %dx%d)\n",
           cfg.queue_name, cfg.width, cfg.height);

    /* 2) Parser rescuers & emergency types */
    if (parse_rescuers_file(resc_path,
                            &rescuer_list, &n_rescuers) != 0) {
        fprintf(stderr, "Errore parsing %s\n", resc_path);
        return 1;
    }
    if (parse_emergency_types_file(etype_path,
                                   rescuer_list, n_rescuers,
                                   &etype_list, &n_etypes) != 0) {
        fprintf(stderr, "Errore parsing %s\n", etype_path);
        return 1;
    }

    /* 3) Logger */
    int rc = mkdir("logs", 0755);
    if (rc == -1 && errno != EEXIST)
        SYS_CHECK(rc);
    SYS_CHECK(log_init("logs/server.log"));
    log_event("Loaded %d rescuers, %d emergency types",
              n_rescuers, n_etypes);

    if (digital_twin_factory(rescuer_list, n_rescuers,
                             &dt_list, &dt_count) != 0) {
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
    PTH_CHECK(pthread_create(&listener_thread, NULL,
                             mq_listener, &args));
    log_event("Server avviato e in ascolto su %s", cfg.queue_name);

    /* 6) Attende CTRL-C */
    PTH_CHECK(pthread_join(listener_thread, NULL));
    scheduler_stop();
    digital_twin_shutdown(dt_list, dt_count);
    bqueue_destroy(&queue);
    log_close();

    /* 7) Cleanup parser arrays */
    free(rescuer_list);
    free(etype_list);
    return 0;
}

