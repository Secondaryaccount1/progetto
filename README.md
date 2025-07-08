# Progetto Lab2

Questo repository contiene un sistema semplificato di gestione delle emergenze pensato per scopi didattici. Include i parser dei file di configurazione, un modulo di "digital twin" e uno scheduler che assegna i soccorritori alle richieste in arrivo.

## Rilevamento deadlock

Un thread di monitoraggio controlla periodicamente le emergenze messe in pausa. Se una richiesta rimane nella coda `paused` per più di 30 secondi viene automaticamente incrementata di priorità. Questo può forzare la preemption di assegnazioni a priorità più bassa ed evita situazioni di deadlock o starvation.

I test unitari coprono la logica di escalation tramite `test-deadlock`.

## Configurazione del server

Il server legge tre file di configurazione:

* `env.conf` per i parametri della coda e le dimensioni della griglia
* `rescuers.conf` con l'elenco dei soccorritori disponibili
* `emergency_types.conf` che definisce le tipologie di emergenza

Di default i file sono cercati nella directory `conf`. Percorsi alternativi possono essere forniti a riga di comando all'avvio del server:

```sh
./bin/server -r miei_rescuers.conf -e mie_emergencies.conf -n mio_env.conf
```

L'opzione `-n` permette di indicare un altro `env.conf`. Modificando la voce `queue=` si cambia il nome della coda POSIX usata dal server (di default `/emergenze123`).

I log di esecuzione sono salvati in `logs/server.log`.

## Build e test

Il `Makefile` in `progetti.3/lab2` fornisce alcuni target utili:

```sh
make -C progetti.3/lab2 server          # compila il server
make -C progetti.3/lab2 client          # compila il client
make -C progetti.3/lab2 test-utils      # esegue i test delle utility
make -C progetti.3/lab2 test-parsers    # esegue i test dei parser
make -C progetti.3/lab2 test-parse-env  # esegue i test del parser env
make -C progetti.3/lab2 test-deadlock   # esegue i test del monitor deadlock
make -C progetti.3/lab2 test-scheduler  # esegue il test di integrazione dello scheduler
```

Il target predefinito `make build` compila il server, mentre `make run` lo avvia con i file di configurazione standard.
