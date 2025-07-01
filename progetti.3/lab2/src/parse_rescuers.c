#include "parse_rescuers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_rescuers_file(const char *path,
                        rescuer_type_t **out, int *n_out)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    size_t cap = 8;
    size_t cnt = 0;
    rescuer_type_t *arr = malloc(sizeof *arr * cap);
    if (!arr) { fclose(fp); return -1; }

    char line[128];
    char name[32];
    int number, speed, x, y;
    while (fgets(line, sizeof line, fp)) {
        if (sscanf(line, "[%31[^]]][%d][%d][%d;%d]",
                   name, &number, &speed, &x, &y) != 5)
            continue;
        if (cnt == cap) {
            cap *= 2;
            rescuer_type_t *tmp =
                realloc(arr, sizeof *arr * cap);
            if (!tmp) { free(arr); fclose(fp); return -1; }
            arr = tmp;
        }
        strncpy(arr[cnt].name, name, sizeof arr[cnt].name);
        arr[cnt].speed  = speed;
        arr[cnt].number = number;
        arr[cnt].x      = x;
        arr[cnt].y      = y;
        cnt++;
    }

    fclose(fp);
    *out   = arr;
    *n_out = (int)cnt;
    return 0;
}

