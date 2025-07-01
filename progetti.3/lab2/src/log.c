#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static FILE *fp = NULL;

int log_init(const char *path)
{
    fp = fopen(path, "a");
    return fp ? 0 : -1;
}

void log_close(void)
{
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
}

static const char *evstr[] = {
    "FILE_PARSING",
    "MESSAGE_QUEUE",
    "EMERGENCY_STATUS",
    "RESCUER_STATUS"
};

void log_event(long id, event_t ev, const char *fmt, ...)
{
    if (!fp) return;                      /* logger non inizializzato */

    long ts = (long)time(NULL);

    pthread_mutex_lock(&mtx);

    fprintf(fp, "[%ld] ", ts);
    if (id >= 0)
        fprintf(fp, "[%ld] ", id);
    else
        fprintf(fp, "[N/A] ");
    fprintf(fp, "[%s] ", evstr[ev]);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fputc('\n', fp);
    fflush(fp);          /* flush immediato: semplice e sicuro */

    pthread_mutex_unlock(&mtx);
}
