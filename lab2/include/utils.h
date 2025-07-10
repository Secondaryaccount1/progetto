// file include/utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <limits.h>
#include "models.h"  /* per rescuer_type_t ed emergency_type_t */

/* -------------------------------------- */
/* Calcola la distanza di Manhattan       */
/* -------------------------------------- */
static inline int manhattan_distance(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

/* -------------------------------------- */
/* Tempo di viaggio in secondi = d / v    */
/* dove v = r->speed                     */
/* -------------------------------------- */
static inline double travel_time_secs(const rescuer_type_t *r,
                                      int from_x, int from_y,
                                      int to_x,   int to_y)
{
    return (double)manhattan_distance(from_x, from_y, to_x, to_y)
         / r->speed;
}

/* -------------------------------------- */
/* Deadline in secondi in base alla priorità */
/*  priority 2 → 10s, 1 → 30s, 0 → ∞      */
/* -------------------------------------- */
static inline int priority_deadline_secs(short priority)
{
    switch (priority) {
        case 2:  return 10;
        case 1:  return 30;
        default: return INT_MAX;
    }
}

#endif /* UTILS_H */
