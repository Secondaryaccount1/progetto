#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>
#include "models.h"

#define CHECK(x)  do { if ((x) == -1) { perror(#x); exit(1);} } while (0)

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Uso: %s <queue> <id> <type> <prio> <x> <y>\n", argv[0]);
        return 1;
    }

    const char *qname = argv[1];
    emergency_request_t req = {
        .id       = atoi(argv[2]),
        .priority = atoi(argv[4]),
        .x        = atoi(argv[5]),
        .y        = atoi(argv[6]),
        .timestamp = time(NULL)
    };
    strncpy(req.type, argv[3], sizeof(req.type)-1);

    mqd_t mq = mq_open(qname, O_WRONLY | O_CREAT, 0660, NULL);
    CHECK(mq);
    CHECK(mq_send(mq, (char *)&req, sizeof(req), 0));
    mq_close(mq);
    puts("Richiesta inviata!");
    return 0;
}
