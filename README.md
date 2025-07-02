# Progetto Lab2

This repository contains a simplified emergency management system used for
educational purposes. It includes parsers, a digital twin module and a
scheduler which assigns rescuers to incoming emergency requests.

## Deadlock detection

A monitoring thread now checks paused emergencies. If a request stays in the
paused queue for more than 30 seconds it is automatically bumped to a higher
priority. This may force preemption of lower priority assignments and avoids
resource deadlocks/starvation.

Unit tests cover the new escalation logic via `test-deadlock`.

## Server configuration

The server reads three configuration files:

* `env.conf` for queue settings and grid size
* `rescuers.conf` describing available rescuers
* `emergency_types.conf` defining emergency categories

Default files are looked up under the `conf` directory.  Custom paths can
be provided using command line options when launching the server:

```sh
./bin/server -r my_rescuers.conf -e my_emergencies.conf -n my_env.conf
```

The `-n` flag accepts an alternative `env.conf` file.  Edit its `queue=` entry
to change the POSIX queue name used by the server instead of the default
`/emergenze123`.

Runtime logs are stored in `logs/server.log`.

## Build and test

The `Makefile` under `progetti.3/lab2` provides convenient targets:

```sh
make -C progetti.3/lab2 server   # build the server binary
make -C progetti.3/lab2 client   # build the client binary
make -C progetti.3/lab2 test-utils      # run unit tests for utils
make -C progetti.3/lab2 test-parsers    # run parser tests
make -C progetti.3/lab2 test-parse-env  # run env parser tests
make -C progetti.3/lab2 test-deadlock   # run deadlock monitor tests
make -C progetti.3/lab2 test-scheduler  # run scheduler integration test
```

The default `make build` target builds the server, while `make run` launches it
with the default configuration files.
