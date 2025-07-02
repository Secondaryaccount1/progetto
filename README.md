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
