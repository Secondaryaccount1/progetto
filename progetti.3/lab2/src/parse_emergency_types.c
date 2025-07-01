#include "parse_emergency_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_emergency_types_file(const char *path,
                               emergency_type_t **out, int *n_out)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    size_t cap = 8;
    size_t cnt = 0;
    emergency_type_t *arr = malloc(sizeof *arr * cap);
    if (!arr) { fclose(fp); return -1; }

    char name[32];
    int priority, time_to_manage, n_required;
    while (fscanf(fp, "%31s %d %d %d",
                  name, &priority, &time_to_manage,
                  &n_required) == 4)
    {
        if (cnt == cap) {
            cap *= 2;
            emergency_type_t *tmp =
                realloc(arr, sizeof *arr * cap);
            if (!tmp) { free(arr); fclose(fp); return -1; }
            arr = tmp;
        }
        /* copia base */
        strncpy(arr[cnt].name, name, sizeof arr[cnt].name);
        arr[cnt].priority        = priority;
        arr[cnt].time_to_manage  = time_to_manage;
        arr[cnt].n_required      = n_required;

        /* leggi i required_units */
        for (int i = 0; i < n_required; i++) {
            if (fscanf(fp, "%d", &arr[cnt].required_units[i]) != 1) {
                arr[cnt].required_units[i] = -1;
            }
        }
        cnt++;
        /* consuma eventuali newline/spazi */
        int c; while ((c = fgetc(fp)) != EOF && c!='\n');
    }

    fclose(fp);
    *out   = arr;
    *n_out = (int)cnt;
    return 0;
}

