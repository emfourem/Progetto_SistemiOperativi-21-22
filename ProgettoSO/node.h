#ifndef HEADER
#define HEADER
#include "header.h"

transaction vuota(void);
void dealloca_coda(void);
int estrai_random(int estremo_sx,int estremo_dx);
int crea_coda(void);
transaction genera_transazione(int reward);
void handle_sigusr2(int sig);
void reserve_sem(int sem_id);
void wait_for_zero(int sem_id);
void release_sem(int sem_id);
int sem_find(int destinatario);
void wait_nsec_block(void);
#endif