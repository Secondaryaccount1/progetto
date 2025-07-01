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

void log_event(const char *fmt, ...)
{
    if (!fp) return;                      /* logger non inizializzato */

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    char ts[24];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);

    pthread_mutex_lock(&mtx);

    fprintf(fp, "[%s] ", ts);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fputc('\n', fp);
    fflush(fp);          /* flush immediato: semplice e sicuro */

    pthread_mutex_unlock(&mtx);
}
