#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse_env.h"

/* Rimuove spazi a inizio/fine riga */
static void trim(char *s) {
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    memmove(s, p, strlen(p)+1);
    size_t len = strlen(s);
    if (len == 0) return;
    for (char *e = s + len - 1; e >= s && isspace((unsigned char)*e); --e)
        *e = '\0';
}

int parse_env_file(const char *path, env_config_t *cfg) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    /* valori di default */
    strncpy(cfg->queue_name, "/emergenze123", sizeof cfg->queue_name);
    cfg->width = 0;
    cfg->height = 0;

    char line[128];
    while (fgets(line, sizeof line, fp)) {
        trim(line);
        if (line[0]=='\0' || line[0]=='#') continue;
        char key[64], val[64];
        if (sscanf(line, "%63[^=]=%63s", key, val) == 2) {
            trim(key); trim(val);
            if (strcmp(key, "queue")==0) {
                strncpy(cfg->queue_name, val, sizeof cfg->queue_name);
            } else if (strcmp(key, "height")==0) {
                cfg->height = atoi(val);
            } else if (strcmp(key, "width")==0) {
                cfg->width = atoi(val);
            }
        }
    }
    fclose(fp);
    return 0;
}

