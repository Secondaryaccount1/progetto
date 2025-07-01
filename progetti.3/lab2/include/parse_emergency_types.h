#ifndef PARSE_EMERGENCY_TYPES_H
#define PARSE_EMERGENCY_TYPES_H

#include "models.h"

/**
 * Legge il file `path`, formati del tipo:
 *   nome priority time_to_manage n_required idx0 idx1 …
 * es.:
 *   Incendio 2 20 2 0 1
 * Restituisce un array allocato con malloc() in *out e la sua lunghezza in *n_out.
 * Ritorna 0 su successo, -1 su errore.
 */
int parse_emergency_types_file(const char *path,
                               emergency_type_t **out, int *n_out);

#endif /* PARSE_EMERGENCY_TYPES_H */
