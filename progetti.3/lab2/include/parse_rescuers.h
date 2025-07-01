#ifndef PARSE_RESCUERS_H
#define PARSE_RESCUERS_H

#include "models.h"

/**
 * Legge il file `path`, riga per riga, formati del tipo:
 *   nome speed number x y
 * dove:
 *   nome   → stringa (es. "Vigili"), 
 *   speed  → intero,
 *   number → intero (# unità),
 *   x,y    → coordinate iniziali.
 * Restituisce un array allocato con malloc() in *out, e ne
 * mette la dimensione in *n_out. Ritorna 0 su sucesso, -1 su errore.
 */
int parse_rescuers_file(const char *path,
                        rescuer_type_t **out, int *n_out);

#endif /* PARSE_RESCUERS_H */
