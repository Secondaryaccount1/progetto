#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>
#include "models.h"

#define CHECK(x)  do { if ((x) == -1) { perror(#x); exit(1);} } while (0)

static int send_request(mqd_t mq, emergency_request_t *req)
{
    return mq_send(mq, (char *)req, sizeof(*req), 0);
}

int main(int argc, char *argv[]) {
    if (argc == 7) {
        /* invio di una singola emergenza */
        const char *qname = argv[1];
        emergency_request_t req = {
            .id       = atoi(argv[2]),
            .priority = atoi(argv[4]),
            .x        = atoi(argv[5]),
            .y        = atoi(argv[6]),
            .timestamp = time(NULL)
        };
        strncpy(req.type, argv[3], sizeof(req.type)-1);
        req.type[sizeof(req.type)-1] = '\0';

        mqd_t mq = mq_open(qname, O_WRONLY | O_CREAT, 0660, NULL);
        CHECK(mq);
        CHECK(send_request(mq, &req));
        mq_close(mq);
        puts("Richiesta inviata!");
        return 0;

    } else if (argc == 4 && strcmp(argv[2], "-f") == 0) {
        /* invio di emergenze da file */
        const char *qname = argv[1];
        const char *fname = argv[3];
        FILE *fp = fopen(fname, "r");
        if (!fp) {
            perror(fname);
            return 1;
        }

        mqd_t mq = mq_open(qname, O_WRONLY | O_CREAT, 0660, NULL);
        if (mq == (mqd_t)-1) {
            perror("mq_open");
            fclose(fp);
            return 1;
        }

        char line[128];
        while (fgets(line, sizeof line, fp)) {
            emergency_request_t req;
            char type[32];
            if (sscanf(line, "%d %31s %d %d %d",
                       &req.id, type, &req.priority,
                       &req.x, &req.y) == 5) {
                strncpy(req.type, type, sizeof req.type - 1);
                req.type[sizeof req.type - 1] = '\0';
                req.timestamp = time(NULL);
                if (send_request(mq, &req) == -1)
                    perror("mq_send");
            }
        }

        fclose(fp);
        mq_close(mq);
        return 0;

    } else {
        fprintf(stderr,
                "Uso: %s <queue> <id> <type> <prio> <x> <y>\n"
                "   oppure: %s <queue> -f <file>\n",
                argv[0], argv[0]);
        return 1;
    }
}
