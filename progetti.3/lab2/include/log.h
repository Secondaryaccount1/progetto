#ifndef LOG_H
#define LOG_H

/*  log_init() va chiamata una sola volta all’avvio del server.
 *  Il percorso fornito deve avere già la directory di destinazione
 *  già esistente (la funzione non la crea). Ritorna 0 se il file è
 *  stato aperto, −1 su errore.                                     */
int  log_init(const char *filepath);

/*  log_event() funziona come printf(): accetta una stringa di
 *  formato + argomenti variabili. Aggiunge data-ora e newline.  */
void log_event(const char *fmt, ...);

/*  Versione estesa: consente di specificare un identificatore e una
 *  categoria dell'evento da registrare. L'output sarà del tipo:
 *  [timestamp] [id] [event] messaggio                                 */
void log_event_ex(const char *id, const char *event, const char *fmt, ...);

/*  Chiudere il file prima di terminare il processo.             */
void log_close(void);

#endif /* LOG_H */
