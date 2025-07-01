#ifndef LOG_H
#define LOG_H

#include "models.h" /* per event_t */

/*  log_init() va chiamata una sola volta all’avvio del server.
 *  Il percorso fornito deve avere già la directory di destinazione
 *  già esistente (la funzione non la crea). Ritorna 0 se il file è
 *  stato aperto, −1 su errore.                                     */
int  log_init(const char *filepath);

/*  log_event() funziona come printf(): accetta una stringa di
 *  formato + argomenti variabili. Aggiunge data-ora e newline.  */
void log_event(long id, event_t ev, const char *fmt, ...);

/*  Chiudere il file prima di terminare il processo.             */
void log_close(void);

#endif /* LOG_H */
