# Report del Progetto Lab2

## Introduzione
Questo documento descrive l'architettura del sistema di gestione delle emergenze, le scelte progettuali adottate e le istruzioni per compilare ed eseguire i componenti software. Il progetto nasce come esercizio didattico e implementa un server in grado di gestire diverse richieste di soccorso, assegnando in modo dinamico i soccorritori disponibili. La documentazione è organizzata nelle sezioni seguenti:

1. Panoramica dell'architettura
2. Scelte progettuali
3. Istruzioni di build e uso
4. Struttura dei file
5. Conclusioni e possibili estensioni

---

## 1. Panoramica dell'architettura

Il sistema è basato su un'architettura modulare (Figura 1) che comprende i seguenti elementi principali:

- **Listener della coda**: un thread che riceve i messaggi di emergenza da una coda POSIX.
- **Scheduler**: il componente che decide quale soccorritore assegnare a ogni emergenza.
- **Digital Twin dei soccorritori**: un insieme di thread che simulano lo spostamento e le attività dei soccorritori reali.
- **Modulo di logging**: registra su file gli eventi chiave del sistema.
- **Parser di configurazione**: caricano le informazioni da file `conf/*.conf`.
- **Code di gestione interna**: per lo scambio di richieste tra i thread.

```text
          +-------------------------------+
          |          POSIX MQ             |
          +---------------+---------------+
                          |
                 +--------v-------+
                 |   Listener     |
                 +--------+-------+
                          | queue interna
                 +--------v-------+
                 |   Scheduler    |
                 +--------+-------+
                          |
       +------------------+------------------+
       |                  |                  |
+------v------+   +------v------+   +-------v-----+
|  Digital    |   |  Digital    |   |   Digital   |
|  Twin #0    |   |  Twin #1    |   |   Twin #N   |
+-------------+   +-------------+   +-------------+
```

**Figura 1** – Schema semplificato dei thread principali e dei flussi di dati.

Ogni emergenza viene inviata tramite un messaggio (`emergency_request_t`) a una coda POSIX definita in `env.conf`. Il server lancia un thread che ascolta questa coda e inserisce i messaggi in una coda circolare interna (`bqueue_t`). Lo scheduler preleva le richieste da questa coda, verifica la disponibilità dei soccorritori e li assegna alla gestione dell'emergenza.

I soccorritori sono rappresentati da thread "digital twin" che simulano l'avvicinamento al luogo dell'emergenza, l'intervento e il rientro alla base. Questo consente di verificare il corretto funzionamento dello scheduler e delle politiche di assegnazione senza disporre di dispositivi fisici.

### Componenti principali

1. **server.c** – Punto di ingresso del server. Si occupa di caricare i file di configurazione, avviare i thread di digital twin e lo scheduler, e infine il listener sulla coda.
2. **scheduler.c** – Implementa la logica di assegnazione delle emergenze e include un meccanismo di "aging" e un monitor di deadlock che aumenta la priorità delle emergenze in pausa se trascorre troppo tempo.
3. **digital_twin.c** – Ogni thread in questa unità rappresenta un soccorritore. Riceve istruzioni dallo scheduler, esegue spostamenti e fasi di lavoro simulati, e riporta l'esito dell'intervento.
4. **mq_manager.c** – Gestisce l'apertura della coda POSIX e l'invio/ricezione di messaggi. È nascosto dietro un'interfaccia per semplificarne l'uso da parte del server.
5. **Parser** – I file `parse_rescuers.c`, `parse_emergency_types.c` e `parse_env.c` estraggono le configurazioni da file testuali. Le strutture sono definite in `models.h`.
6. **Log** – Il modulo `log.c` produce un file `logs/server.log` con timestamp, identificatore e descrizione di ogni evento.

I test unitari si trovano in `tests/` e coprono la maggior parte dei moduli, compresa l'interazione dello scheduler con le priorità e il monitor di deadlock.

---

## 2. Scelte progettuali

Di seguito le principali decisioni prese per l'implementazione del progetto.

### Linguaggio e standard

Il codice è scritto in C e segue lo standard C11. Sono abilitate opzioni di compilazione come `-Wall` e `-Wextra` per ottenere il maggior numero di avvisi possibili e mantenere il codice più robusto. L'uso di `-pthread` è necessario per abilitare i thread POSIX.

### Modularità e separazione delle responsabilità

Ogni funzionalità rilevante è stata isolata in un modulo dedicato. In questo modo è più semplice capire dove intervenire per futuri miglioramenti o eventuali correzioni.

- `scheduler.c` contiene soltanto la logica di pianificazione e di monitoraggio.
- `digital_twin.c` è dedicato alla simulazione dei soccorritori.
- Il parsing dei vari file è distribuito in moduli distinti (`parse_env`, `parse_rescuers`, `parse_emergency_types`).
- `log.c` fornisce un'interfaccia semplice per generare messaggi su file, senza mescolare codice di I/O con la logica principale.

### Sincronizzazione tra thread

L'uso di mutex, variabili di condizione e queue circolari permette di coordinare i thread in modo relativamente sicuro:

- La coda `bqueue_t` è protetta da mutex e condizione `not_empty`/`not_full` per evitare condizioni di gara.
- Nel digital twin, ogni soccorritore ha un proprio mutex e condizione per segnalare l'avvio di un nuovo incarico o l'eventuale preemption.
- Lo scheduler mantiene una coda di emergenze "in pausa". Un thread secondario controlla se una di esse è bloccata per più di 30 secondi, aumentando la priorità per evitare starvation.

