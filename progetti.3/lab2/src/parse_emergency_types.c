#include "parse_emergency_types.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int rescuer_index_by_name(const char *name,
                                 rescuer_type_t *list, int n)
{
    for (int i=0;i<n;i++)
        if (strcmp(list[i].name, name)==0)
            return i;
    return -1;
}

int parse_emergency_types_file(const char *path,
                               rescuer_type_t *rlist, int n_rlist,
                               emergency_type_t **out, int *n_out)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    size_t cap = 8;
    size_t cnt = 0;
    emergency_type_t *arr = malloc(sizeof *arr * cap);
    if (!arr) { fclose(fp); return -1; }

    char line[256];
    char name[32];
    int priority;
    int line_no = 0;
    while (fgets(line, sizeof line, fp)) {
        line_no++;
        char rest[200];
        if (sscanf(line, "[%31[^]]] [%d] %199[^\n]", name, &priority, rest) != 3)
            continue;

        if (cnt == cap) {
            cap *= 2;
            emergency_type_t *tmp =
                realloc(arr, sizeof *arr * cap);
            if (!tmp) { free(arr); fclose(fp); return -1; }
            arr = tmp;
        }

        strncpy(arr[cnt].name, name, sizeof arr[cnt].name);
        arr[cnt].priority = priority;
        arr[cnt].n_required = 0;
        arr[cnt].time_to_manage = 0;

        char *p = rest;
        int valid_line = 1;
        while (*p) {
            char rname[32];
            int count, tman;
            if (sscanf(p, "%31[^:]:%d,%d;", rname, &count, &tman) != 3) {
                valid_line = 0;
                break;
            }
            int ridx = rescuer_index_by_name(rname, rlist, n_rlist);
            if (ridx < 0) {
                log_event_ex("PARSE", "FILE_PARSING",
                            "tipo di soccorritore sconosciuto '%s' alla riga %d",
                            rname, line_no);
                valid_line = 0;
                break;
            }
            for (int i=0;i<count && arr[cnt].n_required<10;i++)
                arr[cnt].required_units[arr[cnt].n_required++] = ridx;
            if (tman > arr[cnt].time_to_manage)
                arr[cnt].time_to_manage = tman;
            char *semi = strchr(p, ';');
            if (!semi) break;
            p = semi + 1;
        }

        if (valid_line)
            cnt++;
    }

    fclose(fp);
    *out   = arr;
    *n_out = (int)cnt;
    return 0;
}

