#ifndef PARSE_ENV_H
#define PARSE_ENV_H

#include "models.h"

/**
 * Legge da `path` il file di configurazione nel formato:
 *   queue=<nome_coda>
 *   height=<numero>
 *   width=<numero>
 * Riempie la struct out (ritorna 0 se ok, -1 su errore).
 */
int parse_env_file(const char *path, env_config_t *out);

#endif /* PARSE_ENV_H */
