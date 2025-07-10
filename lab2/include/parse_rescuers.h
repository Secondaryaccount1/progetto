#ifndef PARSE_RESCUERS_H
#define PARSE_RESCUERS_H

#include "models.h"

/**
 * Legge il file `path` nel formato:
 *   [nome][numero][speed][x;y]
 * Restituisce un array allocato con `malloc()` in `*out` e
 * scrive la lunghezza in `*n_out`. Ritorna 0 su successo,
 * -1 in caso di errore.
 */
int parse_rescuers_file(const char *path,
                        rescuer_type_t **out, int *n_out);

#endif /* PARSE_RESCUERS_H */
