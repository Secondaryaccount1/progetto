#ifndef MQ_MANAGER_H
#define MQ_MANAGER_H

#include <signal.h>
#include "queue.h"

/* Parametri passati al thread listener */
typedef struct {
    const char   *qname;     /* nome della coda POSIX    */
    bqueue_t     *queue;     /* coda interna condivisa   */
    long          msg_size;  /* dimensione max messaggio */
} mq_args_t;

/* Handler per SIGINT (Ctrl-C) */
void sigint_handler(int signo);

/* Thread di ascolto sulla POSIX MQ (producer) */
void *mq_listener(void *arg);

#endif /* MQ_MANAGER_H */

