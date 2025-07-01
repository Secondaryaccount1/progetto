#ifndef LOG_H
#define LOG_H

/*  log_init() va chiamata una sola volta all’avvio del server.
 *  Ritorna 0 se il file è stato aperto, −1 su errore.           */
int  log_init(const char *filepath);

/*  log_event() funziona come printf(): accetta una stringa di
 *  formato + argomenti variabili. Aggiunge data-ora e newline.  */
void log_event(const char *fmt, ...);

/*  Chiudere il file prima di terminare il processo.             */
void log_close(void);

#endif /* LOG_H */
