#ifndef MODELS_H
#define MODELS_H

/* --------------- soccorritore ---------------- */
typedef struct {
    char name[32];
    int  number;
    int  speed;
    int  x, y;
} rescuer_type_t;

/* --------------- tipo di emergenza ------------ */
typedef struct {
    char name[32];
    int  priority;
    int  required_units[10];
    int  n_required;
    int  time_to_manage;
} emergency_type_t;

/* --------------- emergenza attiva ------------- */
typedef struct {
    int  id;
    char type[32];
    int  x, y;
    int  priority;
    long creation_time;
} emergency_t;

/* --------------- config dell’ambiente --------- */
typedef struct {
    char queue_name[64];    /* POSIX queue name */
    int  width;             /* larghezza griglia */
    int  height;            /* altezza griglia */
} env_config_t;

/* --------------- formato del messaggio MQ ----- */
typedef struct {
    int  id;
    char type[32];
    int  x, y;
    int  priority;
    long timestamp;      /* epoch time when request was generated */
} emergency_request_t;

#endif /* MODELS_H */

