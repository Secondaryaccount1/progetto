#ifndef PARSE_EMERGENCY_TYPES_H
#define PARSE_EMERGENCY_TYPES_H

#include "models.h"

/**
 * Legge il file `path` nel formato BNF:
 *   [nome] [priorita] Rescuer:numero,tempo;...
 * `rlist` e `n_rlist` servono a mappare i nomi dei soccorritori
 * sugli indici utilizzati dal sistema.
 * Restituisce un array allocato con `malloc()` in `*out` e la
 * relativa lunghezza in `*n_out` (0 su successo, -1 su errore).
 */
int parse_emergency_types_file(const char *path,
                               rescuer_type_t *rlist, int n_rlist,
                               emergency_type_t **out, int *n_out);

#endif /* PARSE_EMERGENCY_TYPES_H */
