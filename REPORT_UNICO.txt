# Documentazione Unificata del Progetto Lab2

Questo documento unisce i contenuti presenti in `README.md` e `REPORT.md`. Di seguito trovi una panoramica del progetto, le motivazioni progettuali, le istruzioni di compilazione e un riepilogo della struttura dei file.

## Introduzione

Il progetto implementa un sistema semplificato di gestione delle emergenze. Include parser dei file di configurazione, un modulo "digital twin" che simula i soccorritori e uno scheduler che assegna le risorse alle richieste in arrivo. La documentazione è organizzata nei capitoli seguenti:

1. Rilevamento deadlock
2. Configurazione del server
3. Build e test
4. Panoramica dell'architettura
5. Scelte progettuali
6. Istruzioni di build e uso
7. Struttura dei file
8. Conclusioni

---
## 1. Rilevamento deadlock

Un thread di monitoraggio controlla periodicamente le emergenze messe in pausa. Se una richiesta rimane nella coda `paused` per più di 30 secondi viene automaticamente incrementata di priorità. In questo modo si evita la starvation e si possono preemptare assegnazioni meno urgenti. I test unitari relativi a questa logica sono eseguibili con `make -C progetti.3/lab2 test-deadlock`.

## 2. Configurazione del server

Il server legge tre file di configurazione:

* `env.conf` per i parametri della coda e le dimensioni della griglia
* `rescuers.conf` con l'elenco dei soccorritori disponibili
* `emergency_types.conf` che definisce le tipologie di emergenza

Di default i file sono cercati nella directory `conf`. Percorsi alternativi possono essere forniti a riga di comando all'avvio del server:

```sh
./bin/server -r miei_rescuers.conf -e mie_emergencies.conf -n mio_env.conf
```

L'opzione `-n` permette di indicare un altro `env.conf`. Modificando la voce `queue=` si cambia il nome della coda POSIX usata dal server (di default `/emergenze123`). I log di esecuzione sono salvati in `logs/server.log`.

## 3. Build e test

Il `Makefile` in `progetti.3/lab2` fornisce i seguenti target principali:

```sh
make -C progetti.3/lab2 server          # compila il server
make -C progetti.3/lab2 client          # compila il client
make -C progetti.3/lab2 test-utils      # esegue i test delle utility
make -C progetti.3/lab2 test-parsers    # esegue i test dei parser
make -C progetti.3/lab2 test-parse-env  # esegue i test del parser env
make -C progetti.3/lab2 test-deadlock   # esegue i test del monitor deadlock
make -C progetti.3/lab2 test-scheduler  # esegue il test di integrazione dello scheduler
```

Il target predefinito è `build`, equivalente a `make -C progetti.3/lab2 server`. Una volta compilato il server è possibile avviarlo con `make run` oppure direttamente con il comando indicato sopra.

---
## 4. Panoramica dell'architettura

Il sistema è basato su un'architettura modulare con i seguenti componenti principali:

- **Listener della coda**: riceve i messaggi di emergenza da una coda POSIX.
- **Scheduler**: decide quale soccorritore assegnare a ogni emergenza.
- **Digital Twin dei soccorritori**: thread che simulano spostamenti e attività dei soccorritori reali.
- **Modulo di logging**: registra su file gli eventi chiave del sistema.
- **Parser di configurazione**: caricano le informazioni da `conf/*.conf`.
- **Code di gestione interna**: per lo scambio di richieste tra i thread.

I soccorritori sono rappresentati da thread "digital twin" che simulano l'avvicinamento al luogo dell'emergenza, l'intervento e il rientro alla base. In questo modo si può testare lo scheduler senza hardware reale.

### Componenti principali

1. **server.c** – Punto di ingresso del server. Carica i file di configurazione, avvia i thread di digital twin e lo scheduler e poi il listener sulla coda.
2. **scheduler.c** – Logica di assegnazione con monitor di deadlock che aumenta la priorità delle emergenze bloccate da troppo tempo.
3. **digital_twin.c** – Simulazione dei soccorritori.
4. **mq_manager.c** – Gestisce la coda POSIX.
5. **Parser** – `parse_rescuers.c`, `parse_emergency_types.c` e `parse_env.c` estraggono le configurazioni.
6. **Log** – Il modulo `log.c` produce `logs/server.log` con timestamp e descrizione degli eventi.

I test unitari sono nella cartella `tests/` e coprono la maggior parte dei moduli, inclusa l'interazione dello scheduler con le priorità e il monitor di deadlock.

## 5. Scelte progettuali

### Linguaggio e standard
Il codice è scritto in C (standard C11) con l'uso di `-Wall` e `-Wextra` per evidenziare il maggior numero di avvisi. L'opzione `-pthread` abilita i thread POSIX.

### Modularità e separazione delle responsabilità
Le funzionalità sono suddivise in moduli dedicati (`scheduler.c`, `digital_twin.c`, i vari parser, `log.c`). Ciò semplifica eventuali estensioni e correzioni.

### Sincronizzazione tra thread
Mutex, variabili di condizione e una coda circolare (`bqueue_t`) coordinano i thread. Un monitor secondario incrementa la priorità delle emergenze in pausa da oltre 30 secondi per evitare starvation.

### Gestione delle priorità
I tipi di emergenza definiscono una priorità base e le risorse richieste. Lo scheduler usa un modello a priorità con timeout per le emergenze che restano in attesa troppo a lungo.

### Logging e tracciabilità
Ogni azione importante genera una riga nel log. Ciò facilita il debug e la verifica delle funzionalità.

## 6. Istruzioni di build e uso

Per compilare e testare il sistema utilizzare i target del `Makefile` come descritto in precedenza. Dopo la compilazione si ottengono i binari eseguibili nella cartella `bin`:

```sh
./bin/server -r conf/rescuers.conf -e conf/emergency_types.conf -n conf/env.conf
```

È inoltre disponibile un client di esempio che può essere lanciato con `./bin/client conf/emergency_types.conf`.

### A cosa serve la cartella `bin/`

La directory `bin/` è creata automaticamente dal `Makefile` (variabile `BIN_DIR`). Durante la compilazione, ogni target invoca `mkdir -p bin` e produce al suo interno i file eseguibili (`server`, `client`, e i vari test). Non è quindi presente di default nel repository ma viene generata quando si esegue `make`.

## 7. Struttura dei file

- `src/` – codici sorgente in C
- `include/` – header con le definizioni delle strutture
- `conf/` – file di configurazione di esempio (`*.conf`)
- `tests/` – test unitari e di integrazione
- `Makefile` – script di build
- `README.md` e `REPORT.md` – documenti originali ora riuniti in questo file
- `bin/` – contiene i binari compilati (generata dal Makefile)
- `build/` – file oggetto temporanei
- `logs/` – file di log prodotti dall'esecuzione

## 8. Conclusioni

Il progetto fornisce una base solida per simulare un sistema di gestione delle emergenze con thread e code POSIX. L'approccio modulare consente di estendere facilmente le funzionalità. Questo documento riassume sia le informazioni introduttive sia i dettagli architetturali e operativi utili per compilare ed eseguire il software.

