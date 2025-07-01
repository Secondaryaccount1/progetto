// src/mq_manager.c
#include "mq_manager.h"
#include "models.h"
#include "log.h"

#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/* flag globale per fermare il loop su SIGINT */
static volatile sig_atomic_t stop = 0;

/* ────────────────────────────────────────────── */
/* Handler Ctrl-C                                */
/* ────────────────────────────────────────────── */
void sigint_handler(int signo)
{
    (void)signo;
    stop = 1;
}

/* ────────────────────────────────────────────── */
/* Thread listener: POSIX MQ → coda interna      */
/* ────────────────────────────────────────────── */
void *mq_listener(void *arg)
{
    mq_args_t *a   = (mq_args_t *)arg;
    const char *qn = a->qname;
    bqueue_t   *q  = a->queue;

    /* Attributi della coda */
    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = 10,
        .mq_msgsize = a->msg_size,
        .mq_curmsgs = 0
    };

    /* Ripulisci eventuale run precedente */
    mq_unlink(qn);

    mqd_t mq = mq_open(qn, O_CREAT | O_RDONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        log_event(-1, MESSAGE_QUEUE,
                  "mq_open(%s) failed: %s", qn, strerror(errno));
        return NULL;
    }
    log_event(-1, MESSAGE_QUEUE,
              "Listener attivo sulla coda %s", qn);

    /* Buffer grande abbastanza per qualunque messaggio */
    char *buf = malloc(attr.mq_msgsize);
    if (!buf) {
        log_event(-1, MESSAGE_QUEUE,
                  "malloc(%ld) failed", (long)attr.mq_msgsize);
        mq_close(mq); mq_unlink(qn);
        return NULL;
    }

    while (!stop) {
        ssize_t n = mq_receive(mq, buf, attr.mq_msgsize, NULL);

        if (n == sizeof(emergency_request_t)) {
            /* Copia sicura dentro la struct */
            emergency_request_t req;
            memcpy(&req, buf, sizeof req);

            log_event(req.id, MESSAGE_QUEUE,
                      "Queued emergenza (%s) prio=%d @(%d,%d)",
                      req.type, req.priority, req.x, req.y);

            bqueue_push(q, req);         /* producer → coda interna */
        }
        else if (n < 0 && errno != EINTR) {
            /* Logga una sola volta ciascun codice d’errore */
            static int warned_emsgsize = 0;
            if (errno == EMSGSIZE && !warned_emsgsize) {
                log_event(-1, MESSAGE_QUEUE,
                          "mq_receive error EMSGSIZE (msg troppo grande)");
                warned_emsgsize = 1;
            } else if (errno != EMSGSIZE) {
                log_event(-1, MESSAGE_QUEUE,
                          "mq_receive error: %s", strerror(errno));
            }
        }
    }

    free(buf);
    mq_close(mq);
    mq_unlink(qn);
    log_event(-1, MESSAGE_QUEUE,
              "Listener terminato, coda %s chiusa", qn);
    return NULL;
}

