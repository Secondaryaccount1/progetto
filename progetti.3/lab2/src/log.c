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

static void log_event_v(const char *id, const char *event,
                        const char *fmt, va_list ap)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    char ts[24];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);

    pthread_mutex_lock(&mtx);

    fprintf(fp, "[%s] [%s] [%s] ", ts,
            id ? id : "", event ? event : "");

    vfprintf(fp, fmt, ap);

    fputc('\n', fp);
    fflush(fp);          /* flush immediato: semplice e sicuro */

    pthread_mutex_unlock(&mtx);
}

void log_event_ex(const char *id, const char *event, const char *fmt, ...)
{
    if (!fp) return;                      /* logger non inizializzato */

    va_list ap;
    va_start(ap, fmt);
    log_event_v(id, event, fmt, ap);
    va_end(ap);
}

void log_event(const char *fmt, ...)
{
    if (!fp) return;                      /* logger non inizializzato */

    va_list ap;
    va_start(ap, fmt);
    log_event_v("", "", fmt, ap);
    va_end(ap);
}