### Gestione delle priorità

I tipi di emergenza definiscono la priorità base e i soccorritori necessari. Il file `emergency_types.conf` permette di specificare per ogni tipo di emergenza quante unità (e di che tipo) servono e il tempo di gestione previsto. Lo scheduler adotta un modello a priorità: quelle più alte hanno scadenze più strette. Se lo scheduler non riesce ad assegnare un intervento entro il limite di tempo calcolato, l'emergenza va in timeout.

### Logging e tracciabilità

Ogni azione significativa produce una riga nel file log. Questo facilita il debug e la verifica delle funzionalità. Il log riporta anche eventuali errori di parsing o situazioni di timeout/preempted.

---

## 3. Istruzioni di build e uso

Per compilare e testare il sistema è fornito un `Makefile` all'interno della cartella `progetti.3/lab2`. I target principali sono i seguenti:

```sh
make -C progetti.3/lab2 server          # compila il server
make -C progetti.3/lab2 client          # compila il client
make -C progetti.3/lab2 test-utils      # esegue i test delle utility
make -C progetti.3/lab2 test-parsers    # esegue i test dei parser
make -C progetti.3/lab2 test-parse-env  # esegue i test del parser env
make -C progetti.3/lab2 test-deadlock   # esegue i test del monitor deadlock
make -C progetti.3/lab2 test-scheduler  # esegue il test di integrazione dello scheduler
```

Il target predefinito è `build`, equivalente a `make -C progetti.3/lab2 server`. Una volta compilato il server, si può avviare con `make run` oppure manualmente:

```sh
./bin/server -r conf/rescuers.conf -e conf/emergency_types.conf -n conf/env.conf
```

È possibile fornire percorsi alternativi ai file di configurazione tramite le opzioni `-r`, `-e` e `-n`.

### Struttura dei file di configurazione

- `rescuers.conf`: contiene un elenco di soccorritori nel formato `[nome][numero][velocità][x;y]`. Esempio:
  [DRONE][5][4][0;0]
  [AUTOMEDICA][1][3][0;0]
- `emergency_types.conf`: definisce i tipi di emergenza e le risorse richieste. Una riga tipica è:
  [INCIDENTE_STRADALE] [1] DRONE:1,10; AUTOMEDICA:1,20;
  Significa che per un incidente stradale (priorità 1) servono un drone per 10 secondi e un'auto medica per 20 secondi.
- `env.conf`: parametri generali dell'ambiente. Ad esempio:
  queue=/emergenze123
  width=20
  height=20

I log verranno salvati nella cartella `logs`. Se non esiste, il server la creerà automaticamente.

### Esecuzione del client di test

Il progetto include un client di esempio (`src/client.c`) che invia richieste sulla coda. Una volta compilato con `make client`, può essere lanciato con:

```sh
./bin/client conf/emergency_types.conf
```

Il client legge i tipi di emergenza e ne genera alcuni in sequenza per testare il funzionamento del server.

---

## 4. Struttura dei file

Per facilitare l'orientamento, ecco una panoramica delle directory principali del progetto:

- `src/` – file sorgenti in C.
- `include/` – header pubblici con le definizioni delle strutture.
- `conf/` – esempi di file di configurazione (`*.conf`).
- `tests/` – test unitari e di integrazione.
- `Makefile` – script di build con i target descritti sopra.
- `README.md` – breve introduzione al progetto e quick-start.

La directory `bin/` viene generata dal Makefile e ospita i binari compilati. Analogamente, `build/` contiene i file oggetto temporanei e `logs/` i file di log.

La figura seguente mostra la gerarchia del progetto semplificata:

```text
progetti.3/lab2/
├── Makefile
├── conf/
│   ├── emergency_types.conf
│   ├── env.conf
│   └── rescuers.conf
├── include/
│   ├── digital_twin.h
│   ├── log.h
│   ├── models.h
│   ├── mq_manager.h
│   ├── parse_emergency_types.h
│   ├── parse_env.h
│   ├── parse_rescuers.h
│   ├── queue.h
│   ├── scheduler.h
│   └── utils.h
├── src/
│   ├── client.c
│   ├── digital_twin.c
│   ├── log.c
│   ├── mq_manager.c
│   ├── parse_emergency_types.c
│   ├── parse_env.c
│   ├── parse_rescuers.c
│   ├── queue.c
│   ├── scheduler.c
│   └── server.c
└── tests/
    ├── test_deadlock.c
    ├── test_parse_env.c
    ├── test_parsers.c
    ├── test_scheduler.c
    └── test_utils.c
```

---

## 5. Conclusioni e possibili estensioni

Il progetto fornisce una base solida per simulare un sistema di gestione delle emergenze con thread e code POSIX. L'approccio modulare permette di aggiungere nuove funzionalità senza modifiche invasive. Alcune idee di estensione potrebbero essere:

- Integrazione con una vera rete di sensori o dispositivi fisici per ricevere segnalazioni reali.
- Interfaccia web o dashboard per monitorare in tempo reale lo stato delle emergenze e dei soccorritori.
- Persistenza su database degli interventi completati, per analisi successive.
- Aggiunta di meccanismi di fault tolerance, come il salvataggio periodico della coda e il riavvio automatico degli interventi in caso di crash.

Il presente documento, insieme al `README.md`, copre le informazioni essenziali richieste per la consegna: descrizione dell'architettura, motivazioni delle scelte di implementazione e procedure di compilazione ed esecuzione.

---
